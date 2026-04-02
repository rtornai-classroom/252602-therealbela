#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Global Variables
std::vector<glm::vec3> controlPoints;
int selectedPoint = -1;
bool isDragging = false;
int windowWidth = 600, windowHeight = 600;

using namespace std;

#define numVAOs 1

GLuint curveProgram;  // For the Bezier curve
GLuint basicProgram;  // For the lines and dots
GLuint vao, vbo;
GLint colorLoc;
GLuint renderingProgram;

// Forward Declarations
glm::vec3 screenToWorld(double xpos, double ypos);
void updateBuffers();

bool checkOpenGLError() {
	bool foundError = false;
	int glErr = glGetError();
	while (glErr != GL_NO_ERROR) {
		cout << "glError: " << glErr << endl;
		foundError = true;
		glErr = glGetError();
	}
	return foundError;
}

void printShaderLog(GLuint shader) {
	int len = 0;
	int chWrittn = 0;
	char* log;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)malloc(len);
		glGetShaderInfoLog(shader, len, &chWrittn, log);
		cout << "Shader Info Log: " << log << endl;
		free(log);
	}
}

void printProgramLog(int prog) {
	int len = 0;
	int chWrittn = 0;
	char* log;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	if (len > 0) {
		log = (char*)malloc(len);
		glGetProgramInfoLog(prog, len, &chWrittn, log);
		cout << "Program Info Log: " << log << endl;
		free(log);
	}
}

//Függvény a shader fájlok sorainak beolvasásához
string readShaderSource(const char* filePath) {
	string content;
	ifstream fileStream(filePath, ios::in);

	if (!fileStream.is_open()) {
		cerr << "Could not read file " << filePath << ". File does not exist." << endl;
		return "";
	}

	string line = "";
	while (getline(fileStream, line)) {
		content.append(line + "\n");
	}
	fileStream.close();
	return content;
}

GLuint createShaderProgram() {
	string v = readShaderSource("vertexShader.glsl");
	string f = readShaderSource("fragmentShader.glsl");
	string tcs = readShaderSource("tessControlShader.glsl");
	string tes = readShaderSource("tessEvalShader.glsl");

	auto compile = [](const char* src, GLenum type) {
		GLuint s = glCreateShader(type);
		glShaderSource(s, 1, &src, NULL);
		glCompileShader(s);
		return s;
		};

	GLuint vs = compile(v.c_str(), GL_VERTEX_SHADER);
	GLuint fs = compile(f.c_str(), GL_FRAGMENT_SHADER);
	GLuint tc = compile(tcs.c_str(), GL_TESS_CONTROL_SHADER);
	GLuint te = compile(tes.c_str(), GL_TESS_EVALUATION_SHADER);

	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glAttachShader(prog, tc);
	glAttachShader(prog, te);
	glLinkProgram(prog);
	return prog;
}

GLuint compileShader(const char* src, GLenum type) {
	GLuint s = glCreateShader(type);
	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);
	// (Add your checkShaderCompile logic here for debugging)
	return s;
}

void initShaders() {
	string v = readShaderSource("vertexShader.glsl");
	string f = readShaderSource("fragmentShader.glsl");
	string tcs = readShaderSource("tessControlShader.glsl");
	string tes = readShaderSource("tessEvalShader.glsl");

	const char* vs_src = v.c_str();
	const char* fs_src = f.c_str();
	const char* tcs_src = tcs.c_str();
	const char* tes_src = tes.c_str();

	// --- Build Curve Program (All 4 shaders) ---
	GLuint vs = compileShader(vs_src, GL_VERTEX_SHADER);
	GLuint fs = compileShader(fs_src, GL_FRAGMENT_SHADER);
	GLuint tc = compileShader(tcs_src, GL_TESS_CONTROL_SHADER);
	GLuint te = compileShader(tes_src, GL_TESS_EVALUATION_SHADER);

	curveProgram = glCreateProgram();
	glAttachShader(curveProgram, vs);
	glAttachShader(curveProgram, fs);
	glAttachShader(curveProgram, tc);
	glAttachShader(curveProgram, te);
	glLinkProgram(curveProgram);

	// --- Build Basic Program (Only Vertex and Fragment) ---
	basicProgram = glCreateProgram();
	glAttachShader(basicProgram, vs);
	glAttachShader(basicProgram, fs);
	glLinkProgram(basicProgram);

	glDeleteShader(vs); glDeleteShader(fs); glDeleteShader(tc); glDeleteShader(te);
}

//A jelenetünk utáni takarítás.
void cleanUpScene() {
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(renderingProgram);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

}

void cursorPosCallback(GLFWwindow* window, double x, double y) {
	if (isDragging && selectedPoint != -1) {
		controlPoints[selectedPoint] = screenToWorld(x, y);
		updateBuffers();
	}
}

// --- Interaction ---
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	double x, y;
	glfwGetCursorPos(window, &x, &y);
	glm::vec3 mouseWorld = screenToWorld(x, y);

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		selectedPoint = -1;
		for (int i = 0; i < controlPoints.size(); i++) {
			if (glm::distance(mouseWorld, controlPoints[i]) < 0.05f) {
				selectedPoint = i;
				isDragging = true;
				return;
			}
		}
		controlPoints.push_back(mouseWorld);
		updateBuffers();
	}
	else if (action == GLFW_RELEASE) {
		isDragging = false;
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		for (int i = 0; i < controlPoints.size(); i++) {
			if (glm::distance(mouseWorld, controlPoints[i]) < 0.05f) {
				controlPoints.erase(controlPoints.begin() + i);
				updateBuffers();
				break;
			}
		}
	}
}

glm::vec3 screenToWorld(double xpos, double ypos) {
	float x = (2.0f * (float)xpos) / windowWidth - 1.0f;
	float y = 1.0f - (2.0f * (float)ypos) / windowHeight;
	return glm::vec3(x, y, 0.0f);
}

void updateBuffers() {
	if (controlPoints.empty()) return;
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), controlPoints.data(), GL_DYNAMIC_DRAW);
}



void init(GLFWwindow* window) {
	// 1. Enable point rounding and sizing
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// --- NEW: Create 4 initial points in a zig-zag pattern ---
	controlPoints.push_back(glm::vec3(-0.6f, -0.4f, 0.0f));
	controlPoints.push_back(glm::vec3(-0.2f, 0.4f, 0.0f));
	controlPoints.push_back(glm::vec3(0.2f, -0.4f, 0.0f));
	controlPoints.push_back(glm::vec3(0.6f, 0.4f, 0.0f));

	// 2. Load Source
	string v = readShaderSource("vertexShader.glsl");
	string f = readShaderSource("fragmentShader.glsl");
	string tcs = readShaderSource("tessControlShader.glsl");
	string tes = readShaderSource("tessEvalShader.glsl");

	const char* vs_src = v.c_str();
	const char* fs_src = f.c_str();
	const char* tcs_src = tcs.c_str();
	const char* tes_src = tes.c_str();

	// 3. Build Programs
	GLuint vs = compileShader(vs_src, GL_VERTEX_SHADER);
	GLuint fs = compileShader(fs_src, GL_FRAGMENT_SHADER);
	GLuint tc = compileShader(tcs_src, GL_TESS_CONTROL_SHADER);
	GLuint te = compileShader(tes_src, GL_TESS_EVALUATION_SHADER);

	curveProgram = glCreateProgram();
	glAttachShader(curveProgram, vs);
	glAttachShader(curveProgram, fs);
	glAttachShader(curveProgram, tc);
	glAttachShader(curveProgram, te);
	glLinkProgram(curveProgram);

	basicProgram = glCreateProgram();
	glAttachShader(basicProgram, vs);
	glAttachShader(basicProgram, fs);
	glLinkProgram(basicProgram);

	// 4. Buffers
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Make sure to call updateBuffers() at the end of init!
	updateBuffers();
}

void display(GLFWwindow* window, double currentTime) {
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	if (controlPoints.empty()) return;

	glBindVertexArray(vao);

	// 1. Draw actual Bezier Curve (Red)
	glUseProgram(curveProgram);
	glUniform3f(glGetUniformLocation(curveProgram, "uColor"), 1.0f, 0.2f, 0.2f);
	glUniform1i(glGetUniformLocation(curveProgram, "isDrawingPoint"), 0);
	glPatchParameteri(GL_PATCH_VERTICES, (GLint)controlPoints.size());
	glDrawArrays(GL_PATCHES, 0, (GLsizei)controlPoints.size());

	// 2. Draw Polygon and Points (Basic)
	glUseProgram(basicProgram);
	GLint colorLoc = glGetUniformLocation(basicProgram, "uColor");
	GLint isPointLoc = glGetUniformLocation(basicProgram, "isDrawingPoint");

	// Draw Lines (Green)
	glUniform3f(colorLoc, 0.2f, 0.8f, 0.2f);
	glUniform1i(isPointLoc, 0);
	glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)controlPoints.size());

	// Draw Points (Blue)
	glUniform3f(colorLoc, 0.3f, 0.6f, 1.0f);
	glUniform1i(isPointLoc, 1);
	glPointSize(15.0f);
	glDrawArrays(GL_POINTS, 0, (GLsizei)controlPoints.size());
}

int main() {
	if (!glfwInit()) return -1;

	cout << "Program Hasznalata" << endl;
	cout << " Bal egergomb:\tpont hozzaadasa" << endl;
	cout << " Jobb egergomb:\tpont torlese" << endl;
	cout << " Drag and drop teknikaval a pontok pozgathatoak" << endl;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(600, 600, "Bezier Gorbe", NULL, NULL);
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();

	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);

	init(window);
	while (!glfwWindowShouldClose(window)) {
		display(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}