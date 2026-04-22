#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

// --- Konstansok a shader típusokhoz (Source.cpp alapján) ---
#define BEZIER_BERNSTEIN 3

// --- Globális változók ---
vector<glm::vec3> controlPoints;
int selectedPoint = -1;
bool isDragging = false;
int windowWidth = 800, windowHeight = 800;

GLuint curveProgram;  // CurveVert, CurveTessCont, CurveTessEval, CurveFrag
GLuint basicProgram;  // QuadScreenVert, QuadScreenFrag
GLuint vao, vbo;

// Mátrixok (Shader elvárja õket)
glm::mat4 matProjection, matModelView;

// --- Segédfüggvény: Shader beolvasás ---
string readShaderSource(const char* filePath) {
    ifstream fileStream(filePath, ios::in);
    if (!fileStream.is_open()) return "";
    string content((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());
    return content;
}

// --- Segédfüggvény: Shader fordítás ---
GLuint compileShader(GLenum type, const char* path) {
    string src = readShaderSource(path);
    const char* c_src = src.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &c_src, NULL);
    glCompileShader(shader);
    return shader;
}

void initShaders() {
    // 1. Görbe program (Tesszellációval)
    GLuint cv = compileShader(GL_VERTEX_SHADER, "CurveVertShader.glsl");
    GLuint cf = compileShader(GL_FRAGMENT_SHADER, "CurveFragShader.glsl");
    GLuint tc = compileShader(GL_TESS_CONTROL_SHADER, "CurveTessContShader.glsl");
    GLuint te = compileShader(GL_TESS_EVALUATION_SHADER, "CurveTessEvalShader.glsl");

    curveProgram = glCreateProgram();
    glAttachShader(curveProgram, cv);
    glAttachShader(curveProgram, cf);
    glAttachShader(curveProgram, tc);
    glAttachShader(curveProgram, te);
    glLinkProgram(curveProgram);

    // 2. Alap program (Kontroll poligonhoz és pontokhoz)
    GLuint qv = compileShader(GL_VERTEX_SHADER, "QuadScreenVertShader.glsl");
    GLuint qf = compileShader(GL_FRAGMENT_SHADER, "QuadScreenFragShader.glsl");

    basicProgram = glCreateProgram();
    glAttachShader(basicProgram, qv);
    glAttachShader(basicProgram, qf);
    glLinkProgram(basicProgram);
}

void updateBuffers() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, controlPoints.size() * sizeof(glm::vec3), controlPoints.data(), GL_DYNAMIC_DRAW);
}

glm::vec3 screenToWorld(double xpos, double ypos) {
    float x = (2.0f * (float)xpos) / windowWidth - 1.0f;
    float y = 1.0f - (2.0f * (float)ypos) / windowHeight;
    return glm::vec3(x, y, 0.0f);
}

// --- Interakciók ---
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    glm::vec3 mouseWorld = screenToWorld(x, y);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        selectedPoint = -1;
        // Kijelölés keresése
        for (int i = 0; i < controlPoints.size(); i++) {
            if (glm::distance(mouseWorld, controlPoints[i]) < 0.05f) {
                selectedPoint = i;
                isDragging = true;
                return;
            }
        }
        // Ha nem talált pontot, új pont hozzáadása
        controlPoints.push_back(mouseWorld);
        updateBuffers();
    } 
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        isDragging = false;
    }
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // Pont törlése
        for (int i = 0; i < controlPoints.size(); i++) {
            if (glm::distance(mouseWorld, controlPoints[i]) < 0.05f) {
                controlPoints.erase(controlPoints.begin() + i);
                updateBuffers();
                break;
            }
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double x, double y) {
    if (isDragging && selectedPoint != -1) {
        controlPoints[selectedPoint] = screenToWorld(x, y);
        updateBuffers();
    }
}

void init(GLFWwindow* window) {
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Kezdõ 4 pont (feladat szerint)
    controlPoints.push_back(glm::vec3(-0.6f, -0.4f, 0.0f));
    controlPoints.push_back(glm::vec3(-0.2f, 0.4f, 0.0f));
    controlPoints.push_back(glm::vec3(0.6f, 0.4f, 0.0f));
    controlPoints.push_back(glm::vec3(0.2f, -0.4f, 0.0f));

    initShaders();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    updateBuffers();

    // Identitás mátrixok alapértelmezésnek
    matProjection = glm::mat4(1.0f);
    matModelView = glm::mat4(1.0f);
}

void display(GLFWwindow* window) {
    // 1. Háttér törlése
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (controlPoints.empty()) return;

    glBindVertexArray(vao);

    // --- I. KONTROLL POLIGON (Kék vonalak) ---
    // Azért ezzel kezdünk, hogy ez legyen a legalsó réteg.
    glUseProgram(basicProgram);
    glUniformMatrix4fv(glGetUniformLocation(basicProgram, "matProjection"), 1, GL_FALSE, glm::value_ptr(matProjection));
    glUniformMatrix4fv(glGetUniformLocation(basicProgram, "matModelView"), 1, GL_FALSE, glm::value_ptr(matModelView));

    // isPoint = false -> Kék szín a vonalaknak [cite: 40]
    glUniform1i(glGetUniformLocation(basicProgram, "isPoint"), 0);
    glDrawArrays(GL_LINE_STRIP, 0, (int)controlPoints.size());


    // --- II. BÉZIER GÖRBE (Zöld ív) ---
    // A poligon felett, de a pontok alatt jelenik meg.
    glUseProgram(curveProgram);
    glUniformMatrix4fv(glGetUniformLocation(curveProgram, "matProjection"), 1, GL_FALSE, glm::value_ptr(matProjection));
    glUniformMatrix4fv(glGetUniformLocation(curveProgram, "matModelView"), 1, GL_FALSE, glm::value_ptr(matModelView));
    glUniform1i(glGetUniformLocation(curveProgram, "curveType"), BEZIER_BERNSTEIN);
    glUniform1i(glGetUniformLocation(curveProgram, "controlPointsNumber"), (int)controlPoints.size());

    // Átadjuk a pontokat a uniform tömbnek, hogy ne érjük el a 32-es patch limitet
    GLint pointsLoc = glGetUniformLocation(curveProgram, "u_ControlPoints");
    if (pointsLoc != -1) {
        glUniform3fv(pointsLoc, (int)controlPoints.size(), glm::value_ptr(controlPoints[0]));
    }

    // Fixen 1 patchet küldünk, a shader a uniformból dolgozik
    glPatchParameteri(GL_PATCH_VERTICES, 1);
    glDrawArrays(GL_PATCHES, 0, 1);


    // --- III. KONTROLLPONTOK (Piros körök) ---
    // Ezt rajzoljuk utoljára, így ez lesz legfelül.
    glUseProgram(basicProgram);
    // Ismét beállítjuk a mátrixokat, biztos ami biztos
    glUniformMatrix4fv(glGetUniformLocation(basicProgram, "matProjection"), 1, GL_FALSE, glm::value_ptr(matProjection));
    glUniformMatrix4fv(glGetUniformLocation(basicProgram, "matModelView"), 1, GL_FALSE, glm::value_ptr(matModelView));

    // isPoint = true -> Piros körök élsimítással [cite: 36, 37, 39]
    glUniform1i(glGetUniformLocation(basicProgram, "isPoint"), 1);
    glPointSize(15.0f);
    glDrawArrays(GL_POINTS, 0, (int)controlPoints.size());
}


int main() {
    if (!glfwInit()) return -1;
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Bezier Curve Editor", NULL, NULL);
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);

    init(window);

    while (!glfwWindowShouldClose(window)) {
        display(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}