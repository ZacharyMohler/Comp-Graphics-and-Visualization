#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library

//glm headers
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//image loader for texturing
#include "stb_image.h"

using namespace std; // Uses the standard namespace

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Final Scene Zachary Mohler (sunset lighting, red source left, white source right)"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	struct GLMesh //define (in c) the GLMesh type
	{
		GLuint vao;			//vertex array object
		GLuint vbos[2];			//vertex buffer object
		GLuint nIndices;	//number of vertices in the mesh
	};

	GLFWwindow* gWindow = nullptr;
	//battery meshes
	GLMesh gMesh;
	//plane mesh
	GLMesh plane;
	//cube mesh
	GLMesh cube;
	GLMesh flatCylinder;
	GLMesh rect;

	GLuint gProgramId;

	//for standardized movement speed
	float deltaTime = 0.0f;	// Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame
	//for camera movement
	glm::vec3 cameraPos;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;
	//for camera look
	float lastX = WINDOW_WIDTH/2, lastY = WINDOW_HEIGHT/2;
	float yaw = -90;
	float pitch = 0;
	bool firstMouse = true;
	float cameraSpeed = 0.01;
	//for toggling orthonographic projection
	bool ortho = false;
	//for texturing 

}

//FUNCTION PROTOTYPES (IDENTIFIERS)===========================================================================================================

bool UInitialize(int, char*[], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
glm::vec3 UProcessInput(GLFWwindow* window);

void UCreateCylinderWallMesh(GLMesh &mesh, float numSections);
void UCreateFlatCylinderWallMesh(GLMesh &mesh, float numSections);
void UCreatePlaneMesh(GLMesh &mesh, float scale);
void UCreateCubeMesh(GLMesh &mesh);
void UCreateRectMesh(GLMesh &mesh);
void UDestroyMesh(GLMesh &mesh);

void URenderMesh(const GLMesh& mesh, glm::mat4 translation, glm::mat4 rotation, glm::mat4 scale, glm::mat4 view);

bool UCreateShaderProgram(const char* vertexShaderSource, const char* fragShaderSource, GLuint &programId);
void UDestroyShaderProgram(GLuint programId);

//mouse input callbacks
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
//p callback
void p_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

//VERTEX SHADER SOURCE =====================================================================================================================

const char *vertexShaderSource = "#version 440 core\n"
"layout (location = 0) in vec3 aPos;\n" //Position coordinates
"layout (location = 1) in vec4 colorFromVBO;\n" //color values
"layout (location = 2) in vec2 texCoordFromVBO;\n"  //texture coorinate values
"layout (location = 3) in vec3 aNormal;\n" //normal vector values (for lighting)

"out vec4 colorFromVS;\n" //to pass to FS
"out vec2 texCoord;\n"
"out vec3 normal;\n"
"out vec3 fragPos;\n"

"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"

"void main()\n"
"{\n"
"	normal = mat3(transpose(inverse(model))) * aNormal;\n"
"   gl_Position = projection * view * model * vec4(aPos, 1.0f);\n" //transforms vertices to clip coords (creates view)
"	colorFromVS = colorFromVBO;\n"
"	texCoord = vec2(texCoordFromVBO.x, texCoordFromVBO.y);\n"
"	fragPos = vec3(model * vec4(aPos, 1.0));\n"
"}\0";

//TEXTURED FRAGMENT SHADER SOURCE ===================================================================================================================

const char *fragmentShaderSource = "#version 440 core\n"
"struct Material\n" //material object for lighting properties
"{\n"
"vec3 ambient;\n"
"vec3 diffuse;\n"
"vec3 specular;\n"
"float shininess;\n"
"};\n"

"struct Light\n"
"{\n"
"	vec3 position;\n"
"	vec3 ambient;\n"
"	vec3 diffuse;\n"
"	vec3 specular;\n"
"	vec3 direction;\n"
"	vec3 direction2;\n"
"};\n"

"in vec4 colorFromVS;\n" //get color from VS
"in vec2 texCoord;\n" //get texture coordinates from VS
"in vec3 fragPos;\n" //get fragment position from VS
"in vec3 normal;\n"

"out vec4 FragColor;\n" 

//get color for object and light from uniforms
"uniform vec3 objColor;\n"
"uniform vec3 lightColor;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 lightPos2;\n"
"uniform vec3 cameraPos;\n"
"uniform Material material;\n"
"uniform Light light;\n"
//uniform for setting texture
"uniform sampler2D ourTexture;\n"

"void main()\n"
"{\n"
//LIGHT1/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//directional lighting
"	vec3 lightDir = normalize(-light.direction);\n"

//ambient lighting
"	vec3 ambient = light.ambient * material.ambient;\n"

//diffuse lighting
"	vec3 norm = normalize(normal);\n"
"	//vec3 lightDir = normalize(lightPos - fragPos);\n"
"	float diff = max(dot(norm, lightDir), 0.0);\n"
"	vec3 diffuse = (diff * material.diffuse) * light.diffuse;\n"

//specular lighting
"	vec3 viewDir = normalize(cameraPos - fragPos);\n"
"	vec3 reflectDir = reflect(-lightDir, norm);\n"//, norm;\n"
"	float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n" //32 is the 'shininess'
"	vec3 specular = (spec * material.specular) * (vec3(0.5, 0.5, 0.5) * light.specular);\n"


//"	vec3 result = (ambient + diffuse + specular) * texture(ourTexture, texCoord).rgb;\n"// * objColor;\n"
//"   FragColor = vec4(result, 1.0);\n"// colorFromVS;\n //set our color to the one we got from the FS
//LIGHT2 ///////////////////////////////////////////////////////////////////////////////////////////////////////////
//directional lighting
"	vec3 lightDir2 = normalize(-light.direction2);\n" //opposite the first one

//diffuse lighting
"	float diff2 = max(dot(norm, lightDir2), 0.0);\n"
"	diffuse = diffuse + ((diff2 * material.diffuse) * (vec3(1.0, 0.5, 0.25) * light.diffuse));\n"

//specular lighting
"	vec3 reflectDir2 = reflect(-lightDir2, norm);\n"//, norm;\n"
"	float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), material.shininess);\n" //32 is the 'shininess'
"	specular = specular + ((spec2 * material.specular) * (vec3(1.0, 0.5, 0.25) * light.specular));\n"


"	vec3 result = (ambient + diffuse + specular) * texture(ourTexture, texCoord).rgb;\n"// * objColor;\n"
"   FragColor = vec4(result, 1.0);\n"// colorFromVS;\n //set our color to the one we got from the FS

"}\n\0";



//MAIN FUNCTION ============================================================================================================================

int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	UCreateCylinderWallMesh(gMesh, 12.0f);
	UCreateFlatCylinderWallMesh(flatCylinder, 24.0f);
	UCreatePlaneMesh(plane, 5.0f);
	UCreateRectMesh(rect);
	UCreateCubeMesh(cube);

	//BOTH BATTS TRANSFORM
	glm::mat4 bothBatts = glm::translate(glm::vec3(-1.5f, 0.0f, -0.5f));

	//BATTERY1 BODY TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translation = bothBatts * glm::translate(glm::vec3(-1.0f,  0.2f, 2.0f));
	glm::mat4 scale = glm::scale(glm::vec3(0.2f, 0.2f, 1.3f));
	glm::mat4 rotationInit = glm::rotate(75.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	//BATTERY2 BODY TRANSFORMATIONS
	glm::mat4 translationB2 = bothBatts * glm::translate(glm::vec3(-0.3f, 0.18f, 2.5f));
	glm::mat4 rotationYB2 = glm::rotate(glm::radians(-23.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 rotationXB2 = glm::rotate(glm::radians(270.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationInitB2 = rotationYB2 * rotationXB2;

	//BATTERY TERMINAL1 TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationT = glm::translate(glm::vec3(-0.48f, 0.0f, 1.15f));
	glm::mat4 scaleT = glm::scale(glm::vec3(0.1f, 0.1f, 0.09f));

	//BATTERY TERMINAL2 TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationT2 = translationB2 * glm::translate(glm::vec3(-1.15f, 0.48f, 0.0f));

	//PLANE TRANSFORMATION AND SCALE MATRICES
	glm::mat4 translationPl = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 scalePl = glm::scale(glm::vec3(1.0f, 1.0f, 1.0f));
	glm::mat4 rotationPl = glm::rotate(0.0f, glm::vec3(0.0f, 1.0f, 1.0f));


	//CHARGER ACTUAL TRANSFORM
	glm::mat4 chargerActual = glm::translate(glm::vec3(3.7, 0.0, 0.5));
	//CHARGER BODY TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationCB = chargerActual * glm::translate(glm::vec3(0.0f, 0.39f, 0.0f));
	glm::mat4 scaleCorrect = glm::scale(glm::vec3(0.5f, 0.5f, 1.0f));
	glm::mat4 scaleCB = scaleCorrect * glm::scale(glm::vec3(0.75f, 0.75f, 0.75f));
	glm::mat4 rotationCB = glm::rotate(12.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	//CHARGER PRONG1 TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationP1 = chargerActual * glm::translate(glm::vec3(0.0f, 1.0f, 0.38f));
	glm::mat4 scaleP1 = scaleCorrect * glm::scale(glm::vec3(0.05f, 0.5f, 0.2f));
	glm::mat4 rotationP1 = rotationCB;

	//CHARGER PRONG2 TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationP2 = chargerActual * glm::translate(glm::vec3(-0.33f, 1.0f, 0.16f));
	glm::mat4 scaleP2 = scaleCorrect * glm::scale(glm::vec3(0.05f, 0.5f, 0.2f));
	glm::mat4 rotationP2 = rotationCB;

	//CD TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationCD = glm::translate(glm::vec3(2.0f, 0.002f, 1.0f));
	glm::mat4 scaleCD = glm::scale(glm::vec3(2.0f, 2.0f, 0.01f));
	glm::mat4 rotationInitCD = glm::rotate(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));


	//SPEAKER TRANSFORMATION AND SCALE MATRICIES 
	glm::mat4 translationSP = glm::translate(glm::vec3(-0.5f, 1.0f, -1.5f));
	glm::mat4 scaleSP = scaleCorrect * glm::scale(glm::vec3(6.5f, 2.0f, 2.0f));
	glm::mat4 rotationSP = glm::rotate(glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));



	//camera stuff===========================================
	cameraPos = glm::vec3(0.0f, 2.0f, 9.0f);
	cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	//create textured shader program and store in gProgramId (if it fails, abort)
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
	{
		std::cout << std::endl << "ABORTING PROGRAM\n" << std::endl;
		exit(EXIT_SUCCESS);
	}

	//texturing stuff========================================
	{
		unsigned int groundTexture, batteryTexture, terminalTexture, bodyTexture, prongTexture, cdTexture, speakerTexture;
		// texture 1
		// ---------
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &groundTexture);
		glBindTexture(GL_TEXTURE_2D, groundTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load image, create texture and generate mipmaps
		int width, height, nrChannels;

		unsigned char *data = stbi_load("../Resources/tabletop.jpg", &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);
		// texture 2
		// ---------
		glActiveTexture(GL_TEXTURE1);
		glGenTextures(1, &batteryTexture);
		glBindTexture(GL_TEXTURE_2D, batteryTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// load image, create texture and generate mipmaps
		data = stbi_load("../Resources/battery.png", &width, &height, &nrChannels, 0);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);

		// texture 3
		// ---------
		glActiveTexture(GL_TEXTURE2);
		glGenTextures(1, &terminalTexture);
		glBindTexture(GL_TEXTURE_2D, terminalTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// load image, create texture and generate mipmaps
		data = stbi_load("../Resources/terminal.png", &width, &height, &nrChannels, 0);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);

		// texture 4
		// ---------
		glActiveTexture(GL_TEXTURE3);
		glGenTextures(1, &bodyTexture);
		glBindTexture(GL_TEXTURE_2D, bodyTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// load image, create texture and generate mipmaps
		data = stbi_load("../Resources/chargertop.png", &width, &height, &nrChannels, 0);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);

		// texture 5
		// ---------
		glActiveTexture(GL_TEXTURE4);
		glGenTextures(1, &prongTexture);
		glBindTexture(GL_TEXTURE_2D, prongTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		// load image, create texture and generate mipmaps
		data = stbi_load("../Resources/prongs.png", &width, &height, &nrChannels, 0);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);

		// texture 6
		// ---------
		glActiveTexture(GL_TEXTURE5);
		glGenTextures(1, &cdTexture);
		glBindTexture(GL_TEXTURE_2D, cdTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load image, create texture and generate mipmaps
		data = stbi_load("../Resources/cd.png", &width, &height, &nrChannels, 4);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);

		// texture 7
		// ---------
		glActiveTexture(GL_TEXTURE6);
		glGenTextures(1, &speakerTexture);
		glBindTexture(GL_TEXTURE_2D, speakerTexture);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// load image, create texture and generate mipmaps
		data = stbi_load("../Resources/speaker.png", &width, &height, &nrChannels, 4);
		if (data)
		{
			// note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}
		stbi_image_free(data);
	}


	// render loop
	// -----------	

	while (!glfwWindowShouldClose(gWindow))
	{
		glUseProgram(gProgramId);
		//LIGHTING STUFF===============================================================

		//light 1 color
		GLuint ambientLiLoc = glGetUniformLocation(gProgramId, "light.ambient");
		GLuint diffuseLiLoc = glGetUniformLocation(gProgramId, "light.diffuse");
		GLuint specularLiLoc = glGetUniformLocation(gProgramId, "light.specular");
		glm::vec3 lightColor;
		lightColor.x = 1.0f;//(sin(glfwGetTime() * 2.0f));
		lightColor.y = 1.0f;//(sin(glfwGetTime() * 0.7f));
		lightColor.z = 1.0f;//(sin(glfwGetTime() * 1.3f));
		glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f); // decrease the influence
		glm::vec3 ambientColor = diffuseColor * glm::vec3(1.0f); // low influence



		glUniform3fv(ambientLiLoc, 1, glm::value_ptr(ambientColor));
		glUniform3fv(diffuseLiLoc, 1, glm::value_ptr(diffuseColor));
		glUniform3fv(specularLiLoc, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 1.0f)));

		GLuint ambientLoc = glGetUniformLocation(gProgramId, "material.ambient");
		GLuint diffuseLoc = glGetUniformLocation(gProgramId, "material.diffuse");
		GLuint specularLoc = glGetUniformLocation(gProgramId, "material.specular");
		GLuint shininessLoc = glGetUniformLocation(gProgramId, "material.shininess");

		glm::vec3 ambientVal(1.0f, 1.0f, 1.0f);
		glm::vec3 diffuseVal(1.0f, 1.0f, 1.0f);
		glm::vec3 specularVal(1.0f, 1.0f, 1.0f);

		glUniform3fv(ambientLoc, 1, glm::value_ptr(ambientVal));
		glUniform3fv(diffuseLoc, 1, glm::value_ptr(diffuseVal));
		glUniform3fv(specularLoc, 1, glm::value_ptr(specularVal));
		glUniform1f(shininessLoc, 999.0f);

		GLuint objColorLoc = glGetUniformLocation(gProgramId, "objColor");
		GLuint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
		//values
		glm::vec3 objectColor(1.0f, 1.0f, 1.0f);

		//setting uniforms
		glUniform3fv(objColorLoc, 1, glm::value_ptr(objectColor));
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		//set light position uniform
		GLuint lightPosLoc = glGetUniformLocation(gProgramId, "lightPos");
		GLuint lightPos2Loc = glGetUniformLocation(gProgramId, "lightPos2");
		glm::vec3 lightPos(3.0f, 3.0f, 3.0f);
		glm::vec3 lightPos2(-3.0f, 3.0f, -3.0f);
		glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
		glUniform3fv(lightPos2Loc, 1, glm::value_ptr(lightPos2));

		//set light direction uniform
		GLuint lightDirectionLoc = glGetUniformLocation(gProgramId, "light.direction");
		glUniform3fv(lightDirectionLoc, 1, glm::value_ptr(glm::vec3(-5.2f, -1.0f, -0.3f)));
		//set light direction2 uniform
		GLuint lightDirection2Loc = glGetUniformLocation(gProgramId, "light.direction2");
		glUniform3fv(lightDirection2Loc, 1, glm::value_ptr(glm::vec3(5.2f, -1.0f, 0.3f)));

		//==============================================================================

		//clear buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//init view matrix for camera 
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		//get input (camera movement)
		cameraPos = UProcessInput(gWindow);
		GLuint cameraPosLoc = glGetUniformLocation(gProgramId, "cameraPos");
		glUniform3fv(cameraPosLoc, 1, glm::value_ptr(cameraPos));

		//get delta time (currently unused)
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
//======TABLETOP========================================================================================
		//set shader program and texutre for ground
		glUseProgram(gProgramId);
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 0);

		//create plane
		URenderMesh(plane, translationPl, rotationPl ,scalePl, view);

//======BATTERY 1========================================================================================
		//set texture for battery 
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 1);
		//set shininess for battery
		glUniform1f(shininessLoc, 12.0f);

		//render the body mesh
		GLfloat rotationFactor = 0;
		for (int i = 0; i < 12; i++)
		{
			glm::mat4 rotation = rotationInit * glm::rotate(glm::radians(rotationFactor), glm::vec3(0.0f, 0.0f, 1.0f));
			URenderMesh(gMesh, translation, rotation, scale, view);
			rotationFactor += 30.0f;
		}

		//set shininess for terminal
		glUniform1f(shininessLoc, 2.0f);

		//set texture for terminal
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 2);

		rotationFactor = 0;
		//render the terminal mesh
		for (int i = 0; i < 12; i++)
		{
			glm::mat4 rotation = rotationInit * glm::rotate(glm::radians(rotationFactor), glm::vec3(0.0f, 0.0f, 1.0f));
			URenderMesh(gMesh, translationT * translation, rotation, scaleT, view);
			rotationFactor += 30.0f;
		}

//======BATTERY 2========================================================================================
		//set texture for battery 
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 1);
		//set shininess for battery
		glUniform1f(shininessLoc, 12.0f);

		//render the body mesh
		rotationFactor = 0;
		for (int i = 0; i < 12; i++)
		{
			glm::mat4 rotation = rotationInitB2 * glm::rotate(glm::radians(rotationFactor), glm::vec3(0.0f, 0.0f, 1.0f));
			URenderMesh(gMesh, translationB2, rotation, scale, view);
			rotationFactor += 30.0f;
		}

		//set shininess for terminal
		glUniform1f(shininessLoc, 2.0f);

		//set texture for terminal
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 2);

		rotationFactor = 0;
		//render the terminal mesh
		for (int i = 0; i < 12; i++)
		{
			glm::mat4 rotation = rotationInitB2 * glm::rotate(glm::radians(rotationFactor), glm::vec3(0.0f, 0.0f, 1.0f));
			URenderMesh(gMesh, translationT2, rotation, scaleT, view);
			rotationFactor += 30.0f;
		}

//======CHARGER BODY========================================================================================
		//set texture
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 3);
		URenderMesh(cube, translationCB, rotationCB, scaleCB, view);

//======CHARGER PRONG1========================================================================================
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 4);
		glUniform1f(shininessLoc, 1.0f);
		URenderMesh(cube, translationP1, rotationP1, scaleP1, view);

//======CHARGER PRONG2========================================================================================
		URenderMesh(cube, translationP2, rotationP2, scaleP2, view);

//======CD ===================================================================================================
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 5);
		//render the cd mesh/*
		for (int i = 0; i < 24; i++)
		{
			glm::mat4 rotationCD = rotationInitCD * glm::rotate(glm::radians(rotationFactor), glm::vec3(0.0f, 0.0f, 1.0f));
			URenderMesh(flatCylinder, translationCD, rotationCD, scaleCD, view);
			rotationFactor += 15.0f;
		}

//======SPEAKER ==============================================================================================
		glUniform1i(glGetUniformLocation(gProgramId, "ourTexture"), 6);
		URenderMesh(rect, translationSP, rotationSP, scaleSP, view);
		diffuseColor = lightColor * glm::vec3(0.002f); // decrease the influence
		ambientColor = diffuseColor * glm::vec3(0.001f); // low influence

		glUniform3fv(ambientLiLoc, 1, glm::value_ptr(ambientColor));
		glUniform3fv(diffuseLiLoc, 1, glm::value_ptr(diffuseColor));


		// glfw: Swap buffers and poll IO events (keys pressed/released, mouse moved, and so on).
		glfwPollEvents();
		glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
	}


	//destroy meshes and shader to clean up
	UDestroyMesh(gMesh);
	UDestroyMesh(cube);
	UDestroyMesh(plane);
	UDestroyMesh(flatCylinder);
	UDestroyMesh(rect);
	UDestroyShaderProgram(gProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}

//INITIALIZE FUNCTION (standard) ==================================================================================================================

bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure (specify desired OpenGL version)
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif


	// GLFW: window creation
	// ---------------------
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);

	//enable cursor and scroll capture and set callback functions
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(*window, mouse_callback);
	glfwSetScrollCallback(*window, scroll_callback);
	glfwSetKeyCallback(*window, p_callback);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}

//PROCESS INPUT FUNCTION (standard)================================================================================================================

glm::vec3 UProcessInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//camera movement-------------------------------
	//forward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	//backward
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;
	//strafe left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	//strafe right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
	//fly down
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraUp;
	//fly up
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraUp;

	//return cameraPos for use
	return cameraPos;

}

//RESIZE FUNCTION (standard) ======================================================================================================================

void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

//CREATE CYLINDER WALL ============================================================================================================================

void UCreateCylinderWallMesh(GLMesh &mesh, float numSections)
{
	float sectionAngle((360 / numSections) / 2);

	float vertices[] =
	{
		//POS====================================================================        COLOR                             TEXTURE COORDS			NORMALS

		 // TOP TRIANGLE
		 0.0,   0.0f, 0.0f,																 1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.0f,		0.0f, 0.0f, -1.0f,// origin
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 1.0f,  0.1f,		0.0f, 0.0f, -1.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 0.1f,  0.1f,		0.0f, 0.0f, -1.0f,// bottom corner

		 // BOTTOM TRIANGLE
		 0.0,   0.0f, 1.0f,																 1.0f, 1.0f, 1.0f, 1.0f,			 -1.5f,  -1.5f,		0.0f, 0.0f, 1.0f,// origin					
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			  0.9f,   0.9f,		0.0f, 0.0f, 1.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			  0.0f,   0.9f,		0.0f, 0.0f, 1.0f,// bottom corner

		 // BOTTOM TRIANGLE	
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			 0.9f,  0.9f,		1.0f, 0.0f, 0.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.9f,		1.0f, 0.0f, 0.0f,// bottom corner
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 1.0f,  0.1f,		1.0f, 0.0f, 0.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 0.1f,  0.1f,		1.0f, 0.0f, 0.0f,// bottom corner




	};

	// Index data to share position data
	short indices[] = {
	
		0, 1, 2, // TOP 
		3, 4, 5, // BOTTOM
		6, 7, 8,
		9, 8, 7,

	};

	glGenVertexArrays(1, &mesh.vao);			//init vao
	glBindVertexArray(mesh.vao);				//bind vertex array to the vao

	//generate buffers for vertex and index info
	glGenBuffers(2, mesh.vbos);					//init buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);//bind the vertex info to the 0th vbo
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //send vertex data to gpu (VBO)

	//indexing vertex attributes so VBO knows what the hell is going on with it's data

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//how many floats per vertex and color? (3 for 3 3d coordinates) (4 for RGB and alpha values)
	const GLuint FLOATS_PER_VERTEX = 3;
	const GLuint FLOATS_PER_COLOR = 4;
	const GLuint FLOATS_PER_TEXCORD = 2;
	const GLuint FLOATS_PER_NORMAL = 3;

	//indicate stride between vertex info (slice point for each individual vertex)
	GLint stride = sizeof(float) * (FLOATS_PER_VERTEX + FLOATS_PER_COLOR + FLOATS_PER_TEXCORD + FLOATS_PER_NORMAL);

	//tell GPU how to handle VBO info
	glVertexAttribPointer(0, FLOATS_PER_VERTEX, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0); //initial coordinate position (it's just the first position in our simple array)

	glVertexAttribPointer(1, FLOATS_PER_COLOR, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, FLOATS_PER_TEXCORD, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, FLOATS_PER_NORMAL, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

}

//CREATE FLAT CYLINDER WALL ============================================================================================================================

void UCreateFlatCylinderWallMesh(GLMesh &mesh, float numSections)
{
	float sectionAngle((360 / numSections) / 2);

	float vertices[] =
	{
		//POS====================================================================        COLOR                             TEXTURE COORDS			NORMALS

		 // TOP TRIANGLE
		 0.0,   0.0f, 0.0f,																 1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.0f,		0.0f, 0.0f, -1.0f,// origin
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 0.0f,  1.0f,		0.0f, 0.0f, -1.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 1.0f,  1.0f,		0.0f, 0.0f, -1.0f,// bottom corner

		 // BOTTOM TRIANGLE
		 0.0,   0.0f, 1.0f,																 1.0f, 1.0f, 1.0f, 1.0f,			  0.0f,   0.0f,		0.0f, 0.0f, 1.0f,// origin					
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			  1.0f,   1.0f,		0.0f, 0.0f, 1.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			  0.5f,   1.0f,		0.0f, 0.0f, 1.0f,// bottom corner

		 // BOTTOM TRIANGLE	
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.0f,		1.0f, 0.0f, 0.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 1.0f,		 1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.0f,		1.0f, 0.0f, 0.0f,// bottom corner
		 cos(glm::radians(sectionAngle)),  sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 0.0f,  0.0f,		1.0f, 0.0f, 0.0f,// top corner
		 cos(glm::radians(sectionAngle)), -sin(glm::radians(sectionAngle)), 0.0f,		 1.0f, 1.0f, 1.0f, 1.0f,		 	 0.0f,  0.0f,		1.0f, 0.0f, 0.0f,// bottom corner




	};

	// Index data to share position data
	short indices[] = {

		0, 1, 2, // TOP 
		3, 4, 5, // BOTTOM
		6, 7, 8,
		9, 8, 7,

	};

	glGenVertexArrays(1, &mesh.vao);			//init vao
	glBindVertexArray(mesh.vao);				//bind vertex array to the vao

	//generate buffers for vertex and index info
	glGenBuffers(2, mesh.vbos);					//init buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);//bind the vertex info to the 0th vbo
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //send vertex data to gpu (VBO)

	//indexing vertex attributes so VBO knows what the hell is going on with it's data

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//how many floats per vertex and color? (3 for 3 3d coordinates) (4 for RGB and alpha values)
	const GLuint FLOATS_PER_VERTEX = 3;
	const GLuint FLOATS_PER_COLOR = 4;
	const GLuint FLOATS_PER_TEXCORD = 2;
	const GLuint FLOATS_PER_NORMAL = 3;

	//indicate stride between vertex info (slice point for each individual vertex)
	GLint stride = sizeof(float) * (FLOATS_PER_VERTEX + FLOATS_PER_COLOR + FLOATS_PER_TEXCORD + FLOATS_PER_NORMAL);

	//tell GPU how to handle VBO info
	glVertexAttribPointer(0, FLOATS_PER_VERTEX, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0); //initial coordinate position (it's just the first position in our simple array)

	glVertexAttribPointer(1, FLOATS_PER_COLOR, GL_FLOAT, GL_FALSE, stride, (void*)(4 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, FLOATS_PER_TEXCORD, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, FLOATS_PER_NORMAL, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

}

//CREATE PLANE FUNCTION (z = 0)====================================================================================================================

void UCreatePlaneMesh(GLMesh &mesh, float scale)
{
	float vertices[] =
	{
		//POS						//COLOR							//TEXTURE COORDS				//NORMALS
		-scale,  0.0f,  scale,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, 1.0f, 0.0f,// TL
		 scale,  0.0f,  scale,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  1.0f,				0.0f, 1.0f, 0.0f,// TR
		 scale,  0.0f, -scale,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  0.0f,				0.0f, 1.0f, 0.0f,// BR
		-scale,  0.0f, -scale,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.0f,				0.0f, 1.0f, 0.0f,// BL
	};

	// Index data to share position data
	short indices[] = {

		0, 1, 2,
		0, 3, 2 
	};

	glGenVertexArrays(1, &mesh.vao);			//init vao
	glBindVertexArray(mesh.vao);				//bind vertex array to the vao

	//generate buffers for vertex and index info
	glGenBuffers(2, mesh.vbos);					//init buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);//bind the vertex info to the 0th vbo
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //send vertex data to gpu (VBO)

	//indexing vertex attributes so VBO knows what the hell is going on with it's data

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//how many floats per vertex and color? (3 for 3 3d coordinates) (4 for RGB and alpha values)
	const GLuint FLOATS_PER_VERTEX = 3;
	const GLuint FLOATS_PER_COLOR = 4;
	const GLuint FLOATS_PER_TEXCORD = 2;
	const GLuint FLOATS_PER_NORMAL = 3;

	//indicate stride between vertex info (slice point for each individual vertex)
	GLint stride = sizeof(float) * (FLOATS_PER_VERTEX + FLOATS_PER_COLOR + FLOATS_PER_TEXCORD + FLOATS_PER_NORMAL);

	//tell GPU how to handle VBO info
	glVertexAttribPointer(0, FLOATS_PER_VERTEX, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0); //initial coordinate position (it's just the first position in our simple array)

	glVertexAttribPointer(1, FLOATS_PER_COLOR, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, FLOATS_PER_TEXCORD, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, FLOATS_PER_NORMAL, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

}

//CREATE RECT FUNCTION ============================================================================================================================
void UCreateRectMesh(GLMesh &mesh)
{
	float vertices[] =
	{
			   //POS					   //COLOR				  //TEXTURE COORDS				       //NORMALS
		//TOP
		-1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, 0.0f, -1.0f,// TL 0
		 1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  0.5f,				0.0f, 0.0f, -1.0f,// TR 1
		 1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  1.0f,				0.0f, 0.0f, -1.0f,// BR 2
		-1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, 0.0f, -1.0f,// BL 3
		//BOTTOM
		-1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, 0.0f, 1.0f,// TL 4
		 1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  0.5f,				0.0f, 0.0f, 1.0f,// TR 5
		 1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  1.0f,				0.0f, 0.0f, 1.0f,// BR 6
		-1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, 0.0f, 1.0f,// BL 7
		//TOP WALL
		-1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.0f,				0.0f, 1.0f, 0.0f,// TL 8
		 1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  0.0f,				0.0f, 1.0f, 0.0f,// TR 9
		 1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  0.5f,				0.0f, 1.0f, 0.0f,// BR 10
		-1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, 1.0f, 0.0f,// BL 11
		//RIGHT WALL
		 1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,				1.0f, 0.0f, 0.0f,// TL 12
		 1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,				1.0f, 0.0f, 0.0f,// TR 13
		 1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				1.0f, 0.0f, 0.0f,// BR 14
		 1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				1.0f, 0.0f, 0.0f,// BL 15
		//BOTTOM WALL
		-1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, -1.0f, 0.0f,// TL 16
		 1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  0.5f,				0.0f, -1.0f, 0.0f,// TR 17
		 1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 1.0f,  1.0f,				0.0f, -1.0f, 0.0f,// BR 18
		-1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, -1.0f, 0.0f,// BL 19
		//LEFT WALL
		-1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,			   -1.0f, 0.0f, 0.0f,// TL 20
		-1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,			   -1.0f, 0.0f, 0.0f,// TR 21
		-1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,			   -1.0f, 0.0f, 0.0f,// BR 22
		-1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,			   -1.0f, 0.0f, 0.0f,// BL 23
	};

	// Index data to share position data
	short indices[] = {
		//top
		0, 1, 2,
		0, 2, 3,
		//bottom
		4, 5, 6,
		4, 6, 7,
		//top wall
		8, 9, 10,
		8, 11, 10,
		//right wall
		12, 13, 14,
		12, 15, 14,
		//bottom wall
		16, 17, 18,
		16, 19, 18,
		//left wall
		20, 21, 22, 
		20, 23, 22,
	};

	glGenVertexArrays(1, &mesh.vao);			//init vao
	glBindVertexArray(mesh.vao);				//bind vertex array to the vao

	//generate buffers for vertex and index info
	glGenBuffers(2, mesh.vbos);					//init buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);//bind the vertex info to the 0th vbo
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //send vertex data to gpu (VBO)

	//indexing vertex attributes so VBO knows what the hell is going on with it's data

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//how many floats per vertex and color? (3 for 3 3d coordinates) (4 for RGB and alpha values)
	const GLuint FLOATS_PER_VERTEX = 3;
	const GLuint FLOATS_PER_COLOR = 4;
	const GLuint FLOATS_PER_TEXCORD = 2;
	const GLuint FLOATS_PER_NORMAL = 3;

	//indicate stride between vertex info (slice point for each individual vertex)
	GLint stride = sizeof(float) * (FLOATS_PER_VERTEX + FLOATS_PER_COLOR + FLOATS_PER_TEXCORD + FLOATS_PER_NORMAL);

	//tell GPU how to handle VBO info
	glVertexAttribPointer(0, FLOATS_PER_VERTEX, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0); //initial coordinate position (it's just the first position in our simple array)

	glVertexAttribPointer(1, FLOATS_PER_COLOR, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, FLOATS_PER_TEXCORD, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, FLOATS_PER_NORMAL, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

}

//CREATE CUBE MESH==============================================================================================================
void UCreateCubeMesh(GLMesh &mesh)
{
	float vertices[] =
	{
		//POS					   //COLOR				  //TEXTURE COORDS				       //NORMALS
 //TOP
 -1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, 0.0f, -1.0f,// TL 0
  1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,				0.0f, 0.0f, -1.0f,// TR 1
  1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,				0.0f, 0.0f, -1.0f,// BR 2
 -1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, 0.0f, -1.0f,// BL 3
 //BOTTOM
 -1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, 0.0f, 1.0f,// TL 4
  1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,				0.0f, 0.0f, 1.0f,// TR 5
  1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,				0.0f, 0.0f, 1.0f,// BR 6
 -1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, 0.0f, 1.0f,// BL 7
 //TOP WALL
 -1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.0f,				0.0f, 1.0f, 0.0f,// TL 8
  1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.0f,				0.0f, 1.0f, 0.0f,// TR 9
  1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,				0.0f, 1.0f, 0.0f,// BR 10
 -1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, 1.0f, 0.0f,// BL 11
 //RIGHT WALL
  1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				1.0f, 0.0f, 0.0f,// TL 12
  1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,				1.0f, 0.0f, 0.0f,// TR 13
  1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,				1.0f, 0.0f, 0.0f,// BR 14
  1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				1.0f, 0.0f, 0.0f,// BL 15
 //BOTTOM WALL
 -1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,				0.0f, -1.0f, 0.0f,// TL 16
  1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,				0.0f, -1.0f, 0.0f,// TR 17
  1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,				0.0f, -1.0f, 0.0f,// BR 18
 -1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,				0.0f, -1.0f, 0.0f,// BL 19
 //LEFT WALL
 -1.0f,  1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  1.0f,			   -1.0f, 0.0f, 0.0f,// TL 20
 -1.0f, -1.0f,  1.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  1.0f,			   -1.0f, 0.0f, 0.0f,// TR 21
 -1.0f, -1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.5f,  0.5f,			   -1.0f, 0.0f, 0.0f,// BR 22
 -1.0f,  1.0f,  0.0f,		1.0f, 1.0f, 1.0f, 1.0f,			 0.0f,  0.5f,			   -1.0f, 0.0f, 0.0f,// BL 23
	};

	// Index data to share position data
	short indices[] = {
		//top
		0, 1, 2,
		0, 2, 3,
		//bottom
		4, 5, 6,
		4, 6, 7,
		//top wall
		8, 9, 10,
		8, 11, 10,
		//right wall
		12, 13, 14,
		12, 15, 14,
		//bottom wall
		16, 17, 18,
		16, 19, 18,
		//left wall
		20, 21, 22,
		20, 23, 22,
	};

	glGenVertexArrays(1, &mesh.vao);			//init vao
	glBindVertexArray(mesh.vao);				//bind vertex array to the vao

	//generate buffers for vertex and index info
	glGenBuffers(2, mesh.vbos);					//init buffer
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]);//bind the vertex info to the 0th vbo
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //send vertex data to gpu (VBO)

	//indexing vertex attributes so VBO knows what the hell is going on with it's data

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//how many floats per vertex and color? (3 for 3 3d coordinates) (4 for RGB and alpha values)
	const GLuint FLOATS_PER_VERTEX = 3;
	const GLuint FLOATS_PER_COLOR = 4;
	const GLuint FLOATS_PER_TEXCORD = 2;
	const GLuint FLOATS_PER_NORMAL = 3;

	//indicate stride between vertex info (slice point for each individual vertex)
	GLint stride = sizeof(float) * (FLOATS_PER_VERTEX + FLOATS_PER_COLOR + FLOATS_PER_TEXCORD + FLOATS_PER_NORMAL);

	//tell GPU how to handle VBO info
	glVertexAttribPointer(0, FLOATS_PER_VERTEX, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0); //initial coordinate position (it's just the first position in our simple array)

	glVertexAttribPointer(1, FLOATS_PER_COLOR, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, FLOATS_PER_TEXCORD, GL_FLOAT, GL_FALSE, stride, (void*)(7 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, FLOATS_PER_NORMAL, GL_FLOAT, GL_FALSE, stride, (void*)(9 * sizeof(float)));
	glEnableVertexAttribArray(3);

}

//DESTROY MESH FUNCTION ============================================================================================================================
void UDestroyMesh(GLMesh &mesh)
{
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, mesh.vbos);
}

//CREATE SHADER PROGRAM FUNCTION ===================================================================================================================
bool UCreateShaderProgram(const char* vertexShaderSource, const char* fragShaderSource, GLuint &programId)
{
	//error handling vars
	int success = 0;
	char infoLog[512];

	//create the program and point to programId
	programId = glCreateProgram();

	//create shader ids
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	//get code for shaders and store in shader ids
	glShaderSource(vertexShaderId, 1, &vertexShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	//compile the vertex shader
	glCompileShader(vertexShaderId);
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		//print errors
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR COMPILING VERTEX SHADER\n" << infoLog << std::endl;

		return false;
	}

	//compile the fragment shader
	glCompileShader(fragmentShaderId);
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR COMPILING FRAGMENT SHADER\n" << infoLog << std::endl;
	
		return false;
	}

	//attach the shaders to the program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	//link for use
	glLinkProgram(programId);
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR LINKING SHADER PROGRAM\n" << infoLog << std::endl;

		return false;
	}

	return true;
}

//DESTROY SHADER PROGRAM FUNCTION ===================================================================================================================

void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}

//RENDER FUNCTION ===================================================================================================================================

void URenderMesh(const GLMesh& mesh, glm::mat4 translation, glm::mat4 rotation, glm::mat4 scale, glm::mat4 view)
{
	//enable z-depth
	glEnable(GL_DEPTH_TEST);

	//right to left order for transformations (ORDER MATTERS!)
	glm::mat4 model = translation * rotation * scale;
	//^^^ THIS CREATES A TRANSFORMATION MATRIX WE CAN APPLY TO THE MESH USING THE DATA FROM EACH PRECEEDING MATRIX

	//initialize projection matrix
	glm::mat4 projection;

	//create projection matrix
	if (ortho == false)
	{
		projection = glm::perspective(glm::radians(45.0f), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f); //perspective projection
	}
	else
	{
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f); //orthogonol projection
	}

	//setting up MVP
	GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//WIREFRAME MODE
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	//bind the vao so we dont have to send things to the vbo
	glBindVertexArray(mesh.vao);
	//draw the triangles
	glDrawElements(GL_TRIANGLES, mesh.nIndices, GL_UNSIGNED_SHORT, NULL);

	//"unbind" vao
	glBindVertexArray(0);

}

//MOUSE CALLBACK ====================================================================================================================================

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	//check if it's the first time we get mouse focus (prevents jolt of first mouse move)
	if (firstMouse) 
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	
	//detect change in mouse position and set
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
	lastX = xpos;
	lastY = ypos;
	//define mouse movement sensitivity and apply to change in position
	const float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	//change pitch and yaw depending on new position
	yaw += xoffset;
	pitch += yoffset;

	//restrict camera from rotating past straight up (90) or straight down (-90)
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	//set direction and apply to cameraFront (used in setting view)
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}

//SCROLL CALLBACK ====================================================================================================================================

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//NOTE: I decided not to normalize movement speed based on delta time
	//since you're able to change the speed anyway.

	cameraSpeed = glm::abs(cameraSpeed + (yoffset * 0.01));
	//define minimum speed
	if (cameraSpeed <= 0.01f)
	{
		cameraSpeed = 0.01f;
	}

	//log speed to the console (used for testing)
	cout << "CAM SPEED: " << cameraSpeed << endl;
}

//P KEY CALLBACK =====================================================================================================================================

void p_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		ortho = !ortho;
	}
}

