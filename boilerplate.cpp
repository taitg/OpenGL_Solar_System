// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <iterator>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

// Specify that we want the OpenGL core profile before including GLFW headers
#ifndef LAB_LINUX
#include <glad/glad.h>
#else
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#endif
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;
using namespace glm;

// --------------------------------------------------------------------------
// constants and global vars

const float piVal = 3.14159265359;
const int maxShapes = 65536;
const int wWidth = 1920;
const int wHeight = 1080;
const bool showWireframe = true;
const bool antialiasing = true;

// field of view
float fovDegrees = 38.0;
float fov = fovDegrees * piVal / 180.0;

// camera
int camFocus = 0;
float cameraP = piVal / 2.0;
float cameraT = piVal / 2.0;
float cameraR = 50.0;
float scrollSpeed = 0.01;
float maxHeight = piVal * 0.99;
float minHeight = piVal - maxHeight;
float zoomSpeed = 2.0;
float maxDistance = 500.0;
float minDistance = 5.0;

// animation
bool animate = true;
double animSpeed = 4.0;
double minSpeed = 0.1;
double maxSpeed = 500.0;
double speedInterval = 1.2;
double lastFrameTime;

// scaling values
double unit = 1111.1; // km
double factor = 0.5;
double base = 2.0;

// star values
char starTexture[] = "stars.png";
float starResolution = 40.0; // #of divisions per polar coordinate

// earth values
char earthTexture[] = "earth.png";
char cloud1Texture[] = "clouds1.png";
char cloud2Texture[] = "clouds2.png";
float cloudIntensity = 1.0;
float earthResolution = 40.0; // #of divisions per polar coordinate
double distEarthToSun = log(149597890.0 / unit) / log(base); // km
double earthR = factor * log(6378.1 / unit) / log(base); // km
float earthRotate = 0.99726968; // days
float earthOrbit = 365.25; // days
float earthTilt = 23.44 * piVal / 180.0; // degrees -> radians

// moon values
char moonTexture[] = "moon.png";
float moonResolution = 20.0; // #of divisions per polar coordinate
double distMoonToEarth = log(384403.08 / unit) / log(base); // km
double moonR = factor * log(1737.1 / unit) / log(base); // km
float moonOrbit = 27.32158; // days
float moonTilt = 6.68 * piVal / 180.0; // degrees -> radians
float moonIncl = 23.435 * piVal / 180.0; // degrees -> radians

// sun values
char sunTexture[] = "sun.png";
float sunResolution = 100.0; // #of divisions per polar coordinate
double sunR = factor * log(695700.0 / unit) / log(base); // km
float sunRotate = 25.38; // days
float sunTilt = 7.25 * piVal / 180.0; // degrees -> radians
float light[] = { 0.0, 0.0, 0.0 }; // x,y,z
float ambient = 0.15;		// ambient intensity
float diffRatio = 1.0;		// diffuse lighting ratio
float intensity = 1.0;		// light intensity
float glowIntensity = 0.25;	// sun glow effect intensity
float phong = 2.0;			// normal phong exponent
float waterPhong = 48.0;	// phong exponent for water
float specColour[] = { 0.1, 0.1, 0.1 };	// specular colour

// model rotation
float xangle = piVal / 2.0;
float yangle = 0.0;

// mouse
double mousex, mousey;
bool rotating = false;

// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

struct MyTexture
{
	GLuint textureID;
	GLuint target;
	int width;
	int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  elementBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

struct MySphere {
	int lastVertex;
	int lastIndex;
};

// arrays
GLfloat vertices[maxShapes * 3][3];
GLfloat colours[maxShapes * 3][3];
GLfloat texCoords[maxShapes * 3][3];
unsigned indices[maxShapes * 3];
MyTexture textures[6];
MySphere spheres[4];

// creates a sphere using triangles
MySphere generateSphere(float div, float R, int i, int j, int texID) {
	MySphere result;

	float unit = piVal / div;
	float x = 0.0, y = 0.0;

	for (float it = 0.0; it < piVal; it += unit) {
		x = 0.0;

		for (float ip = 0.0; ip < 2.0 * piVal; ip += unit) {
			float nextp = ip + unit;
			float nextt = it + unit;
			float px, py, pz;

			// p1
			px = R * cos(ip) * sin(it);
			py = R * sin(ip) * sin(it);
			pz = R * cos(it);
			vertices[i][0] = px;
			vertices[i][1] = py;
			vertices[i][2] = pz;
			texCoords[i][0] = 1.0 - x;
			texCoords[i][1] = y;
			texCoords[i][2] = texID;
			i++;
			x += 1.0 / (2.0 * div);

			// p2
			px = R * cos(nextp) * sin(it);
			py = R * sin(nextp) * sin(it);
			pz = R * cos(it);
			vertices[i][0] = px;
			vertices[i][1] = py;
			vertices[i][2] = pz;
			texCoords[i][0] = 1.0 - x;
			texCoords[i][1] = y;
			texCoords[i][2] = texID;
			i++;

			// p3
			px = R * cos(ip) * sin(nextt);
			py = R * sin(ip) * sin(nextt);
			pz = R * cos(nextt);
			vertices[i][0] = px;
			vertices[i][1] = py;
			vertices[i][2] = pz;
			texCoords[i][0] = 1.0 - (x - 1.0 / (2.0 * div));
			texCoords[i][1] = y + 1.0 / div;
			texCoords[i][2] = texID;
			i++;

			int offset = 0;
			if (ip > 2.0 * piVal - unit) {
				// p4
				px = R * cos(nextp) * sin(nextt);
				py = R * sin(nextp) * sin(nextt);
				pz = R * cos(nextt);
				vertices[i][0] = px;
				vertices[i][1] = py;
				vertices[i][2] = pz;
				texCoords[i][0] = 1.0 - x;
				texCoords[i][1] = y + 1.0 / div;
				texCoords[i][2] = texID;
				i++;
				offset = 1;
			}

			if (i > maxShapes * 3) break;

			// indices
			indices[j] = i - 1 - offset;
			j++;
			indices[j] = i - 2 - offset;
			j++;
			indices[j] = i - 3 - offset;
			j++;

			if (it >= 0.0 && it < piVal - unit) {
				indices[j] = i - 1 - offset;
				j++;
				indices[j] = i - 2 - offset;
				j++;

				if (ip < 2.0 * piVal - unit) {
					indices[j] = i + 2 - offset;
					j++;
				}
				else {
					indices[j] = i - offset;
					j++;
				}
			}
		}

		y += 1.0 / div;
	}

	result.lastVertex = i - 1;
	result.lastIndex = j - 1;
	return result;
}

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry)
{
	// earth
	spheres[0] = generateSphere(earthResolution, earthR, 0, 0, 0);

	// stars
	spheres[1] = generateSphere(starResolution, maxDistance + 0.65, spheres[0].lastVertex + 1, spheres[0].lastIndex + 1, 1);

	// moon
	spheres[2] = generateSphere(moonResolution, moonR, spheres[1].lastVertex + 1, spheres[1].lastIndex + 1, 2);

	// sun
	spheres[3] = generateSphere(sunResolution, sunR, spheres[2].lastVertex + 1, spheres[2].lastIndex + 1, 3);

	geometry->elementCount = spheres[3].lastIndex;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;
	const GLuint TEXTURE_INDEX = 2;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create another one for storing our texture data
	glGenBuffers(1, &geometry->textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// element array -> buffer
	glGenBuffers(1, &geometry->elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// texture array -> buffer
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(TEXTURE_INDEX, 3	, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(TEXTURE_INDEX);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
	glDeleteBuffers(1, &geometry->elementBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyShader *shader, MyTexture *textures)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);

	// bind textures
	for (int i = 0; i < 6; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(textures[i].target, textures[i].textureID);
	}

	// glDrawElements instead of glDrawArrays
	glDrawElements(GL_TRIANGLES, geometry->elementCount, GL_UNSIGNED_INT, 0);

	// reset state to default (no shader or geometry bound)
	glBindTexture(textures[0].target, 0);
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// close window
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	// set focus to sun
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		camFocus = 0;

	// set focus to earth
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		camFocus = 1;

	// set focus to moon
	if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		camFocus = 2;

	// toggle animation
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		animate = !animate;

	// increase speed
	if (key == GLFW_KEY_UP)
		if (animSpeed < maxSpeed) animSpeed *= speedInterval;

	// decrease speed
	if (key == GLFW_KEY_DOWN)
		if (animSpeed > minSpeed) animSpeed /= speedInterval;
}

// handles mouse button events
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_1) {
		// when pressed, store mouse location
		if (action == GLFW_PRESS) {
			glfwGetCursorPos(window, &mousex, &mousey);
			rotating = true;
		}

		else if (action == GLFW_RELEASE) 
			rotating = false;
	}
}

// handles cursor moving events
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
	if (rotating) {
		// adjust azimuth
		cameraP += scrollSpeed * double(xpos - mousex) / double(wWidth);

		// adjust altitude
		cameraT -= scrollSpeed * double(ypos - mousey) / double(wHeight);
		if (cameraT > maxHeight) cameraT = maxHeight;
		if (cameraT < minHeight) cameraT = minHeight;
	}
}

// handles mousewheel events
void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	// adjust zoom level
	cameraR -= zoomSpeed * yOffset;
	if (cameraR < minDistance) cameraR = minDistance;
	if (cameraR > maxDistance) cameraR = maxDistance;
}

// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// set window values
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// toggle antialiasing
	if (antialiasing) glfwWindowHint(GLFW_SAMPLES, 4);

	// attempt to create a window with an OpenGL 4.1 core profile context
	window = glfwCreateWindow(wWidth, wHeight, "CPSC 453 Assignment 5", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set callback functions and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetCursorPosCallback(window, CursorPosCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwMakeContextCurrent(window);

	//Intialize GLAD
#ifndef LAB_LINUX
	if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}
#endif

	// toggle wireframe only
	if (showWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	MyShader shader;
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	// initialize textures
	if (!InitializeTexture(&textures[0], earthTexture, GL_TEXTURE_2D))
		cout << "Program failed to intialize texture!" << endl;
	if (!InitializeTexture(&textures[1], starTexture, GL_TEXTURE_2D))
		cout << "Program failed to intialize texture!" << endl;
	if (!InitializeTexture(&textures[2], moonTexture, GL_TEXTURE_2D))
		cout << "Program failed to intialize texture!" << endl;
	if (!InitializeTexture(&textures[3], sunTexture, GL_TEXTURE_2D))
		cout << "Program failed to intialize texture!" << endl;
	if (!InitializeTexture(&textures[4], cloud1Texture, GL_TEXTURE_2D))
		cout << "Program failed to intialize texture!" << endl;
	if (!InitializeTexture(&textures[5], cloud2Texture, GL_TEXTURE_2D))
		cout << "Program failed to intialize texture!" << endl;

	// call function to create and fill buffers with geometry data
	MyGeometry geometry;
	if (!InitializeGeometry(&geometry))
		cout << "Program failed to intialize geometry!" << endl;

	lastFrameTime = glfwGetTime();
	float aspectRatio = (float)wWidth / (float)wHeight;
	float zNear = .1f, zFar = 1000.f;
	mat4 I(1);

	// axes and translation vectors
	vec3 xaxis = vec3(1, 0, 0);
	vec3 yaxis = vec3(0, 1, 0);
	vec3 zaxis = vec3(0, 0, 1);
	vec3 earthLoc(distEarthToSun, 0, 0);
	vec3 moonLoc(distMoonToEarth, 0, 0);

	// get uniform locations
	glUseProgram(shader.program);
	GLint starsUniform = glGetUniformLocation(shader.program, "starsModel");
	GLint sunUniform = glGetUniformLocation(shader.program, "sunModel");
	GLint earthUniform = glGetUniformLocation(shader.program, "earthModel");
	GLint moonUniform = glGetUniformLocation(shader.program, "moonModel");
	GLint viewUniform = glGetUniformLocation(shader.program, "view");
	GLint projUniform = glGetUniformLocation(shader.program, "proj");
	GLint animUniform = glGetUniformLocation(shader.program, "animation");
	GLint camUniform = glGetUniformLocation(shader.program, "camPoint");

	// set texture uniforms
	GLint texUniform;
	texUniform = glGetUniformLocation(shader.program, "tex0");
	glUniform1i(texUniform, 0);
	texUniform = glGetUniformLocation(shader.program, "tex1");
	glUniform1i(texUniform, 1);
	texUniform = glGetUniformLocation(shader.program, "tex2");
	glUniform1i(texUniform, 2);
	texUniform = glGetUniformLocation(shader.program, "tex3");
	glUniform1i(texUniform, 3);
	texUniform = glGetUniformLocation(shader.program, "tex4");
	glUniform1i(texUniform, 4);
	texUniform = glGetUniformLocation(shader.program, "tex5");
	glUniform1i(texUniform, 5);

	// set lighting uniforms
	GLint u;
	u = glGetUniformLocation(shader.program, "light");
	glUniform3fv(u, 1, light);
	u = glGetUniformLocation(shader.program, "ambient");
	glUniform1f(u, ambient);
	u = glGetUniformLocation(shader.program, "diffRatio");
	glUniform1f(u, diffRatio);
	u = glGetUniformLocation(shader.program, "intensity");
	glUniform1f(u, intensity);
	u = glGetUniformLocation(shader.program, "glowInt");
	glUniform1f(u, glowIntensity);
	u = glGetUniformLocation(shader.program, "phong");
	glUniform1f(u, phong);
	u = glGetUniformLocation(shader.program, "waterPhong");
	glUniform1f(u, waterPhong);
	u = glGetUniformLocation(shader.program, "specColour");
	glUniform3fv(u, 1, specColour);
	u = glGetUniformLocation(shader.program, "cloudInt");
	glUniform1f(u, cloudIntensity);

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		mat4 fixModel = rotate(I, xangle, xaxis);	// rotate model 90 degrees

		// sun model matrix
		float sunAngleR = yangle / sunRotate;
		mat4 sunModel = rotate(I, sunTilt, xaxis) *		// set tilt
						rotate(I, sunAngleR, yaxis) *	// self rotation
						fixModel;

		// earth model matrix
		float earthAngleR = yangle / earthRotate;
		float earthAngleO = yangle / earthOrbit;
		mat4 earthPos = rotate(I, earthAngleO, yaxis) *		// orbit rotation
						translate(I, earthLoc) *			// set orbit distance
						rotate(I, -earthAngleO, yaxis) *	// counter orbit rotation
						rotate(I, earthTilt, xaxis);		// set tilt
		mat4 earthModel = earthPos *
						rotate(I, earthAngleR, yaxis) *		// self rotation
						fixModel;

		// moon model matrix
		float moonAngleO = yangle / moonOrbit;
		mat4 moonPos = earthPos *
						rotate(I, moonIncl, zaxis) *		// orbit inclination
						rotate(I, moonAngleO, yaxis) *		// orbit rotation
						translate(I, moonLoc) *				// set orbit distance
						rotate(I, moonTilt, xaxis);			// set tilt
		mat4 moonModel = moonPos * 
						fixModel;

		// stars model matrix
		mat4 starsModel = fixModel;

		// camera and view/projection matrices
		float camX = cameraR * cos(cameraP) * sin(cameraT);
		float camY = cameraR * cos(cameraT);
		float camZ = cameraR * sin(cameraP) * sin(cameraT);
		vec3 cameraLoc(camX, camY, camZ);
		mat4 focus = I;
		if (camFocus == 1) focus = earthPos;
		else if (camFocus == 2) focus = moonPos;
		cameraLoc = focus * vec4(cameraLoc, 1.0);
		vec3 cameraDir = focus * vec4(0.0, 0.0, 0.0, 1.0) - vec4(cameraLoc, 1.0);
		vec3 cx = cross(yaxis, cameraDir);
		vec3 cameraUp = normalize(cross(cameraDir, cx));
		GLfloat camPoint[3] = { cameraLoc.x, cameraLoc.y, cameraLoc.z };

		mat4 view = lookAt(cameraLoc, cameraLoc + cameraDir, cameraUp);
		mat4 proj = perspective(fov, aspectRatio, zNear, zFar);

		// set uniforms
		glUseProgram(shader.program);
		glUniformMatrix4fv(starsUniform, 1, false, value_ptr(starsModel));
		glUniformMatrix4fv(sunUniform, 1, false, value_ptr(sunModel));
		glUniformMatrix4fv(earthUniform, 1, false, value_ptr(earthModel));
		glUniformMatrix4fv(moonUniform, 1, false, value_ptr(moonModel));
		glUniformMatrix4fv(viewUniform, 1, false, value_ptr(view));
		glUniformMatrix4fv(projUniform, 1, false, value_ptr(proj));
		glUniform1f(animUniform, yangle);
		glUniform3fv(camUniform, 1, camPoint);

		// call function to draw our scene
		RenderScene(&geometry, &shader, textures);

		// update animation values
		if (animate) yangle += animSpeed * (glfwGetTime() - lastFrameTime);
		lastFrameTime = glfwGetTime();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	DestroyShaders(&shader);
	for (int i = 0; i < 6; i++)
		DestroyTexture(&textures[i]);

	glfwDestroyWindow(window);
	glfwTerminate();

	cout << "Goodbye!" << endl;
	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file "
			<< filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
