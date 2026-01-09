#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <fstream>
#include <sstream>
#include <string>

#include "shaderClass.h"

#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>






// ----- size of window -----
int windowWidth = 1000;
int windowHeight = 600;

// ----- function used when window is resized -----
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
// ----- function to process keys -----
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}




// ----- terrain mesh generation from heightmap -----
struct Vertex 
{
	glm::vec3 Position;		// x, y, z
	glm::vec3 Normal;	   // how light reflects
	glm::vec2 TexCoords;   // u, v for mapping texture
};
// generate terrain vertices
std::vector<Vertex> generateTerrainVertices(const char* heightmapPath, int& outWidth, int& outHeight, float heightScale = 20.0f, float texRepeat = 10.0f)
{
	int width, height, numChannels;
	// load heightmap
	unsigned char* data = stbi_load(heightmapPath, &width, &height, &numChannels, 1);
	// need for generating indices
	outWidth = width;
	outHeight = height;
	// allocate vertex storage // one vertex per pixel
	std::vector<Vertex> vertices(width * height);

	// generating vertices // each pixel becomes a vertex at x, y, z
	for (int z = 0; z < height; ++z) {
		for (int x = 0; x < width; ++x) {
			// height calculation
			float y = data[z * width + x] / 255.0f * heightScale;   //read grayscale value, normalize it to [0.0, 1.0], becomes the height Y of the terrain
			// terrain is on the XZ plane
			vertices[z * width + x].Position = glm::vec3((float)x, y, (float)z);
			// maps texture
			vertices[z * width + x].TexCoords = glm::vec2((float)x / width * texRepeat, (float)z / height * texRepeat);
			// initial normal
			vertices[z * width + x].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
		}
	}

	// normals (for lighting) 
	for (int z = 0; z < height; ++z) {
    for (int x = 0; x < width; ++x) {

        float hl = vertices[z * width + glm::max(x - 1, 0)].Position.y;
        float hr = vertices[z * width + glm::min(x + 1, width - 1)].Position.y;
        float hd = vertices[glm::max(z - 1, 0) * width + x].Position.y;
        float hu = vertices[glm::min(z + 1, height - 1) * width + x].Position.y;

        glm::vec3 normal;
        normal.x = hl - hr;
        normal.y = 2.0f;
        normal.z = hd - hu;

        vertices[z * width + x].Normal = glm::normalize(normal);
    }
}

	stbi_image_free(data);
	return vertices;
};


// generate terrain indices
std::vector<unsigned int> generateTerrainIndices(int width, int height)
{
	std::vector<unsigned int> indices;
	for (int z = 0; z < height - 1; ++z) {
		for (int x = 0; x < width - 1; ++x) {
			int topLeft = z * width + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * width + x;
			int bottomRight = bottomLeft + 1;

			// two triangles (in quad)
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}
	return indices;
};











float getTerrainHeight(const std::vector<Vertex>& terrainVertices, int terrainWidth, int terrainHeight, float x, float z) {
	// clamp x, z inside terrain
	if (x < 0) x = 0;
	if (z < 0) z = 0;
	if (x > terrainWidth - 1) x = terrainWidth - 1;
	if (z > terrainHeight - 1) z = terrainHeight - 1;

	int ix = (int)x;
	int iz = (int)z;

	int ix1 = std::min(ix + 1, terrainWidth - 1);
	int iz1 = std::min(iz + 1, terrainHeight - 1);

	float fx = x - ix;
	float fz = z - iz;

	float h00 = terrainVertices[iz * terrainWidth + ix].Position.y;
	float h10 = terrainVertices[iz * terrainWidth + ix1].Position.y;
	float h01 = terrainVertices[iz1 * terrainWidth + ix].Position.y;
	float h11 = terrainVertices[iz1 * terrainWidth + ix1].Position.y;

	// bilinear interpolation
	float h0 = h00 * (1 - fx) + h10 * fx;
	float h1 = h01 * (1 - fx) + h11 * fx;
	return h0 * (1 - fz) + h1 * fz;
}
// camera
struct Camera {
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp;

	float Yaw;    // rotation around Y-axis
	float Pitch;  // rotation around X-axis

	float MovementSpeed;
	float MouseSensitivity;

	Camera(glm::vec3 startPos)
		: Position(startPos), Front(glm::vec3(0, 0, -1)), Up(glm::vec3(0, 1, 0)), WorldUp(glm::vec3(0, 1, 0)),
		Yaw(-90.0f), Pitch(0.0f), MovementSpeed(20.0f), MouseSensitivity(0.1f)
	{
		updateCameraVectors();
	}

	glm::mat4 GetViewMatrix() {
		return glm::lookAt(Position, Position + Front, Up);
	}

	void ProcessKeyboard(GLFWwindow* window, float deltaTime) {
		float velocity = MovementSpeed * deltaTime;
		glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
		glm::vec3 flatRight = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) Position += flatFront * velocity;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) Position -= flatFront * velocity;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) Position -= flatRight * velocity;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) Position += flatRight * velocity;
	}

	void ProcessMouseMovement(float xoffset, float yoffset) {
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		if (Pitch > 89.0f) Pitch = 89.0f;
		if (Pitch < -89.0f) Pitch = -89.0f;

		updateCameraVectors();
	}

private:
	void updateCameraVectors() {
		glm::vec3 front;
		front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		front.y = sin(glm::radians(Pitch));
		front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front = glm::normalize(front);
		Right = glm::normalize(glm::cross(Front, WorldUp));
		Up = glm::normalize(glm::cross(Right, Front));
	}
};
// half of window width
//float lastX = 640.0f;
float lastX = 500.0f;
// half of window height
//float lastY = 360.0f;
float lastY = 200.0f;
bool firstMouse = true;
Camera camera(glm::vec3(0.0f, 50.0f, 100.0f));
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}





// sun sphere (positioned where light is)
struct SimpleVertex {
	glm::vec3 position;
};

void generateSphere(
	float radius,
	int sectors,
	int stacks,
	std::vector<SimpleVertex>& vertices,
	std::vector<unsigned int>& indices
) {
	const float PI = M_PI;

	for (int i = 0; i <= stacks; ++i) {
		float stackAngle = PI / 2 - i * PI / stacks;
		float xy = radius * cos(stackAngle);
		float y = radius * sin(stackAngle);

		for (int j = 0; j <= sectors; ++j) {
			float sectorAngle = j * 2 * PI / sectors;

			float x = xy * cos(sectorAngle);
			float z = xy * sin(sectorAngle);

			vertices.push_back({ glm::vec3(x, y, z) });
		}
	}

	for (int i = 0; i < stacks; ++i) {
		int k1 = i * (sectors + 1);
		int k2 = k1 + sectors + 1;

		for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
			indices.push_back(k1);
			indices.push_back(k2);
			indices.push_back(k1 + 1);

			indices.push_back(k1 + 1);
			indices.push_back(k2);
			indices.push_back(k2 + 1);
		}
	}
}
















int main() {
	// ----- initialize glfw -----
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// ----- window -----
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Terrain", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// hide cursor 
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 
	glfwSetCursorPosCallback(window, mouse_callback);

	// check if glad works
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD\n";
		return -1;
	}
	glEnable(GL_DEPTH_TEST);
	// window location and size setting 
	glViewport(0, 0, windowWidth, windowHeight);
	// resizing of window 
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);



	// ----- shader -----
	Shader shaderProgram("default.vert", "default.frag");



	// ----- terrain -----
	int terrainWidth, terrainHeight;
	//auto vertices = generateTerrainVertices("heightmap257.png", terrainWidth, terrainHeight);
	//auto indices = generateTerrainIndices(terrainWidth, terrainHeight);
	std::vector<Vertex> vertices = generateTerrainVertices("heightmap257.png", terrainWidth, terrainHeight); 
	std::vector<unsigned int> indices = generateTerrainIndices(terrainWidth, terrainHeight);


	// ----- VAO/VBO/EBO -----
	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	// Normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(1);
	// TexCoords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);




	// load texture onto terrain
	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int texWidth, texHeight, texChannels;
	unsigned char* texData = stbi_load("grass.png", &texWidth, &texHeight, &texChannels, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, texChannels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, texData);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(texData);




	// camera / projection
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(terrainWidth / 2.0f, 50.0f, terrainHeight + 50.0f), glm::vec3(terrainWidth / 2.0f, 0.0f, terrainHeight / 2.0f), glm::vec3(0, 1, 0));
	glm::mat4 model = glm::mat4(1.0f);

	// Light
	glm::vec3 lightPos(terrainWidth / 1.0f, 100.0f, terrainHeight / 2.0f);



	// for camera
	float deltaTime = 0.0f;
	float lastFrame = 0.0f;


	// sun generation
	std::vector<SimpleVertex> sphereVertices;
	std::vector<unsigned int> sphereIndices;

	generateSphere(5.0f, 32, 16, sphereVertices, sphereIndices);

	unsigned int sphereVAO, sphereVBO, sphereEBO;
	glGenVertexArrays(1, &sphereVAO);
	glGenBuffers(1, &sphereVBO);
	glGenBuffers(1, &sphereEBO);

	glBindVertexArray(sphereVAO);

	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER,sphereVertices.size() * sizeof(SimpleVertex),sphereVertices.data(),GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,sphereIndices.size() * sizeof(unsigned int),sphereIndices.data(),GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SimpleVertex), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	// sun shader
	Shader sphereShader("light.vert", "light.frag");







	// ----- ----- ----- ----- ----- ----- ----- ----- ----- ----- //
	// ----- render loop -----
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glfwPollEvents();
		// camera movement
		camera.ProcessKeyboard(window, deltaTime);

		// stay withing terrain bounds (horizontal clamping)
		camera.Position.x = glm::clamp(camera.Position.x, 0.0f, (float)(terrainWidth - 1));
		camera.Position.z = glm::clamp(camera.Position.z, 0.0f, (float)(terrainHeight - 1));

		// smooth slope walking
		float eyeOffset = 2.0f;
		float targetY = getTerrainHeight(vertices, terrainWidth, terrainHeight, camera.Position.x, camera.Position.z) + eyeOffset;
		float lerpSpeed = 10.0f;
		camera.Position.y = glm::mix(camera.Position.y, targetY, deltaTime * lerpSpeed);



		// esc key? -> exit program
		processInput(window);

		// color
		glClearColor(0.5f, 0.7f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		
		// pass updated view matrix to shader
		glm::mat4 view = camera.GetViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// ----- draw objects -----
		// draw terrain
		shaderProgram.Activate();

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

		glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightPos"),lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(shaderProgram.ID, "lightColor"),1.0f, 1.0f, 1.0f);
		glUniform3f(glGetUniformLocation(shaderProgram.ID, "viewPos"),camera.Position.x, camera.Position.y, camera.Position.z);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(shaderProgram.ID, "terrainTexture"), 0);

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);



		// draw sun
		sphereShader.Activate();

		glm::mat4 sphereModel = glm::mat4(0.5f);

		sphereModel = glm::translate(sphereModel, lightPos);

		glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "model"),1, GL_FALSE, glm::value_ptr(sphereModel));
		glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "view"),1, GL_FALSE, glm::value_ptr(camera.GetViewMatrix()));
		glUniformMatrix4fv(glGetUniformLocation(sphereShader.ID, "projection"),1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(sphereVAO);
		glDrawElements(GL_TRIANGLES, sphereIndices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);


		// swap buffer
		glfwSwapBuffers(window);
		//glfwPollEvents();
	}

	// clean
	glDeleteVertexArrays(1, &sphereVAO);
	glDeleteBuffers(1, &sphereVBO);
	glDeleteBuffers(1, &sphereEBO);

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	shaderProgram.Delete();
	glfwTerminate();
	return 0;
}

