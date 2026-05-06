#include <array>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>

using namespace std;
using namespace glm;

GLFWwindow* window;
GLboolean keyboard[512] = { GL_FALSE };
bool lightingEnabled = true;

int windowWidth = 800;
int windowHeight = 600;
char windowTitle[] = "Szamitogepes Grafika - III. Beadanado";

// Shader helyek (locations)
GLuint renderingProgram;
GLuint modelLoc, viewLoc, projectionLoc, inverseTransposeMatrixLoc;
GLuint lightPositionLoc, cameraPositionLoc, lightColorLoc;
GLuint lightingEnabledLoc, isSunLoc;

// --- KAMERA VÁLTOZÓK (Hengerkoordináták) ---
GLfloat camAlpha = 0.0f;       // Szög a z-tengely körül
GLfloat camHeight = 0.0f;      // Magasság a z-tengelyen
const GLfloat camRadius = 8.0f; // Sugár (8 <= r <= 10)

vec3 cameraPosition;
vec3 cameraTarget = vec3(0.0f, 0.0f, 0.0f);   // Mindig az origóba néz
vec3 cameraUpVector = vec3(0.0f, 0.0f, 1.0f); // UP vektor (0, 0, 1)

// --- FÉNY VÁLTOZÓK ---
vec3 lightPosition;
vec3 lightColor = vec3(1.0f, 0.8f, 0.0f); // Fehértől eltérő, Nap sárga fény

// Mátrixok
mat4 model, view, projection;
mat3 inverseTransposeMatrix;

// Geometria
#define numVBOs 2 // 0: Kocka, 1: Gömb
#define numVAOs 2 
GLuint VBO[numVBOs], VAO[numVAOs], sunTextureID;
vector<float> sphereData;

// A kocka adatai (pozíció, normálvektor). Az UV-t a shader kapja.
float cubeVertices[] = {
    // positions          normals
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
};

// Gömb és textúra generáló logikák
void generateSphereGeometry(float radius, int sectors, int stacks, vector<float>& vertices) {
    for (int i = 0; i < stacks; ++i) {
        float lat0 = glm::pi<float>() * (-0.5f + (float)(i) / stacks);
        float z0 = radius * sin(lat0);
        float zr0 = radius * cos(lat0);

        float lat1 = glm::pi<float>() * (-0.5f + (float)(i + 1) / stacks);
        float z1 = radius * sin(lat1);
        float zr1 = radius * cos(lat1);

        for (int j = 0; j < sectors; ++j) {
            float lng0 = 2 * glm::pi<float>() * (float)(j) / sectors;
            float x0 = cos(lng0); float y0 = sin(lng0);

            float lng1 = 2 * glm::pi<float>() * (float)(j + 1) / sectors;
            float x1 = cos(lng1); float y1 = sin(lng1);

            auto addVertex = [&](float x, float y, float z, float u, float v) {
                vertices.push_back(x); vertices.push_back(y); vertices.push_back(z); // Pozíció
                glm::vec3 n = glm::normalize(glm::vec3(x, y, z));
                vertices.push_back(n.x); vertices.push_back(n.y); vertices.push_back(n.z); // Normál
                vertices.push_back(u); vertices.push_back(v); // UV Textúra koordináta
                };

            // Háromszög 1
            addVertex(x0 * zr0, y0 * zr0, z0, (float)j / sectors, (float)i / stacks);
            addVertex(x1 * zr0, y1 * zr0, z0, (float)(j + 1) / sectors, (float)i / stacks);
            addVertex(x0 * zr1, y0 * zr1, z1, (float)j / sectors, (float)(i + 1) / stacks);

            // Háromszög 2
            addVertex(x1 * zr0, y1 * zr0, z0, (float)(j + 1) / sectors, (float)i / stacks);
            addVertex(x1 * zr1, y1 * zr1, z1, (float)(j + 1) / sectors, (float)(i + 1) / stacks);
            addVertex(x0 * zr1, y0 * zr1, z1, (float)j / sectors, (float)(i + 1) / stacks);
        }
    }
}

// Generál egy memóriába ágyazott alapszintű Nap textúrát 
void createDummySunTexture() {
    glGenTextures(1, &sunTextureID);
    glBindTexture(GL_TEXTURE_2D, sunTextureID);
    // Egy egyszerű sárgás-fehér gradiens zaj vagy egy 2x2 pixeles kép szimulálására a textúrát
    // Itt ideális esetben stbi_load() lenne használva egy internetről letöltött képhez!
    unsigned char texData[] = { 255, 255, 200, 255,  255, 230, 150, 255,
                                255, 240, 100, 255,  255, 255, 255, 255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

// Shader betöltő
string readShaderSource(const char* filePath) {
    string content;
    ifstream fileStream(filePath, ios::in);
    string line = "";
    while (!fileStream.eof()) {
        getline(fileStream, line);
        content.append(line + "\n");
    }
    fileStream.close();
    return content;
}

GLuint createShaderProgram() {
    string vertShaderStr = readShaderSource("vertexShader.glsl");
    string fragShaderStr = readShaderSource("fragmentShader.glsl");

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* vertShaderSrc = vertShaderStr.c_str();
    const char* fragShaderSrc = fragShaderStr.c_str();

    glShaderSource(vShader, 1, &vertShaderSrc, NULL);
    glShaderSource(fShader, 1, &fragShaderSrc, NULL);

    glCompileShader(vShader);
    glCompileShader(fShader);

    GLuint vfProgram = glCreateProgram();
    glAttachShader(vfProgram, vShader);
    glAttachShader(vfProgram, fShader);
    glLinkProgram(vfProgram);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    return vfProgram;
}

void init(GLFWwindow* window) {
    renderingProgram = createShaderProgram();
    glGenBuffers(numVBOs, VBO);
    glGenVertexArrays(numVAOs, VAO);

    // KOCKA INICIALIZÁLÁS (VAO[0])
    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // GÖMB INICIALIZÁLÁS (VAO[1]) - A Nap számára (d = 0.5 => r = 0.25)
    generateSphereGeometry(0.25f, 36, 18, sphereData);
    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sphereData.size() * sizeof(float), sphereData.data(), GL_STATIC_DRAW);

    // Vertex adatok a Gömbnek (Pos: 3, Norm: 3, UV: 2 -> Lépésköz 8 float)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    createDummySunTexture();

    glUseProgram(renderingProgram);

    // Lekérdezzük a location értékeket
    modelLoc = glGetUniformLocation(renderingProgram, "matModel");
    viewLoc = glGetUniformLocation(renderingProgram, "matView");
    projectionLoc = glGetUniformLocation(renderingProgram, "matProjection");
    inverseTransposeMatrixLoc = glGetUniformLocation(renderingProgram, "inverseTransposeMatrix");
    lightPositionLoc = glGetUniformLocation(renderingProgram, "lightPosition");
    cameraPositionLoc = glGetUniformLocation(renderingProgram, "cameraPosition");
    lightColorLoc = glGetUniformLocation(renderingProgram, "lightColor");
    lightingEnabledLoc = glGetUniformLocation(renderingProgram, "u_lightingEnabled");
    isSunLoc = glGetUniformLocation(renderingProgram, "u_isSun");

    glClearColor(0.0, 0.0, 0.0, 1.0); // Fekete háttér
    glEnable(GL_DEPTH_TEST);          // Z-buffer bekapcsolása (Fontos láthatósághoz!)
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static GLdouble lastTime = 0.0f;
    GLdouble currentTime = glfwGetTime();
    GLdouble deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    GLfloat cameraSpeed = 2.0f * (GLfloat)deltaTime;

    // Kamera vezérlése (Z-tengely körüli és menti forgás/mozgás)
    if (keyboard[GLFW_KEY_LEFT])  camAlpha -= cameraSpeed;
    if (keyboard[GLFW_KEY_RIGHT]) camAlpha += cameraSpeed;
    if (keyboard[GLFW_KEY_UP])    camHeight += cameraSpeed;
    if (keyboard[GLFW_KEY_DOWN])  camHeight -= cameraSpeed;

    // Hengerkoordináták konvertálása Derékszögű (Descartes) koordinátákká
    cameraPosition = vec3(camRadius * cos(camAlpha), camRadius * sin(camAlpha), camHeight);

    // A fény folyamatos mozgatása körpályán Z=0 síkon r = 2 * r_cam
    GLfloat lightMovementRadius = 2.0f * camRadius;
    lightPosition = vec3(lightMovementRadius * cos(currentTime), lightMovementRadius * sin(currentTime), 0.0f);

    // View Matrix (A kamera mindig az origóba néz)
    view = lookAt(cameraPosition, cameraTarget, cameraUpVector);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, value_ptr(view));

    // Uniform adatok küldése
    glUniform3fv(lightPositionLoc, 1, value_ptr(lightPosition));
    glUniform3fv(cameraPositionLoc, 1, value_ptr(cameraPosition));
    glUniform3fv(lightColorLoc, 1, value_ptr(lightColor));
    glUniform1i(lightingEnabledLoc, lightingEnabled ? 1 : 0);

    // KOCKÁK KIRAJZOLÁSA (3 darab: Z = 0, Z = 2, Z = -2. Igy 1 egyseg marad köztük)
    glBindVertexArray(VAO[0]);
    glUniform1i(isSunLoc, 0); // Nem a Napot rajzoljuk
    vec3 cubePositions[] = { vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 2.0f), vec3(0.0f, 0.0f, -2.0f) };

    for (int i = 0; i < 3; i++) {
        model = translate(mat4(1.0f), cubePositions[i]);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));

        inverseTransposeMatrix = mat3(inverseTranspose(model));
        glUniformMatrix3fv(inverseTransposeMatrixLoc, 1, GL_FALSE, value_ptr(inverseTransposeMatrix));

        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    // NAP (FÉNYFORRÁS) GÖMB KIRAJZOLÁSA (Pozíciója egyezik a fényforráséval)
    glBindVertexArray(VAO[1]);
    glUniform1i(isSunLoc, 1); // Most a Napot rajzoljuk világítás nélkül!

    // A gömb eleve r=0.25 sugárral készült, így csak eltoljuk a lightPosition-re
    model = translate(mat4(1.0f), lightPosition);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, value_ptr(model));
    inverseTransposeMatrix = mat3(inverseTranspose(model));
    glUniformMatrix3fv(inverseTransposeMatrixLoc, 1, GL_FALSE, value_ptr(inverseTransposeMatrix));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sunTextureID);

    glDrawArrays(GL_TRIANGLES, 0, sphereData.size() / 8);
}

void cleanUpScene() {
    glfwDestroyWindow(window);
    glDeleteVertexArrays(numVAOs, VAO);
    glDeleteBuffers(numVBOs, VBO);
    glDeleteProgram(renderingProgram);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    windowWidth = glm::max(width, 1);
    windowHeight = glm::max(height, 1);
    glViewport(0, 0, windowWidth, windowHeight);

    // Vetítés: Perspective 55 fokos értékkel!
    projection = perspective(radians(55.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if ((action == GLFW_PRESS) && (key == GLFW_KEY_ESCAPE)) cleanUpScene();

    if (action == GLFW_PRESS) keyboard[key] = GL_TRUE;
    else if (action == GLFW_RELEASE) keyboard[key] = GL_FALSE;

    // Fény ki/be kapcsolása az 'L' gombbal
    if (key == GLFW_KEY_L && action == GLFW_PRESS) {
        lightingEnabled = !lightingEnabled;
    }
}

int main(void) {
    if (!glfwInit()) exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetKeyCallback(window, keyCallback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) exit(EXIT_FAILURE);
    glfwSwapInterval(1);

    init(window);
    framebufferSizeCallback(window, windowWidth, windowHeight);

    cout << "Iranyitas: " << endl;
    cout << "Jobb/Bal Nyil: Kamera forgatasa a Z tengely korul" << endl;
    cout << "Fel/Le Nyil: Kamera mozgatasa a Z tengely menten fel-le" << endl;
    cout << "L: Vilagitas Be / Ki kapcsolasa" << endl;

    while (!glfwWindowShouldClose(window)) {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    cleanUpScene();
    return EXIT_SUCCESS;
}