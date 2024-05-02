// Lab8 - Shadow mapping.cpp : Defines the entry point for the console application.
//
#include <ctime>

#include <stdlib.h>
#include <stdio.h>
#include <math.h> 

#include <GL/glew.h>

#define GLM_FORCE_CTOR_INIT 
#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "OBJ_Loader.h"
#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")

// settings
//const unsigned int SCR_WIDTH = 1920;
//const unsigned int SCR_HEIGHT = 1080;
objl::Loader Loader;


// settings
const unsigned int SCR_WIDTH = 1440;
const unsigned int SCR_HEIGHT = 1440;

enum ECameraMovementType
{
	UNKNOWN,
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
private:
	// Default camera values
	const float zNEAR = 0.1f;
	const float zFAR = 500.f;
	const float YAW = -90.0f;
	const float PITCH = 0.0f;
	const float FOV = 45.0f;
	glm::vec3 startPosition;

public:
	Camera(const int width, const int height, const glm::vec3& position)
	{
		startPosition = position;
		Set(width, height, position);
	}

	void Set(const int width, const int height, const glm::vec3& position)
	{
		this->isPerspective = true;
		this->yaw = YAW;
		this->pitch = PITCH;

		this->FoVy = FOV;
		this->width = width;
		this->height = height;
		this->zNear = zNEAR;
		this->zFar = zFAR;

		this->worldUp = glm::vec3(0, 1, 0);
		this->position = position;

		lastX = width / 2.0f;
		lastY = height / 2.0f;
		bFirstMouseMove = true;

		UpdateCameraVectors();
	}

	void Reset(const int width, const int height)
	{
		Set(width, height, startPosition);
	}

	void Reshape(int windowWidth, int windowHeight)
	{
		width = windowWidth;
		height = windowHeight;

		// define the viewport transformation
		glViewport(0, 0, windowWidth, windowHeight);
	}

	const glm::vec3 GetPosition() const
	{
		return position;
	}

	const glm::mat4 GetViewMatrix() const
	{
		// Returns the View Matrix
		return glm::lookAt(position, position + forward, up);
	}

	const glm::mat4 GetProjectionMatrix() const
	{
		glm::mat4 Proj = glm::mat4(1);
		if (isPerspective) {
			float aspectRatio = ((float)(width)) / height;
			Proj = glm::perspective(glm::radians(FoVy), aspectRatio, zNear, zFar);
		}
		else {
			float scaleFactor = 2000.f;
			Proj = glm::ortho<float>(
				-width / scaleFactor, width / scaleFactor,
				-height / scaleFactor, height / scaleFactor, -zFar, zFar);
		}
		return Proj;
	}

	void ProcessKeyboard(ECameraMovementType direction, float deltaTime)
	{
		float velocity = cameraSpeedFactor * deltaTime;
		glm::vec3 forwardDirection = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
		glm::vec3 rightDirection = glm::normalize(glm::cross(forwardDirection, worldUp));

		switch (direction) {
		case ECameraMovementType::BACKWARD:
			position -= forwardDirection * velocity;
			break;
		case ECameraMovementType::FORWARD:
			position += forwardDirection * velocity;
			break;
		case ECameraMovementType::LEFT:
			position -= rightDirection * velocity;
			break;
		case ECameraMovementType::RIGHT:
			position += rightDirection * velocity;
			break;
		case ECameraMovementType::UP:
			position += worldUp * velocity;
			break;
		case ECameraMovementType::DOWN:
			position -= worldUp * velocity;
			if (position.y < 0)
			{
				position.y = 0;
			}
			break;
		}
	}

	void MouseControl(float xPos, float yPos)
	{
		if (bFirstMouseMove) {
			lastX = xPos;
			lastY = yPos;
			bFirstMouseMove = false;
		}

		float xChange = xPos - lastX;
		float yChange = lastY - yPos;
		lastX = xPos;
		lastY = yPos;

		if (fabs(xChange) <= 1e-6 && fabs(yChange) <= 1e-6) {
			return;
		}
		xChange *= mouseSensitivity;
		yChange *= mouseSensitivity;

		ProcessMouseMovement(xChange, yChange);
	}

	void ProcessMouseScroll(float yOffset)
	{
		if (FoVy >= 1.0f && FoVy <= 90.0f) {
			FoVy -= yOffset;
		}
		if (FoVy <= 1.0f)
			FoVy = 1.0f;
		if (FoVy >= 90.0f)
			FoVy = 90.0f;
	}

private:
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true)
	{
		yaw += xOffset;
		pitch += yOffset;

		//std::cout << "yaw = " << yaw << std::endl;
		//std::cout << "pitch = " << pitch << std::endl;

		// Avem grijã sã nu ne dãm peste cap
		if (constrainPitch) {
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		// Se modificã vectorii camerei pe baza unghiurilor Euler
		UpdateCameraVectors();
	}

	void UpdateCameraVectors()
	{
		// Calculate the new forward vector
		this->forward.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward.y = sin(glm::radians(pitch));
		this->forward.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		this->forward = glm::normalize(this->forward);
		// Also re-calculate the Right and Up vector
		right = glm::normalize(glm::cross(forward, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		up = glm::normalize(glm::cross(right, forward));
	}

protected:
	const float cameraSpeedFactor = 10.0f;
	const float mouseSensitivity = 0.1f;

	// Perspective properties
	float zNear;
	float zFar;
	float FoVy;
	int width;
	int height;
	bool isPerspective;

	glm::vec3 position;
	glm::vec3 forward;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 worldUp;

	// Euler Angles
	float yaw;
	float pitch;

	bool bFirstMouseMove = true;
	float lastX = 0.f, lastY = 0.f;
};

class Shader
{
public:
	// constructor generates the shaderStencilTesting on the fly
	// ------------------------------------------------------------------------
	Shader(const char* vertexPath, const char* fragmentPath)
	{
		Init(vertexPath, fragmentPath);
	}

	~Shader()
	{
		glDeleteProgram(ID);
	}

	// activate the shaderStencilTesting
	// ------------------------------------------------------------------------
	void Use() const
	{
		glUseProgram(ID);
	}

	unsigned int GetID() const { return ID; }

	// MVP
	unsigned int loc_model_matrix;
	unsigned int loc_view_matrix;
	unsigned int loc_projection_matrix;

	// utility uniform functions
	void SetInt(const std::string& name, int value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	void SetFloat(const std::string& name, float value) const
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	void SetVec3(const std::string& name, const glm::vec3& value) const
	{
		glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void SetVec3(const std::string& name, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
	}
	void SetMat4(const std::string& name, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	void Init(const char* vertexPath, const char* fragmentPath)
	{
		// 1. retrieve the vertex/fragment source code from filePath
		std::string vertexCode;
		std::string fragmentCode;
		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		// ensure ifstream objects can throw exceptions:
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		try {
			// open files
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			// read file's buffer contents into streams
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			// close file handlers
			vShaderFile.close();
			fShaderFile.close();
			// convert stream into string
			vertexCode = vShaderStream.str();
			fragmentCode = fShaderStream.str();
		}
		catch (std::ifstream::failure e) {
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
		}
		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		// 2. compile shaders
		unsigned int vertex, fragment;
		// vertex shaderStencilTesting
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		CheckCompileErrors(vertex, "VERTEX");
		// fragment Shader
		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		CheckCompileErrors(fragment, "FRAGMENT");
		// shaderStencilTesting Program
		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);
		CheckCompileErrors(ID, "PROGRAM");

		// 3. delete the shaders as they're linked into our program now and no longer necessery
		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	// utility function for checking shaderStencilTesting compilation/linking errors.
	// ------------------------------------------------------------------------
	void CheckCompileErrors(unsigned int shaderStencilTesting, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM") {
			glGetShaderiv(shaderStencilTesting, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shaderStencilTesting, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else {
			glGetProgramiv(shaderStencilTesting, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shaderStencilTesting, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
private:
	unsigned int ID;
};

Camera* pCamera = nullptr;

unsigned int CreateTexture(const std::string& strTexturePath)
{
	unsigned int textureId = -1;

	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture: " << strTexturePath << std::endl;
	}
	stbi_image_free(data);

	return textureId;
}
bool isLightRotating = false; // Flag to indicate if light is rotating
float rotationSpeed = 0.7f; // Adjust for desired rotation speed

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

void renderWall(const Shader& shader);
void renderParallelepipedFromDoor();
void renderParallelepipedPerpendiculuarFromDoor();
void renderParallelepipedTopDoor();
void renderParallelepipedTopDoorRoom3();
void renderFloor1(const Shader& shader);
void renderFloor();
void renderCeiling();

void renderGrassGround(const Shader& shader);

void renderGround(const Shader& shader);
void renderGround();

void renderGiraffe(const Shader& shader);
void renderGiraffe();

void renderCheetah(const Shader& shader);
void renderCheetah();

void renderMonkey(const Shader& shader);
void renderMonkey();

void renderPanther(const Shader& shader);
void renderPanther();


void renderSavannahTree(const Shader& shader);
void renderSavannahTree();
void renderParallelepipedParalelFirstDoor();


// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;

namespace fs = std::filesystem;
unsigned int leafTexture;
int main(int argc, char** argv)
{


	std::string strFullExeFileName = argv[0];
	std::string strExePath;
	const size_t last_slash_idx = strFullExeFileName.rfind('\\');
	size_t last_slash_idx1 = strFullExeFileName.find_last_of("\\");
	size_t last_slash_idx2 = strFullExeFileName.find_last_of("\\", last_slash_idx1 - 1);
	size_t last_slash_idx3 = strFullExeFileName.find_last_of("\\", last_slash_idx2 - 1);
	if (std::string::npos != last_slash_idx3) {
		strExePath = strFullExeFileName.substr(0, last_slash_idx3);
	}
	std::cout << strExePath << "\n";
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Lab8 - Maparea umbrelor", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();

	// Create camera
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(-10.0, 1.0, 20.0));

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile shaders
	// -------------------------
	Shader shadowMappingShader("ShadowMapping.vs", "ShadowMapping.fs");
	Shader shadowMappingDepthShader("ShadowMappingDepth.vs", "ShadowMappingDepth.fs");

	// load textures
	// -------------
	unsigned int floorTexture = CreateTexture(strExePath + "\\Museum\\Walls\\floor1.jpg");
	unsigned int wallTexture = CreateTexture(strExePath + "\\Museum\\Walls\\wall.jpg");

	unsigned int giraffeTexture = CreateTexture(strExePath + "\\Museum\\Animals\\Giraffe\\giraffe.jpg");
	unsigned int cheetahTexture = CreateTexture(strExePath + "\\Museum\\Animals\\Cheetah\\cheetah.png");

	unsigned int monkeyTexture = CreateTexture(strExePath + "\\Museum\\Animals\\Monkey1\\Gorilla_Bake1_PBR StoA_Diffuse.png");
	unsigned int pantherTexture = CreateTexture(strExePath + "\\Museum\\Animals\\Panther\\panther.jpg");


	unsigned int savannahGroundTexture = CreateTexture(strExePath + "\\Museum\\Walls\\SavannahGround\\test.jpg");
	unsigned int grassGroundTexture = CreateTexture(strExePath + "\\Museum\\Walls\\Grass.jpg");



	unsigned int treeTexture = CreateTexture(strExePath + "\\Museum\\Tree\\savannahTree\\bark_0021.jpg");
	//unsigned int leafTexture = CreateTexture(strExePath + "\\Museum\\Tree\\Tree\\branch.png");
	leafTexture = CreateTexture(strExePath + "\\Museum\\Tree\\Tree\\branch.png");

	// std::string strExePath; // Asigur?-te c? aceast? variabil? este ini?ializat? corect în codul t?u
	 // Restul codului pentru ini?ializarea lui strExePath ...

	//std::string imagePath = strExePath + "\\Museum\\Animals\\Giraffe\\giraffe.jpg";
	std::string imagePath = strExePath + "\\Museum\\Animals\\Cheetah\\cheetah.png";




	// Verific? dac? directorul "Museum" exist?
	if (fs::exists(strExePath + "\\Museum")) {
		// Verific? dac? fi?ierul "giraffe.jpg" exist? în interiorul directorului "Museum"
		if (fs::exists(imagePath)) {
			std::cout << "Calea catre imagine este corecta.\n";
		}
		else {
			std::cout << "Fisierul 'giraffe.jpg' nu exista în directorul 'Museum'.\n";
		}
	}
	else {
		std::cout << "Directorul 'Museum' nu exista în calea specificata.\n";
	}



	//aici trebuie modificat pentru viziunea camerei
	// 
	// 

	// configure depth map FBO
	// -----------------------
	//const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

	//this need to be the same as :
	// settings
   // const unsigned int SCR_WIDTH = 1440;
	//const unsigned int SCR_HEIGHT = 1440;
	//this is pov of the camera:
	const unsigned int SHADOW_WIDTH = 1440, SHADOW_HEIGHT = 1440;
	unsigned int depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	unsigned int depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	// shader configuration
	// --------------------
	shadowMappingShader.Use();
	shadowMappingShader.SetInt("diffuseTexture", 0);
	shadowMappingShader.SetInt("shadowMap", 1);

	// lighting info
	// -------------
	glm::vec3 lightPos(-2.0f, 4.0f, -1.0f);

	glEnable(GL_CULL_FACE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// Update light position based on rotation state and time
		if (isLightRotating) {
			double currentTime = glfwGetTime();
			float angle = (float)currentTime * rotationSpeed;
			lightPos.x = 2.0f * cos(angle);
			lightPos.z = 2.0f * sin(angle);
		}
		// per-frame time logic
		// --------------------
		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 1. render depth of scene to texture (from light's perspective)
		glm::mat4 lightProjection, lightView;
		glm::mat4 lightSpaceMatrix;
		float near_plane = 1.0f, far_plane = 7.5f;
		lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, near_plane, far_plane);
		lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		lightSpaceMatrix = lightProjection * lightView;

		// render scene from light's point of view
		shadowMappingDepthShader.Use();
		shadowMappingDepthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wallTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderWall(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderFloor1(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, savannahGroundTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderGround(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassGroundTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderGrassGround(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, treeTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderSavannahTree(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, giraffeTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderGiraffe(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cheetahTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderCheetah(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, monkeyTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderMonkey(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pantherTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderPanther(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// reset viewport
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// 2. render scene as normal using the generated depth/shadow map 
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		shadowMappingShader.Use();
		glm::mat4 projection = pCamera->GetProjectionMatrix();
		glm::mat4 view = pCamera->GetViewMatrix();
		shadowMappingShader.SetMat4("projection", projection);
		shadowMappingShader.SetMat4("view", view);
		// set light uniforms
		shadowMappingShader.SetVec3("viewPos", pCamera->GetPosition());
		shadowMappingShader.SetVec3("lightPos", lightPos);
		shadowMappingShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderFloor1(shadowMappingShader);




		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, wallTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderWall(shadowMappingShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, savannahGroundTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderGround(shadowMappingShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassGroundTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderGrassGround(shadowMappingShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, treeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderSavannahTree(shadowMappingShader);



		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, giraffeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderGiraffe(shadowMappingShader);

		/*glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, giraffeTexture);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		renderGiraffe(shadowMappingDepthShader);
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, giraffeTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderGiraffe(shadowMappingShader);*/



		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cheetahTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderCheetah(shadowMappingShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, monkeyTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderMonkey(shadowMappingShader);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pantherTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		glDisable(GL_CULL_FACE);
		renderPanther(shadowMappingShader);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();
	}



	// optional: de-allocate all resources once they've outlived their purpose:
	delete pCamera;

	glfwTerminate();
	return 0;
}
void renderFloor1(const Shader& shader)
{
	// floor
	glm::mat4 model;
	shader.SetMat4("model", model);
	renderFloor();
}


// renders the 3D scene
// --------------------
void renderWall(const Shader& shader)
{
	// floor
	glm::mat4 model, model1;
	shader.SetMat4("model", model);
	renderFloor();

	//ROOM 1

	//right from door room1
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-4.2f, 4.9f, -10.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedFromDoor();


	// top door room1
	model1 = glm::mat4();
	model1 = glm::translate(model1, glm::vec3(-16.5f, 9.6f, -10.2));
	model1 = glm::scale(model1, glm::vec3(5.2f));
	shader.SetMat4("model", model1);
	renderParallelepipedTopDoor();

	// left from door room1
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-22.2f, 4.9f, -10.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedFromDoor();

	// lateral wall room1
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-25.0f, 4.9f, -44.5));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedPerpendiculuarFromDoor();

	// ceiling room1
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-4.2f, 20.1f, -44.5));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderCeiling();

	// lateral wall room1
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-15.6f, 4.9f, -44.5));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedParalelFirstDoor();

	//ROOM2

	// left lateral wall room2
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-25.0f, 4.9f, -9.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedPerpendiculuarFromDoor();

	//right lateral wall room2
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(5.2f, 4.9f, -9.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedPerpendiculuarFromDoor();

	// ceiling room2
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-4.2f, 20.1f, -9.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderCeiling();

	//front lateral wall room2
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-15.6f, 4.9f, 26.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedParalelFirstDoor();

	//ROOM3

	// ceiling room3
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(27.0f, 20.1f, -44.5));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderCeiling();

	//back lateral wall room3
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(15.5f, 4.9f, -44.5));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedParalelFirstDoor();

	//front lateral wall room3
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(15.5f, 4.9f, -10.2));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedParalelFirstDoor();


	//right lateral wall room3
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(37.2f, 4.9f, -44.5));
	model = glm::scale(model, glm::vec3(5.2f));
	shader.SetMat4("model", model);
	renderParallelepipedPerpendiculuarFromDoor();

	//// top door room3
	//model1 = glm::mat4();
	//model1 = glm::translate(model1, glm::vec3(15.5f, 9.6f, -30.2));
	//model1 = glm::scale(model1, glm::vec3(5.2f));
	//shader.SetMat4("model", model1);
	//renderParallelepipedTopDoorRoom3();
}


unsigned int planeVAO = 0;
void renderFloor()
{
	unsigned int planeVBO;

	float scaleFactor = 5.0f;

	if (planeVAO == 0) {
		// set up vertex data (and buffer(s)) and configure vertex attributes
		//float planeVertices[] = {
		//	// positions          // normals          // texcoords
		//	52.7f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Top-right corner (shifted further right)
		//	-22.3f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Top-left corner (shifted further right)
		//	-22.3f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // Bottom-left corner
		//	52.7f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f  // Bottom-right corner
		//};
		float planeVertices[] = {
			// positions            // normals         // texcoords
			25.0f * scaleFactor, -0.5f,  25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f * scaleFactor, -0.5f,  25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-25.0f * scaleFactor, -0.5f, -25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			25.0f * scaleFactor, -0.5f,  25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-25.0f * scaleFactor, -0.5f, -25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  0.0f, 25.0f,
			25.0f * scaleFactor, -0.5f, -25.0f * scaleFactor,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
		};




		// plane VAO
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}

	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderParallelepipedFromDoor()
{
	// initialize (if necessary)
	if (cubeVAO == 0)
	{
		float skew = 1.8f;
		float height = 1.0f;
		float width = 0.38f;

		float vertices[] = {
			// back face
			-1.0f - width, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f - width, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f - width,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f - width, -1.0f,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f - width,  1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f - width, -1.0f,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f - width,  1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f - width,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f - width, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - width, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - width, -1.0f,  1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - width,  1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f , -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f ,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f , -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f , -1.0f,  1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f,  1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f + height , 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}
unsigned int cubeVAO3 = 0;
unsigned int cubeVBO3 = 0;

void renderParallelepipedPerpendiculuarFromDoor()
{
	// initialize (if necessary)
	if (cubeVAO3 == 0)
	{
		float skew = 1.8f;
		float height = 1.0f;
		float width = 4.8f;

		float vertices[] = {
			// back face
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f - skew,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f - skew,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f, -1.0f, 1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f - skew, -1.0f, 1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f - skew,  1.0f + height, 1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f - skew,  1.0f + height, 1.0f + width,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f,  1.0f + height, 1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f, -1.0f, 1.0f + width,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f,  1.0f + height, 1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f, -1.0f, 1.0f + width, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f,  1.0f + height, 1.0f + width, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f - skew,  1.0f + height, 1.0f + width,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f - skew, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f - skew,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f - skew, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f - skew,  1.0f + height, 1.0f + width,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f - skew, -1.0f, 1.0f + width,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f - skew, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f - skew, -1.0f, 1.0f + width,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f - skew, -1.0f, 1.0f + width,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f, -1.0f, 1.0f + width,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f - skew ,  1.0f + height , 1.0f + width,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f - skew ,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f - skew,  1.0f + height, 1.0f + width,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f,  1.0f + height, 1.0f + width,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		glGenVertexArrays(1, &cubeVAO3);
		glGenBuffers(1, &cubeVBO3);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO3);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO3);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO3);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

unsigned int cubeVAO2 = 0;
unsigned int cubeVBO2 = 0;
void renderParallelepipedTopDoor()
{

	// initialize (if necessary)
	if (cubeVAO2 == 0)
	{
		float skew = 1.8f;
		float door_sizing = 0.9f;
		float height = 1.0f;

		float vertices[] = {
			// back face
			-1.0f + door_sizing, -1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f + door_sizing, -1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f + door_sizing, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f + door_sizing, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f + door_sizing, -1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f + door_sizing, -1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f + door_sizing, -1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f - door_sizing + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f + door_sizing, -1.0f + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f + height , -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f + door_sizing, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f + door_sizing, -1.0 + height, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f - door_sizing + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO2);
		glGenBuffers(1, &cubeVBO2);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO2);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO2);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO2);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

unsigned int cubeVAO4 = 0;
unsigned int cubeVBO4 = 0;
void renderCeiling()
{
	// initialize (if necessary)
	if (cubeVAO4 == 0)
	{
		float skew = 1.8f;
		float ceilingLength = 4.0f;
		float ceilingWidth = 4.8f;
		float ceilingWidthBack = 2.0f;
		float ceilingHeight = 2.0f;




		float vertices[] = {
			// back face
			-1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f - skew, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f - skew, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f - ceilingLength,  1.0f - skew, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - skew,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f - skew,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f - ceilingLength,  1.0f - skew,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f - ceilingLength,  1.0f - skew,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f - ceilingLength,  1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - ceilingLength,  1.0f - skew,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f - skew,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f - skew, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f - skew,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f + ceilingWidth + skew,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f - ceilingLength, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f - ceilingLength,  1.0f - skew, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f - skew , 1.0f - ceilingWidthBack ,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - skew, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f - skew,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f - ceilingLength,  1.0f - skew, -1.0f + ceilingWidth + skew + 0.2f ,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f - ceilingLength,  1.0f - skew,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO4);
		glGenBuffers(1, &cubeVBO4);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO4);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO4);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO4);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

unsigned int cubeVAO5 = 0;
unsigned int cubeVBO5 = 0;
void renderParallelepipedParalelFirstDoor()
{
	// initialize (if necessary)
	if (cubeVAO5 == 0)
	{
		float skew = 1.8f;
		float height = 1.0f;
		float side = 2.2f;

		float vertices[] = {
			// back face
			-1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f + side,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f + side, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f + side,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f - skew, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f - skew,  1.0f + height, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f - skew, -1.0f, 1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f + side, -1.0f, 1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f + side,  1.0f + height, 1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f + side,  1.0f + height, 1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f - skew,  1.0f + height, 1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f - skew, -1.0f, 1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f - skew,  1.0f + height, 1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f - skew,  1.0f + height, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f - skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - skew, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - skew, -1.0f, 1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - skew,  1.0f + height, 1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f + side,  1.0f + height, 1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f + side, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f + side,  1.0f + height, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f + side, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f + side,  1.0f + height, 1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f + side, -1.0f, 1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f - skew, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f, 1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f, 1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f - skew, -1.0f, 1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - skew, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f + skew,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f ,  1.0f + height , 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f ,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f ,  1.0f + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f + skew,  1.0f + height, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f + skew,  1.0f + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};

		glGenVertexArrays(1, &cubeVAO5);
		glGenBuffers(1, &cubeVBO5);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO5);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO5);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO5);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}


unsigned int cubeVAO6 = 0;
unsigned int cubeVBO6 = 0;
void renderParallelepipedTopDoorRoom3()
{

	// initialize (if necessary)
	if (cubeVAO6 == 0)
	{
		float skew = 1.8f;
		float door_sizing = 0.9f;
		float height = 1.0f;
		float newSkew = 1.4f;

		float vertices[] = {
			// back face
			-1.0f + door_sizing, -1.0f + height, -1.0f - skew,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f + height, -1.0f - skew,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f + door_sizing, -1.0f + height, -1.0f - skew,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f , -1.0f + height,  1.0f - skew ,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f - skew, -1.0f + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f - skew,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f - skew,  1.0f - door_sizing + height,  1.0f - skew ,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f ,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f , -1.0f + height,  1.0f - skew ,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f + door_sizing,  1.0f - door_sizing + height, -1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f + door_sizing, -1.0f + height, -1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f + door_sizing, -1.0f + height, -1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f + door_sizing, -1.0f + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f + door_sizing,  1.0f - door_sizing + height,  1.0f - skew, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f- door_sizing,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f - door_sizing, -1.0f + height, -1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f - door_sizing,  1.0f - door_sizing + height, -1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f - door_sizing , -1.0f + height, -1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f - door_sizing,  1.0f - door_sizing + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f - door_sizing , -1.0f + height,  1.0f - skew,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f , -1.0f + height, -1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f-skew, -1.0f + height , -1.0f - skew ,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f - skew, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f - skew, -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f , -1.0f + height,  1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f , -1.0 + height, -1.0f - skew,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f ,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f - door_sizing + height, 1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - door_sizing + height, -1.0f,  0.0f - skew,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f,  1.0f - door_sizing + height, -1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f ,  1.0f - door_sizing + height,  1.0f - skew,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
		};
		glGenVertexArrays(1, &cubeVAO6);
		glGenBuffers(1, &cubeVBO6);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO6);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cubeVAO6);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cubeVAO6);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		pCamera->ProcessKeyboard(FORWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		pCamera->ProcessKeyboard(BACKWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		pCamera->ProcessKeyboard(LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		pCamera->ProcessKeyboard(RIGHT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
		pCamera->ProcessKeyboard(UP, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
		pCamera->ProcessKeyboard(DOWN, (float)deltaTime);

	if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		isLightRotating = true;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		isLightRotating = false;


	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		pCamera->Reset(width, height);

	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}

void renderGiraffe(const Shader& shader)
{
	//giraffe

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-23.5f, 4.5f, -1.5f));
	model = glm::scale(model, glm::vec3(0.1f));
	model = glm::rotate(model, glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Adaugarea rota?iei spre dreapta
	float rotationAngle = 120.0f; // Rotire spre dreapta
	model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire spre dreapta pe axa OZ

	shader.SetMat4("model", model);
	renderGiraffe();
}

float vertices[82000];
unsigned int indices[72000];
unsigned int indicesGr[72000];
objl::Vertex verG[82000];
unsigned int indicesG[72000];


GLuint giraffeVAO, giraffeVBO, giraffeEBO;

void renderGiraffe()
{


	// initialize (if necessary)
	if (giraffeVAO == 0)
	{

		std::vector<float> verticess;
		std::vector<float> indicess;



		Loader.LoadFile("..\\Museum\\Animals\\Giraffe\\giraffe.obj");

		std::string filePath = "..\\Museum\\Animals\\Giraffe\\giraffe.obj";

		if (fs::exists(filePath)) {
			std::cout << "Fisierul giraffe.obj exista.\n";
		}
		else {
			std::cerr << "Fisierul giraffe.obj nu exista.\n";
		}


		objl::Mesh curMesh = Loader.LoadedMeshes[0];
		int size = curMesh.Vertices.size();
		objl::Vertex v;
		const float scaleFactor = 0.5f; // factorul de scalare
		for (int j = 0; j < curMesh.Vertices.size(); j++)
		{
			v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
			v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
			v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
			v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
			v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
			v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
			v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
			v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


			verG[j] = v;
		}
		for (int j = 0; j < verticess.size(); j++)
		{
			vertices[j] = verticess.at(j);
		}

		for (int j = 0; j < curMesh.Indices.size(); j++)
		{

			indicess.push_back((float)curMesh.Indices[j]);

		}
		for (int j = 0; j < curMesh.Indices.size(); j++)
		{
			indicesG[j] = indicess.at(j);
		}

		glGenVertexArrays(1, &giraffeVAO);
		glGenBuffers(1, &giraffeVBO);
		glGenBuffers(1, &giraffeEBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, giraffeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verG), verG, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, giraffeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesG), &indicesG, GL_DYNAMIC_DRAW);
		// link vertex attributes
		glBindVertexArray(giraffeVAO);
		glEnableVertexAttribArray(0);


		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(giraffeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, giraffeVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, giraffeEBO);
	int indexArraySize;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
	glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);


}


unsigned int indicesC[72000];
objl::Vertex verC[82000];

GLuint cheetahVAO, cheetahVBO, cheetahEBO;

void renderCheetah(const Shader& shader)
{
	//cheetah

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-25.5f, 1.55f, -9.5f));
	model = glm::scale(model, glm::vec3(6.f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Adaugarea rota?iei spre dreapta
	float rotationAngle = 70.0f; // Rotire spre dreapta
	model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
	shader.SetMat4("model", model);
	renderCheetah();
}



void renderCheetah()
{
	// initialize (if necessary)
	if (cheetahVAO == 0)
	{

		std::vector<float> verticess;
		std::vector<float> indicess;



		Loader.LoadFile("..\\Museum\\Animals\\Cheetah\\cheetah.obj");
		objl::Mesh curMesh = Loader.LoadedMeshes[0];
		int size = curMesh.Vertices.size();
		objl::Vertex v;
		const float scaleFactor = 0.5f; // factorul de scalare
		for (int j = 0; j < curMesh.Vertices.size(); j++)
		{
			v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
			v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
			v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
			v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
			v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
			v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
			v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
			v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


			verC[j] = v;
		}
		for (int j = 0; j < verticess.size(); j++)
		{
			vertices[j] = verticess.at(j);
		}

		for (int j = 0; j < curMesh.Indices.size(); j++)
		{

			indicess.push_back((float)curMesh.Indices[j]);

		}
		for (int j = 0; j < curMesh.Indices.size(); j++)
		{
			indicesC[j] = indicess.at(j);
		}

		glGenVertexArrays(1, &cheetahVAO);
		glGenBuffers(1, &cheetahVBO);
		glGenBuffers(1, &cheetahEBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, cheetahVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verC), verC, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cheetahEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesC), &indicesC, GL_DYNAMIC_DRAW);
		// link vertex attributes
		glBindVertexArray(cheetahVAO);
		glEnableVertexAttribArray(0);


		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(cheetahVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cheetahVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cheetahEBO);
	int indexArraySize;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
	glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}
void renderGround(const Shader& shader)
{

	//Ground

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-26.6f, -2.3f, -12.1f));
	model = glm::scale(model, glm::vec3(2.3f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	shader.SetMat4("model", model);
	renderGround();



	/*model = glm::mat4();
	model = glm::translate(model, glm::vec3(3.25f, 0.4f, -19.95f));
	model = glm::scale(model, glm::vec3(2.6f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	shader.SetMat4("model", model);
	renderGround();*/


}
unsigned int savanVAO = 0;

void renderGrassGround(const Shader& shader)
{

	//Ground

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-7.6f, -2.3f, -12.1f));
	model = glm::scale(model, glm::vec3(2.3f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	shader.SetMat4("model", model);
	renderGround();



	/*model = glm::mat4();
	model = glm::translate(model, glm::vec3(3.25f, 0.4f, -19.95f));
	model = glm::scale(model, glm::vec3(2.6f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	shader.SetMat4("model", model);
	renderGround();*/


}
void renderGround()
{
	unsigned int planeVBO;

	float scaleFactor = 0.2f;

	if (savanVAO == 0) {
		// set up vertex data (and buffer(s)) and configure vertex attributes
		//float planeVertices[] = {
		//	// positions          // normals          // texcoords
		//	52.7f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // Top-right corner (shifted further right)
		//	-22.3f*scaleFactor, -0.5f, 50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // Top-left corner (shifted further right)
		//	-22.3f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // Bottom-left corner
		//	52.7f*scaleFactor, -0.5f, -50.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f  // Bottom-right corner
		//};
		 // set up vertex data (and buffer(s)) and configure vertex attributes
		float skew = 1.8f;
		float skew1 = 1.0f;
		float ceilingLength = 2.3f;
		float ceilingWidth = 13.40f;
		float ceilingWidthBack = 2.0f;
		float ceilingHeight = 2.0f;




		float planeVertices[] = {
			// back face
			-1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			1.0f,  1.0f - skew1, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
			1.0f,  1.0f - skew1, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
			-1.0f - ceilingLength, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
			-1.0f - ceilingLength,  1.0f - skew1, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
			// front face
			-1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
			-1.0f - ceilingLength,  1.0f - skew1,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
			-1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
			// left face
			-1.0f - ceilingLength,  1.0f - skew1,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			-1.0f - ceilingLength,  1.0f - skew1, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
			-1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - ceilingLength, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
			-1.0f - ceilingLength, -1.0f,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - ceilingLength,  1.0f - skew1,  1.0f + ceilingWidth, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
			// right face
			1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f - skew1, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
			1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
			1.0f,  1.0f - skew1,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
			1.0f, -1.0f,  1.0f + ceilingWidth,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
			// bottom face
			-1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew + 0.2f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			1.0f, -1.0f, -1.0f + ceilingWidth + skew + 0.2f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
			1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			1.0f, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
			-1.0f - ceilingLength, -1.0f,  1.0f - ceilingWidthBack,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
			-1.0f - ceilingLength, -1.0f, -1.0f + ceilingWidth + skew + 0.2f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
			// top face
			-1.0f - ceilingLength,  1.0f - skew1, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			1.0f,  1.0f - skew1 , 1.0f - ceilingWidthBack ,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			1.0f,  1.0f - skew1, -1.0f + ceilingWidth + skew + 0.2f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
			1.0f,  1.0f - skew1,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
			-1.0f - ceilingLength,  1.0f - skew1, -1.0f + ceilingWidth + skew + 0.2f ,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
			-1.0f - ceilingLength,  1.0f - skew1,  1.0f - ceilingWidthBack,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left   
		};




		// plane VAO
		glGenVertexArrays(1, &savanVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(savanVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}

	glBindVertexArray(savanVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

unsigned int indicesST[72000];
objl::Vertex verST[82000];

GLuint savanTreeVAO, savanTreeVBO, savanTreeEBO;
GLuint leafVAO, leafVBO, leafEBO;

void renderSavannahTree()
{
	// initialize (if necessary)
	if (savanTreeVAO == 0)
	{
		// Load tree object file
		Loader.LoadFile("..\\Museum\\Tree\\savannahTree\\Tree.obj");

		// Generate VAO, VBO, EBO for tree
		glGenVertexArrays(1, &savanTreeVAO);
		glGenBuffers(1, &savanTreeVBO);
		glGenBuffers(1, &savanTreeEBO);

		// Bind VAO for tree
		glBindVertexArray(savanTreeVAO);

		// Bind vertex buffer for tree
		glBindBuffer(GL_ARRAY_BUFFER, savanTreeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * Loader.LoadedVertices.size(), &Loader.LoadedVertices[0], GL_STATIC_DRAW);

		// Bind index buffer for tree
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, savanTreeEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * Loader.LoadedIndices.size(), &Loader.LoadedIndices[0], GL_STATIC_DRAW);

		// Set vertex attribute pointers for tree
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);

		// Unbind VAO for tree
		glBindVertexArray(0);
	}

	// Draw the tree
	glBindVertexArray(savanTreeVAO);
	glDrawElements(GL_TRIANGLES, Loader.LoadedIndices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void renderSavannahTree(const Shader& shader)
{
	// Set?m matricea de modelare pentru copac
	glm::mat4 model = glm::mat4(1.0f); // Identitate
	model = glm::translate(model, glm::vec3(-23.5f, 0.0f, 11.5f)); // Transla?ie
	model = glm::scale(model, glm::vec3(4.f)); // Scalare
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotire pentru a-l face vertical

	// Transmiterea matricei de modelare pentru copac la shader
	shader.SetMat4("model", model);

	// Activare textura frunzelor
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, leafTexture);
	shader.SetInt("leafTexture", 1);

	// Desen?m copacul (trunchiul ?i frunzele)
	renderSavannahTree();
}

unsigned int indicesM[72000];
objl::Vertex verM[2000000];

GLuint monkeyVAO, monkeyVBO, monkeyEBO;

void renderMonkey(const Shader& shader)
{
	//cheetah

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-29.5f, 3.0f, 27.5f));
	model = glm::scale(model, glm::vec3(6.f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Adaugarea rota?iei spre dreapta
	float rotationAngle = 70.0f; // Rotire spre dreapta
	model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
	float rotationAngle1 = 90.0f; // Rotire spre dreapta
	model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
	shader.SetMat4("model", model);
	renderMonkey();
}




void renderMonkey()
{
	// initialize (if necessary)
	if (monkeyVAO == 0)
	{

		std::vector<float> verticess;
		std::vector<float> indicess;



		Loader.LoadFile("..\\Museum\\Animals\\Monkey1\\gorilla.obj");
		objl::Mesh curMesh = Loader.LoadedMeshes[0];
		int size = curMesh.Vertices.size();
		objl::Vertex v;
		const float scaleFactor = 0.5f; // factorul de scalare
		for (int j = 0; j < curMesh.Vertices.size(); j++)
		{
			v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
			v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
			v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
			v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
			v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
			v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
			v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
			v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


			verM[j] = v;
		}
		for (int j = 0; j < verticess.size(); j++)
		{
			vertices[j] = verticess.at(j);
		}

		for (int j = 0; j < curMesh.Indices.size(); j++)
		{

			indicess.push_back((float)curMesh.Indices[j]);

		}
		for (int j = 0; j < curMesh.Indices.size(); j++)
		{
			indicesM[j] = indicess.at(j);
		}

		glGenVertexArrays(1, &monkeyVAO);
		glGenBuffers(1, &monkeyVBO);
		glGenBuffers(1, &monkeyEBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, monkeyVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verM), verM, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, monkeyEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesM), &indicesM, GL_DYNAMIC_DRAW);
		// link vertex attributes
		glBindVertexArray(monkeyVAO);
		glEnableVertexAttribArray(0);


		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(monkeyVAO);
	glBindBuffer(GL_ARRAY_BUFFER, monkeyVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, monkeyEBO);
	int indexArraySize;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
	glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}


unsigned int indicesP[72000];
objl::Vertex verP[2000000];

GLuint pantherVAO, pantherVBO, pantherEBO;

void renderPanther(const Shader& shader)
{
	//cheetah

	glm::mat4 model;
	model = glm::mat4();
	model = glm::translate(model, glm::vec3(-25.5f, 0.0f, 5.5f));
	model = glm::scale(model, glm::vec3(6.f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Adaugarea rota?iei spre dreapta
	float rotationAngle = 170.0f; // Rotire spre dreapta
	model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotire spre dreapta pe axa OZ
	float rotationAngle1 = 90.0f; // Rotire spre dreapta
	model = glm::rotate(model, glm::radians(rotationAngle1), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotire spre dreapta pe axa OZ
	shader.SetMat4("model", model);
	renderPanther();
}




void renderPanther()
{
	// initialize (if necessary)
	if (pantherVAO == 0)
	{

		std::vector<float> verticess;
		std::vector<float> indicess;



		Loader.LoadFile("..\\Museum\\Animals\\Panther\\PANTHER.obj");
		objl::Mesh curMesh = Loader.LoadedMeshes[0];
		int size = curMesh.Vertices.size();
		objl::Vertex v;
		const float scaleFactor = 0.5f; // factorul de scalare
		for (int j = 0; j < curMesh.Vertices.size(); j++)
		{
			v.Position.X = (float)curMesh.Vertices[j].Position.X * scaleFactor;
			v.Position.Y = (float)curMesh.Vertices[j].Position.Y * scaleFactor;
			v.Position.Z = (float)curMesh.Vertices[j].Position.Z * scaleFactor;
			v.Normal.X = (float)curMesh.Vertices[j].Normal.X;
			v.Normal.Y = (float)curMesh.Vertices[j].Normal.Y;
			v.Normal.Z = (float)curMesh.Vertices[j].Normal.Z;
			v.TextureCoordinate.X = (float)curMesh.Vertices[j].TextureCoordinate.X;
			v.TextureCoordinate.Y = (float)curMesh.Vertices[j].TextureCoordinate.Y;


			verP[j] = v;
		}
		for (int j = 0; j < verticess.size(); j++)
		{
			vertices[j] = verticess.at(j);
		}

		for (int j = 0; j < curMesh.Indices.size(); j++)
		{

			indicess.push_back((float)curMesh.Indices[j]);

		}
		for (int j = 0; j < curMesh.Indices.size(); j++)
		{
			indicesP[j] = indicess.at(j);
		}

		glGenVertexArrays(1, &pantherVAO);
		glGenBuffers(1, &pantherVBO);
		glGenBuffers(1, &pantherEBO);
		// fill buffer
		glBindBuffer(GL_ARRAY_BUFFER, pantherVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(verP), verP, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pantherEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesP), &indicesP, GL_DYNAMIC_DRAW);
		// link vertex attributes
		glBindVertexArray(pantherVAO);
		glEnableVertexAttribArray(0);


		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
	// render Cube
	glBindVertexArray(pantherVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pantherVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pantherEBO);
	int indexArraySize;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &indexArraySize);
	glDrawElements(GL_TRIANGLES, indexArraySize / sizeof(unsigned int), GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}