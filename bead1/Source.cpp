#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <vector>

using namespace std;

#define numVAOs 1

GLuint renderingProgram;
GLuint vao[numVAOs];

// Állapotváltozók
float circleX = 0.0f, circleY = 0.0f;// Kör helyzete (NDC(Normalized Device Coordinates): -1 és 1 között)
float velX = 0.0f, velY = 0.0f;// Sebesség
float lineY = 0.0f;// Vízszintes szakasz függõleges pozíciója
bool isMoving = true;// Mozgás állapotjelzõ
bool colorsSwapped = false;// A kör színezésének állapota

// Konstansok (pixelrõl NDC-re váltva: érték / 300.0)
const float RADIUS = 50.0f / 300.0f; // 50px sugár
const float STEP_LENGTH = 10.0f / 300.0f;// A 10px-es lépéshossz
const float LINE_WIDTH_HALF = (200.0f / 2.0f) / 300.0f;// A szakasz félhosszúsága


// Ezek a függvények kezelik azt, ha a GPU nem tudja lefordítani a shadereket vagy belsõ hiba történik.
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

//Függvény a shader fájlok sorainak beolvasásához, string típusba
string readShaderSource(const char* filePath) {
	string content;
	ifstream fileStream(filePath, ios::in);

	if (!fileStream.is_open()) {
		cerr << "Could not read file " << filePath << ". File does not exist." << endl;
		return "";
	}

	string line = "";
	while (getline(fileStream, line)) { // Olvasás sikerességének vizsgálata
		content.append(line + "\n");
	}
	fileStream.close();
	return content;
}

GLuint createShaderProgram() {

	GLint vertCompiled;
	GLint fragCompiled;
	GLint linked;

	//Beolvassuk a shader fájlok tartalmát
	string vertShaderStr = readShaderSource("vertexShader.glsl");
	string fragShaderStr = readShaderSource("fragmentShader.glsl");

	//Létrehozzuk a shader objektumainkat
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);

	//A shader fájlok tartalmát eltároló string objektum szöveggé konvertálását is elvégezzük
	const char* vertShaderSrc = vertShaderStr.c_str();
	const char* fragShaderSrc = fragShaderStr.c_str();

	//Ekkor a betöltött kódot hozzárendelhetjük a shader objektumainkhoz.
	glShaderSource(vShader, 1, &vertShaderSrc, NULL);
	glShaderSource(fShader, 1, &fragShaderSrc, NULL);

	//Fordítsuk le ezen shader objektumhoz rendelt kódot
	glCompileShader(vShader);
	//Hibakeresési lépések.Például sikeres volt - e a fordítás ? Ha nem, mi volt az oka ?
	checkOpenGLError();
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &vertCompiled);
	if (vertCompiled != 1) {
		cout << "vertex compilation failed" << endl;
		printShaderLog(vShader);
	}

	//A második shader objektumhoz rendelt kódunkat is lefordítjuk.
	glCompileShader(fShader);
	//Ismét hibakeresési lépések.Például sikeres volt - e a fordítás ? Ha nem, mi volt az oka ?
	checkOpenGLError();
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &fragCompiled);
	if (fragCompiled != 1) {
		cout << "fragment compilation failed" << endl;
		printShaderLog(fShader);
	}

	//Shader program objektum létrehozása : összefogja a shadereket.Eltároljuk az ID értékét
	GLuint vfProgram = glCreateProgram();
	//Csatoljuk a shadereket az elõzõ lépésben létrehozott objektumhoz.
	glAttachShader(vfProgram, vShader);
	glAttachShader(vfProgram, fShader);

	//Végül a GLSL fordítónk ellenõrzi, hogy a csatolt shaderek kompatibilisek - e egymással.
	glLinkProgram(vfProgram);
	//Ha hiba lépett fel, nézzük meg mi volt ennek az oka.
	checkOpenGLError();
	glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
	if (linked != 1) {
		cout << "linking failed" << endl;
		printProgramLog(vfProgram);
	}

	//Ha minden rendben ment a linkelés során, az objektumok leválaszthatóak a programról.
	glDeleteShader(vShader);
	glDeleteShader(fShader);

	//A kész program a visszatérési értékünk.
	return vfProgram;
}

//A jelenetünk utáni takarítás.
void cleanUpScene()
{
	//Töröljük a vertex array objektumokat.
	glDeleteVertexArrays(1, vao);

	//Töröljük a shader programot.
	glDeleteProgram(renderingProgram);
}

// Ütközésvizsgálat a szakasz és a kör között
bool checkIntersection() {
	// A szakasz x tartománya: [-LINE_WIDTH_HALF, LINE_WIDTH_HALF]
	// A szakasz y magassága: lineY
	float closestX = max(-LINE_WIDTH_HALF, min(circleX, LINE_WIDTH_HALF));
	float dx = circleX - closestX;
	float dy = circleY - lineY;
	return (dx * dx + dy * dy) <= (RADIUS * RADIUS);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_S) {
			// Feladat szerinti indítás: 25 fokos irányvektor
			float alpha = 25.0f * M_PI / 180.0f;
			velX = cos(alpha) * STEP_LENGTH;
			velY = sin(alpha) * STEP_LENGTH;
		}
		// Szakasz mozgatása a nyilakkal
		if (key == GLFW_KEY_UP)    lineY += 0.05f;
		if (key == GLFW_KEY_DOWN)  lineY -= 0.05f;
	}
}

void updatePhysics() {
	circleX += velX;
	circleY += velY;

	// Arányosított visszapattanás (X tengely)
	if (circleX + RADIUS > 1.0f) {
		float overshoot = (circleX + RADIUS) - 1.0f;
		circleX = 1.0f - RADIUS - overshoot;
		velX = -velX;
	}
	else if (circleX - RADIUS < -1.0f) {
		float overshoot = -1.0f - (circleX - RADIUS);
		circleX = -1.0f + RADIUS + overshoot;
		velX = -velX;
	}

	// Arányosított visszapattanás (Y tengely)
	if (circleY + RADIUS > 1.0f) {
		float overshoot = (circleY + RADIUS) - 1.0f;
		circleY = 1.0f - RADIUS - overshoot;
		velY = -velY;
	}
	else if (circleY - RADIUS < -1.0f) {
		float overshoot = -1.0f - (circleY - RADIUS);
		circleY = -1.0f + RADIUS + overshoot;
		velY = -velY;
	}

	// Színcsere logika: Ha nincs metszés, felcseréljük
	colorsSwapped = !checkIntersection();
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos)
{

}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{

}

void init(GLFWwindow* window) {
	renderingProgram = createShaderProgram();
	glGenVertexArrays(1, vao);
	glBindVertexArray(vao[0]);

	// KEZDETI MOZGÁS BEÁLLÍTÁSA
	// Csak az X tengely mentén mozog oda-vissza
	velX = 0.015f; // Lassabb kezdõ sebesség a vízszintes úthoz
	velY = 0.0f;
}

void display(GLFWwindow* window, double currentTime) {
	// Sárga háttér
	glClearColor(1.0f, 0.9f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	updatePhysics();

	glUseProgram(renderingProgram);

	// Uniformok átadása
	GLuint circleLoc = glGetUniformLocation(renderingProgram, "circleCenter");
	glUniform2f(circleLoc, circleX, circleY);

	GLuint swapLoc = glGetUniformLocation(renderingProgram, "swapColors");
	glUniform1i(swapLoc, colorsSwapped);

	GLuint isLineLoc = glGetUniformLocation(renderingProgram, "isLine");

	// KÖR RAJZOLÁSA
	glUniform1i(isLineLoc, 0);
	glPointSize(100.0f); // Átmérõ (2*50px)
	glBindVertexArray(vao[0]);
	glDrawArrays(GL_POINTS, 0, 1);

	// KÉK SZAKASZ RAJZOLÁSA
	glUniform1i(isLineLoc, 1);
	glLineWidth(3.0f);

	GLuint lineYLoc = glGetUniformLocation(renderingProgram, "lineY");
	glUniform1f(lineYLoc, lineY);
	glDrawArrays(GL_LINES, 0, 2);
}

int main(void) {

	//Próbáljuk meg inicializálni a GLFW - t!
	if (!glfwInit()) { exit(EXIT_FAILURE); }

	cout << "Program Hasznalata" << endl;
	cout << " Iranyitas:" << endl;
	cout << " [S] billentyu:\tA kor inditasa 25 fokos szogben" << endl;
	cout << " [FEL] nyil:\tA kek szakasz mozgatasa felfele" << endl;
	cout << " [LE] nyil:\tA kek szakasz mozgatasa lefele" << endl;

	//A kívánt OpenGL verzió(4.3)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	//Próbáljuk meg létrehozni az ablakunkat
	GLFWwindow* window = glfwCreateWindow(600, 600, "Drawing a point", NULL, NULL);

	//Válasszuk ki az ablakunk OpenGL kontextusát, hogy használhassuk
	glfwMakeContextCurrent(window);

	//Billentyûzethez köthetõ események kezelése
	glfwSetKeyCallback(window, keyCallback);

	//Incializáljuk a GLEW - t, hogy elérhetõvé váljanak az OpenGL függvények
	if (glewInit() != GLEW_OK) { exit(EXIT_FAILURE); }
	glfwSwapInterval(1);

	//Az alkalmazáshoz kapcsolódó elõkészítõ lépések, pl.hozd létre a shader objektumokat.
	init(window);

	while (!glfwWindowShouldClose(window)) {
		/** a kód, amellyel rajzolni tudunk a GLFWwindow ojektumunkba. */
		display(window, glfwGetTime());
		/** double buffered */
		glfwSwapBuffers(window);
		/** események kezelése az ablakunkkal kapcsolatban, pl. gombnyomás */
		glfwPollEvents();
	}

	/** Töröljük a GLFW ablakot. */
	glfwDestroyWindow(window);
	/** Leállítjuk a GLFW-t */
	glfwTerminate();

	/** Felesleges objektumok törlése */
	cleanUpScene();

	exit(EXIT_SUCCESS);
}