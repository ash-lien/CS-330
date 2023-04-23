#include "ShapeGenerator.h"
#include "ShapeData.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "shader.h"
#include "camera.h"
#include "cylinder.h"


#include <iostream>

//Shader program Macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

using namespace std; // Standard namespace
// Unnamed namespace
namespace
{
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbo;         // Handle for the vertex buffer object
		GLuint nVertices;    // Number of indices of the mesh
	};

	// Triangle mesh data
	GLMesh gMesh;
	// Shader program
	GLuint gProgramId;
	GLuint gLampProgramId;
	// Subject position and scale
	glm::vec3 gPyramidPosition(0.0f, 0.0f, 0.0f);
	glm::vec3 gPyramidScale(2.0f);

	// Pyramid and light color
	//m::vec3 gObjectColor(0.6f, 0.5f, 0.75f);
	glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
	glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

	// Light position and scale
	glm::vec3 gLightPosition(1.5f, 0.5f, 3.0f);
	glm::vec3 gLightScale(0.3f);
}



void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
unsigned int loadTexture(const char* path);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

/* Vertex Shader Source Code*/
const GLchar* pyramidVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
}
);


/* Fragment Shader Source Code*/
const GLchar* pyramidFragmentShaderSource = GLSL(440,
	in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;

void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient lighting*/
	float ambientStrength = 1.0f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse = impact * lightColor; // Generate diffuse light color

	//Calculate Specular lighting*/
	float specularIntensity = 0.8f; // Set specular light strength
	float highlightSize = 8.0f; // Set specular highlight size
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;

	// Calculate phong result
	vec3 phong = (ambient + diffuse + specular) * objectColor;

	fragmentColor = vec4(phong, 1.0f); // Send lighting results to GPU
}
);

/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

		//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);


/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,

	out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
}
);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(1.5f, 3.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

bool perspective = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;







// offset variables for plane, sphere
const uint NUM_VERTICES_PER_TRI = 3;
const uint NUM_FLOATS_PER_VERTICE = 9;
const uint VERTEX_BYTE_SIZE = NUM_FLOATS_PER_VERTICE * sizeof(float);




// plane
GLuint planeNumIndices;
GLuint planeVertexArrayObjectID;
GLuint planeIndexByteOffset;

// sphere
GLuint sphereNumIndices;
GLuint sphereVertexArrayObjectID;
GLuint sphereIndexByteOffset;

GLuint sphereNumIndices2;
GLuint sphereVertexArrayObjectID2;
GLuint sphereIndexByteOffset2;

// projection matrix
glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Scene", NULL, NULL);
	if (window == NULL)
	{
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

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	// build and compile our shader zprogram
	// ------------------------------------

	Shader lightingShader("shaderfiles/6.multiple_lights.vs", "shaderfiles/6.multiple_lights.fs");
	Shader lightCubeShader("shaderfiles/6.light_cube.vs", "shaderfiles/6.light_cube.fs");

	// positions of the point lights
	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.8f,  2.8f,  -1.2f),
		glm::vec3(2.0f, 0.5f, 1.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	};

	// cup handle cube's VAO and VBO
	unsigned int cubeVBO, cubeVAO;

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);

	glBindVertexArray(cubeVAO);
	// position attribute for cube
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture attribute for cube
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// normal attribute for cube
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(2);
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	

	// lightCube's VAO and VBO
	unsigned int lightCubeVAO;
	glGenVertexArrays(1, &lightCubeVAO);
	glBindVertexArray(lightCubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);


	// creates plane object
	ShapeData plane = ShapeGenerator::makePlane(10);

	unsigned int planeVBO{}, planeVAO;
	glGenVertexArrays(1, &planeVAO);
	glGenBuffers(1, &planeVBO);

	glBindVertexArray(planeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
	glBufferData(GL_ARRAY_BUFFER, plane.vertexBufferSize() + plane.indexBufferSize(), 0, GL_STATIC_DRAW);

	GLsizeiptr currentOffset = 0;

	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, plane.vertexBufferSize(), plane.vertices);
	currentOffset += plane.vertexBufferSize();

	planeIndexByteOffset = currentOffset;

	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, plane.indexBufferSize(), plane.indices);

	planeNumIndices = plane.numIndices;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeVBO);

	// creates sphere object
	ShapeData sphere = ShapeGenerator::makeSphere();

	unsigned int sphereVBO{}, sphereVAO;
	glGenVertexArrays(1, &sphereVAO);
	glGenBuffers(1, &sphereVBO);
	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sphere.vertexBufferSize() + sphere.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere.vertexBufferSize(), sphere.vertices);
	currentOffset += sphere.vertexBufferSize();
	sphereIndexByteOffset = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere.indexBufferSize(), sphere.indices);
	sphereNumIndices = sphere.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO);

	/*float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
		 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};*/

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);


	glm::mat4 model;
	float angle;


	ShapeData sphere2 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO2{}, sphereVAO2;
	glGenVertexArrays(1, &sphereVAO2);
	glGenBuffers(1, &sphereVBO2);
	glBindVertexArray(sphereVAO2);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO2);
	glBufferData(GL_ARRAY_BUFFER, sphere2.vertexBufferSize() + sphere2.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere2.vertexBufferSize(), sphere2.vertices);
	currentOffset += sphere2.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere2.indexBufferSize(), sphere2.indices);
	sphereNumIndices2 = sphere2.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO2);

	ShapeData sphere3 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO3{}, sphereVAO3;
	glGenVertexArrays(1, &sphereVAO3);
	glGenBuffers(1, &sphereVBO3);
	glBindVertexArray(sphereVAO3);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO3);
	glBufferData(GL_ARRAY_BUFFER, sphere3.vertexBufferSize() + sphere3.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere3.vertexBufferSize(), sphere3.vertices);
	currentOffset += sphere3.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere3.indexBufferSize(), sphere3.indices);
	sphereNumIndices2 = sphere3.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO3);

	ShapeData sphere4 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO4{}, sphereVAO4;
	glGenVertexArrays(1, &sphereVAO4);
	glGenBuffers(1, &sphereVBO4);
	glBindVertexArray(sphereVAO4);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO4);
	glBufferData(GL_ARRAY_BUFFER, sphere4.vertexBufferSize() + sphere4.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere4.vertexBufferSize(), sphere4.vertices);
	currentOffset += sphere4.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere4.indexBufferSize(), sphere4.indices);
	sphereNumIndices2 = sphere4.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO4);

	ShapeData sphere5 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO5{}, sphereVAO5;
	glGenVertexArrays(1, &sphereVAO5);
	glGenBuffers(1, &sphereVBO5);
	glBindVertexArray(sphereVAO5);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO5);
	glBufferData(GL_ARRAY_BUFFER, sphere5.vertexBufferSize() + sphere5.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere5.vertexBufferSize(), sphere5.vertices);
	currentOffset += sphere5.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere5.indexBufferSize(), sphere5.indices);
	sphereNumIndices2 = sphere5.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO5);

	ShapeData sphere6 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO6{}, sphereVAO6;
	glGenVertexArrays(1, &sphereVAO6);
	glGenBuffers(1, &sphereVBO6);
	glBindVertexArray(sphereVAO6);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO6);
	glBufferData(GL_ARRAY_BUFFER, sphere6.vertexBufferSize() + sphere6.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere6.vertexBufferSize(), sphere6.vertices);
	currentOffset += sphere6.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere6.indexBufferSize(), sphere6.indices);
	sphereNumIndices2 = sphere6.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO6);

	ShapeData sphere7 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO7{}, sphereVAO7;
	glGenVertexArrays(1, &sphereVAO7);
	glGenBuffers(1, &sphereVBO7);
	glBindVertexArray(sphereVAO7);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO7);
	glBufferData(GL_ARRAY_BUFFER, sphere7.vertexBufferSize() + sphere7.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere7.vertexBufferSize(), sphere7.vertices);
	currentOffset += sphere7.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere7.indexBufferSize(), sphere7.indices);
	sphereNumIndices2 = sphere7.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO7);

	ShapeData sphere8 = ShapeGenerator::makeSphere();
	unsigned int sphereVBO8{}, sphereVAO8;
	glGenVertexArrays(1, &sphereVAO8);
	glGenBuffers(1, &sphereVBO8);
	glBindVertexArray(sphereVAO8);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO8);
	glBufferData(GL_ARRAY_BUFFER, sphere8.vertexBufferSize() + sphere8.indexBufferSize(), 0, GL_STATIC_DRAW);
	currentOffset = 0;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere8.vertexBufferSize(), sphere8.vertices);
	currentOffset += sphere8.vertexBufferSize();
	sphereIndexByteOffset2 = currentOffset;
	glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sphere8.indexBufferSize(), sphere8.indices);
	sphereNumIndices2 = sphere8.numIndices;
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 3));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERTEX_BYTE_SIZE, (void*)(sizeof(float) * 6));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereVBO8);


	// load textures using utility function
	unsigned int planeTexture = loadTexture("pinkwood.jpg");
	unsigned int sphereTexture = loadTexture("pink.png");
	unsigned int stickTexture = loadTexture("paper.png");
	unsigned int sphere2Texture = loadTexture("bluehardcandytexture.png");
	unsigned int gummyWorm1Texture = loadTexture("yellow.png");
	unsigned int gummyWorm2Texture = loadTexture("redcandytexture.png");
	unsigned int cubeTexture = loadTexture("rainbowsugartexture.png");

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	lightingShader.use();
	lightingShader.setInt("material.diffuse", 0);
	lightingShader.setInt("material.specular", 1);



	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		lightingShader.use();
		lightingShader.setVec3("viewPos", camera.Position);
		lightingShader.setFloat("material.shininess", 32.0f);

		// directional light
		lightingShader.setVec3("dirLight.direction", 2.5f, 0.0f, 0.0f);
		lightingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
		lightingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);
		// key light
		lightingShader.setVec3("pointLights[0].position", pointLightPositions[0]);
		lightingShader.setVec3("pointLights[0].ambient", 0.5f, 0.5f, 0.5f);
		lightingShader.setVec3("pointLights[0].diffuse", 0.5f, 0.5f, 0.5f);
		lightingShader.setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[0].constant", 1.0f);
		lightingShader.setFloat("pointLights[0].linear", 0.09);
		lightingShader.setFloat("pointLights[0].quadratic", 0.032);
		// fill light
		lightingShader.setVec3("pointLights[1].position", pointLightPositions[1]);
		lightingShader.setVec3("pointLights[1].ambient", 0.7f, 0.7f, 0.7f);
		lightingShader.setVec3("pointLights[1].diffuse", 0.1f, 0.1f, 0.1f);
		lightingShader.setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[1].constant", 1.0f);
		lightingShader.setFloat("pointLights[1].linear", 0.09);
		lightingShader.setFloat("pointLights[1].quadratic", 0.032);
		// point light 3
		lightingShader.setVec3("pointLights[2].position", pointLightPositions[2]);
		lightingShader.setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[2].constant", 1.0f);
		lightingShader.setFloat("pointLights[2].linear", 0.09);
		lightingShader.setFloat("pointLights[2].quadratic", 0.032);
		// point light 4
		lightingShader.setVec3("pointLights[3].position", pointLightPositions[3]);
		lightingShader.setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
		lightingShader.setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
		lightingShader.setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("pointLights[3].constant", 1.0f);
		lightingShader.setFloat("pointLights[3].linear", 0.09);
		lightingShader.setFloat("pointLights[3].quadratic", 0.032);
		// spotLight
		lightingShader.setVec3("spotLight.position", camera.Position);
		lightingShader.setVec3("spotLight.direction", camera.Front);
		lightingShader.setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
		lightingShader.setVec3("spotLight.diffuse", 0.7f, 0.7f, 0.7f);
		lightingShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
		lightingShader.setFloat("spotLight.constant", 1.0f);
		lightingShader.setFloat("spotLight.linear", 0.09);
		lightingShader.setFloat("spotLight.quadratic", 0.032);
		lightingShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
		lightingShader.setFloat("spotLight.outerCutOff", glm::cos(glm::radians(15.0f)));

		// view/projection transformations
		glm::mat4 view = camera.GetViewMatrix();
		lightingShader.setMat4("projection", projection);
		lightingShader.setMat4("view", view);

		// world transformation
		glm::mat4 model = glm::mat4(1.0f);
		lightingShader.setMat4("model", model);



		// setup to draw plane
		glBindTexture(GL_TEXTURE_2D, planeTexture);
		glBindVertexArray(planeVAO);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.5f, -0.45f, 0.0f));
		lightingShader.setMat4("model", model);

		// draw plane
		glDrawElements(GL_TRIANGLES, planeNumIndices, GL_UNSIGNED_SHORT, (void*)planeIndexByteOffset);

		// setup to draw cube
		glBindTexture(GL_TEXTURE_2D, cubeTexture);
		glBindVertexArray(cubeVAO);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(1.0f, 0.1, 0.0f));
		lightingShader.setMat4("model", model);

		// draw cube
		glDrawArrays(GL_TRIANGLES, 0, 36);



		// setup to draw sphere
		glBindTexture(GL_TEXTURE_2D, sphereTexture);
		glBindVertexArray(sphereVAO);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.8f, -2.0f));
		model = glm::scale(model, glm::vec3(0.5f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);

		// setup to draw sphere
		glBindTexture(GL_TEXTURE_2D, sphereTexture);
		glBindVertexArray(sphereVAO);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 1.8f, -2.0f));
		model = glm::scale(model, glm::vec3(0.5f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);

		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset);

		// setup to draw sphere 2
		glBindTexture(GL_TEXTURE_2D, sphere2Texture);
		glBindVertexArray(sphereVAO2);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.0f, -0.2f, -2.0f));
		model = glm::scale(model, glm::vec3(0.2f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		// setup to draw sphere 3
		glBindTexture(GL_TEXTURE_2D, gummyWorm1Texture);
		glBindVertexArray(sphereVAO3);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(3.0f, -0.1f, 1.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		// setup to draw sphere 4
		glBindTexture(GL_TEXTURE_2D, gummyWorm1Texture);
		glBindVertexArray(sphereVAO4);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.9f, -0.1f, 1.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		// setup to draw sphere 5
		glBindTexture(GL_TEXTURE_2D, gummyWorm1Texture);
		glBindVertexArray(sphereVAO5);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.8f, -0.1f, 1.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		// setup to draw sphere 6
		glBindTexture(GL_TEXTURE_2D, gummyWorm2Texture);
		glBindVertexArray(sphereVAO6);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.6f, -0.1f, 1.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		// setup to draw sphere 7
		glBindTexture(GL_TEXTURE_2D, gummyWorm2Texture);
		glBindVertexArray(sphereVAO7);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.5f, -0.1f, 1.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		// setup to draw sphere 8
		glBindTexture(GL_TEXTURE_2D, gummyWorm2Texture);
		glBindVertexArray(sphereVAO8);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(2.4f, -0.1f, 1.0f));
		model = glm::scale(model, glm::vec3(0.3f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);
		// draw sphere
		glDrawElements(GL_TRIANGLES, sphereNumIndices2, GL_UNSIGNED_SHORT, (void*)sphereIndexByteOffset2);

		

		// setup to draw cylinder
		glBindTexture(GL_TEXTURE_2D, stickTexture);
		glBindVertexArray(VAO);
		model = model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.5f, -2.0f));
		model = glm::scale(model, glm::vec3(0.5f)); // Make it a smaller sphere
		lightingShader.setMat4("model", model);

		static_meshes_3D::Cylinder C(0.2, 15, 1.8, true, true, true);
		C.render();





		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteBuffers(1, &cubeVBO);

	//glDeleteVertexArrays(1, &cylinderVAO);
	//glDeleteBuffers(1, &cylinderVBO);

	glDeleteVertexArrays(1, &planeVAO);
	glDeleteBuffers(1, &planeVBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);


	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		camera.ProcessKeyboard(UP, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		camera.ProcessKeyboard(DOWN, deltaTime);

	// change view between perspective and orthographics
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
		if (perspective) {
			projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
			perspective = false;
		}
		else {
			projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
			perspective = true;
		}
	}

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

unsigned int loadTexture(char const* path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}
	return textureID;
}
// Functioned called to render a frame
void URender()
{
	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Activate the cube VAO (used by cube and lamp)
	glBindVertexArray(gMesh.vao);

	// CUBE: draw cube
	//----------------
	// Set the shader to be used
	glUseProgram(gProgramId);

	

	// Retrieves and passes transform matrices to the Shader program
	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Reference matrix uniforms from the Shader program for the cub color, light color, light position, and camera position
	GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

	// Pass color, light, and camera data to the Shader program's corresponding uniforms
	glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
	
	

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

	// LAMP: draw lamp
	//----------------
	glUseProgram(gLampProgramId);

	

	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(gLampProgramId, "model");
	viewLoc = glGetUniformLocation(gLampProgramId, "view");
	projLoc = glGetUniformLocation(gLampProgramId, "projection");

	// Pass matrix data to the Lamp Shader program's matrix uniforms
	
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glDrawArrays(GL_TRIANGLES, 0, gMesh.nVertices);

	// Deactivate the Vertex Array Object and shader program
	glBindVertexArray(0);
	glUseProgram(0);

	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	
}


// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
	// Vertex Data
	GLfloat verts[] = {
		//Positions          //Texture Coordinates
		0.0f, 0.5f,  0.0f,  1.0f, 0.0f,
		0.5f, 0.0f, 0.0f,  1.0f, 1.0f,
		0.0f,  0.0f, 0.5f,  0.0f, 1.0f,
		//0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	   //-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	   //-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

		0.0f,  0.5f,  0.0f,  1.0f, 0.0f,
		0.0f,  0.0f,  -0.5f,  1.0f, 1.0f,
		0.5f,  0.0f,  0.0f,  0.0f, 1.0f,
		//0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	   //-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
	   //-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

	   0.0f,  0.5f,  0.0f,  1.0f, 0.0f,
	   -0.5f,  0.0f, 0.0f,  1.0f, 1.0f,
	   0.0f, 0.0f, -0.5f,  0.0f, 1.0f,
	   //-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	   //-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	   //-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

		0.0f,  0.5f,  0.0f,  1.0f, 0.0f,
		0.0f,  0.0f, 0.5f,  1.0f, 1.0f,
		-0.5f, 0.0f, 0.0f,  0.0f, 1.0f,
		//0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
		//0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
		//0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	   0.0f, 0.0f, -0.5f,  1.0f, 0.0f,
		0.5f, 0.0f, 0.0f,  1.0f, 1.0f,
		-0.5f, 0.0f,  0.0f,  0.0f, 1.0f,
		//0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	   //-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	   //-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

	   0.0f,  0.0f, 0.5f,  1.0f, 0.0f,
		-0.5f,  0.0f, 0.0f,  1.0f, 1.0f,
		0.5f,  0.0f,  0.0f,  1.0f, 0.0f,
		//0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	   //-0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	   //-0.5f,  0.5f, -0.5f,  0.0f, 1.0f
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerUV = 2;

	mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerUV));

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create VBO
	glGenBuffers(1, &mesh.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	// Strides between vertex coordinates
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerUV);

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
}


void UDestroyMesh(GLMesh& mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, &mesh.vbo);
}
// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}


void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

