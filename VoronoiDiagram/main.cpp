#include <GL/glew.h>
#include <GL/glut.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <chrono>

#define KEY_CLEAR 'c'
#define KEY_RANDOM 'r'

#define M_PI 3.14159265358979323846264338327950288

class cell {
public:
	glm::vec2 pos;
	glm::vec3 color;
	cell(glm::vec2 pos, glm::vec3 color) : pos(pos), color(color) {}
};

class CircleFan {
private:
	GLuint vboId, iboId;
	GLint attribVPosition;
	unsigned int numPoints;
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
public:
	~CircleFan() {
		glDeleteBuffers(1, &vboId);
		glDeleteBuffers(1, &iboId);
	}
	void build(double centerHeight, double depth, int points) {
		numPoints = points;
		// Generate bottom ring points
		// Bottom not filled in because it is never visible
		double delta = 2 * M_PI / numPoints;
		double angle = 0;
		for (size_t i = 0; i < numPoints; i++) {
			vertices.push_back(cos(angle));
			vertices.push_back(sin(angle));
			vertices.push_back(depth);
			angle += delta;
		}
		// Top center point
		vertices.push_back(0);
		vertices.push_back(0);
		vertices.push_back(centerHeight + depth);
		// Generate indices
		for (size_t i = 0; i < numPoints - 1; i++) {
			indices.push_back(numPoints);
			indices.push_back(i);
			indices.push_back(i + 1);
		}
		// Last connection
		indices.push_back(numPoints);
		indices.push_back(numPoints - 1);
		indices.push_back(0);
		// Generate IDs for VBO and IBO
		glGenBuffers(1, &vboId);
		glGenBuffers(1, &iboId);
		// Copy vertex data
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
		// Copy indices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);
		// Deselect
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	void render() const {
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboId);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawElements(GL_TRIANGLE_FAN, indices.size(), GL_UNSIGNED_INT, nullptr);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
};
CircleFan cone;
CircleFan circle;

// Parameters
const int vWidth = 800, vHeight = 800; // Viewport init data
double orthoScale = 1; // Orthographic scale
double coneRadius = 5, coneHeight = 1, coneDepth = -1;
double circleBaseRadius = 0.01, circleHoverRadius = 0.025;
int selection = -1;
std::vector<cell> cells; // Vector to hold the Voronoi cell data
// Matrices and viewport
glm::mat4 identity = glm::mat4(1.0);
glm::mat4 projectionMatrix, viewMatrix, projViewMatrix;
glm::vec4 viewport;
// Mouse position in the world, init as non-zero
glm::dvec2 mouseWorldPos(1, 1);
// Random Number Generation
std::uniform_real_distribution<double> colorDist(0.0, 1.0), viewDistX, viewDistY;
std::random_device rd;
std::mt19937 gen(rd());
unsigned int randomAmount = 12500;
// Shader data
unsigned int shader;
GLint uColor;
GLint projViewMatLoc;
GLint modelMatLoc;
// Frame time
#ifdef _DEBUG
const bool enableFrameTimeCheck = true;
std::chrono::high_resolution_clock::time_point oldTime;
float frameTimeSmoothing = 0.9f;
float smoothedFrameTime = 0;
#else
const bool enableFrameTimeCheck = false;
#endif

// Random vec3 color
glm::vec3 randomColor(std::uniform_real_distribution<double> range) {
	return glm::vec3(range(gen), range(gen), range(gen));
}
// Random vec2 position
glm::vec2 randomPos(std::uniform_real_distribution<double> x, std::uniform_real_distribution<double> y) {
	return glm::vec2(x(gen), y(gen));
}

// Get the worldspace coordinate of the mouse cursor position
void mouseToWorld(int mouseX, int mouseY) {
	mouseWorldPos = glm::unProject(glm::vec3((GLfloat)mouseX, (GLfloat)(viewport.w - mouseY), 0), identity, projectionMatrix, viewport);
}

// Check if a given point is inside a given circle
bool pointInCircle(glm::dvec2 center, double radius, glm::dvec2 point) {
	double dx = std::abs(point.x - center.x);
	double dy = std::abs(point.y - center.y);
	return dx * dx + dy * dy <= radius * radius;
}

// Read a file into a string
std::string parseFile(const std::string& filePath) {
	std::ifstream stream(filePath);
	std::stringstream strStream;
	strStream << stream.rdbuf();
	return strStream.str();
}
// Compile a shader from a string of a specified type
int compileShader(unsigned int type, const std::string& source) {
	// Get source and type, compile ID
	unsigned int id = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(id, 1, &src, nullptr); // Assume source is null terminated
	glCompileShader(id);
	// Check if shader compiled correctly
	int status;
	glGetShaderiv(id, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		// TODO print error message
		// Failed to compile
		return 0;
	}
	return id;
}
// Create the shader program from the vertex and fragement shaders
unsigned int createShader(const std::string& vertexShader, const std::string& fragmentShader) {
	// Compile shaders
	unsigned int program = glCreateProgram();
	unsigned int vert = compileShader(GL_VERTEX_SHADER, vertexShader); // TODO maybe assert these
	unsigned int frag = compileShader(GL_FRAGMENT_SHADER, fragmentShader);
	// Link shader to program
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);
	glValidateProgram(program);
	// Shaders are already linked to a program, original no longer necessary
	glDeleteShader(vert);
	glDeleteShader(frag);
	return program;
}

// Display callback
void display() {
	if (enableFrameTimeCheck) {
		// Print frame time in milliseconds using a moving average
		auto currentTime = std::chrono::high_resolution_clock::now();
		auto elapsedCount = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - oldTime).count();
		oldTime = currentTime;
		smoothedFrameTime = (smoothedFrameTime * frameTimeSmoothing) + (elapsedCount * (1.0f - frameTimeSmoothing));
		std::cout << "Av. time (ms): " << smoothedFrameTime << "\n";
	}
	// Clear screen and reset selection
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	selection = -1;
	// Draw each cell
	for (size_t i = 0; i < cells.size(); i++) {
		// Draw a circle at the centre of the cone
		glm::mat4 modelCircle = glm::translate(identity, glm::vec3(cells[i].pos, -1));
		glm::mat4 modelCone = modelCircle;
		// If the mouse is intersecting the circle
		if (pointInCircle(cells[i].pos, circleHoverRadius, mouseWorldPos) && selection < 0) {
			// Select it
			selection = i;
			modelCircle = glm::scale(modelCircle, glm::vec3(glm::vec2(circleHoverRadius), 1));
			glUniform4f(uColor, 1, 1, 1, 1);
		} else {
			modelCircle = glm::scale(modelCircle, glm::vec3(circleBaseRadius));
			glUniform4f(uColor, 0, 0, 0, 1);
		}
		glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelCircle));
		circle.render();
		// Draw a cone with a random color
		modelCone = glm::scale(modelCone, glm::vec3(glm::vec2(coneRadius), 1));
		glUniform4f(uColor, cells[i].color.r, cells[i].color.g, cells[i].color.b, 1);
		glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelCone));
		cone.render();
	}
	glutSwapBuffers();
}

// Resize window callback
void reshape(int w, int h) {
	// Prevent divide by zero
	if (h == 0) {
		h = 1;
	}
	// Get aspect
	GLdouble aspect = (GLdouble)w / (GLdouble)h;
	// Set up and store viewport
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	viewport = glm::vec4(0, 0, (GLsizei)w, (GLsizei)h);
	// Set up orthographic projection view
	if (w >= h) {
		// Aspect >= 1, set width larger
		projectionMatrix = glm::ortho(-orthoScale * aspect, orthoScale * aspect, -orthoScale, orthoScale, 0.1, 100.0);
		// Update distribution to fit viewport
		viewDistX = std::uniform_real_distribution<double>(-orthoScale * aspect, orthoScale * aspect);
		viewDistY = std::uniform_real_distribution<double>(-orthoScale, orthoScale);
	} else {
		// Aspect < 1, set height larger
		projectionMatrix = glm::ortho(-orthoScale, orthoScale, -orthoScale / aspect, orthoScale / aspect, 0.1, 100.0);
		// Update distribution to fit viewport
		viewDistX = std::uniform_real_distribution<double>(-orthoScale, orthoScale);
		viewDistY = std::uniform_real_distribution<double>(-orthoScale / aspect, orthoScale / aspect);
	}
	// Set up the camera at position looking at the origin along the -z axis
	viewMatrix = glm::lookAt(glm::vec3(0.0), glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0));
	projViewMatrix = projectionMatrix * viewMatrix;
	// Send projection matrix * view matrix to shader
	glUniformMatrix4fv(projViewMatLoc, 1, GL_FALSE, glm::value_ptr(projViewMatrix));
}

// Mouse buttons callback
void mouse(int button, int state, int mouseX, int mouseY) {
	switch (button) {
	case GLUT_LEFT_BUTTON:
		if (state == GLUT_DOWN && selection < 0) {
			mouseToWorld(mouseX, mouseY);
			cells.push_back(cell(mouseWorldPos, randomColor(colorDist)));
		}
		break;
	case GLUT_RIGHT_BUTTON:
		if (selection >= 0) {
			cells.erase(cells.begin() + selection);
		}
		break;
	}
	// Redraw
	glutPostRedisplay();
}

// Mouse motion while pressing a button callback
void mouseMotion(int mouseX, int mouseY) {
	if (selection >= 0) {
		mouseToWorld(mouseX, mouseY);
		cells[selection].pos = mouseWorldPos;
	}
	// Redraw
	glutPostRedisplay();
}

// Mouse passive motion
void mousePassive(int mouseX, int mouseY) {
	mouseToWorld(mouseX, mouseY);
	// Redraw
	glutPostRedisplay();
}

// Keyboard buttons callback
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case KEY_CLEAR:
		cells.clear();
		break;
	case KEY_RANDOM:
		cells.clear();
		for (size_t i = 0; i < randomAmount; i++) {
			cells.push_back(cell(randomPos(viewDistX, viewDistY), randomColor(colorDist)));
		}
		break;
	}
	// Redraw
	glutPostRedisplay();
}

// Initialize OpenGL
void initOpenGL(int w, int h) {
	glEnable(GL_DEPTH_TEST); // Remove hidded surfaces
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // Color and depth for glClear
	glClearDepth(1.0f);
	// Initialize GLEW
	glewInit();
	// Get shader
	shader = createShader(parseFile("c.vert"), parseFile("c.frag"));
	glUseProgram(shader);
	uColor = glGetUniformLocation(shader, "uColor");
	projViewMatLoc = glGetUniformLocation(shader, "projViewMatrix");
	modelMatLoc = glGetUniformLocation(shader, "modelMatrix");
	// Create VBOs
	cone.build(coneHeight, coneDepth, 64);
	circle.build(0, coneDepth + coneHeight, 16);
}

void idle() {
	glutPostRedisplay();
}

// Program entry
int main(int argc, char **argv) {
	// Initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(vWidth, vHeight);
	// Create window in centre of screen
	glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - vWidth) / 2,
		(glutGet(GLUT_SCREEN_HEIGHT) - vHeight) / 2);
	glutCreateWindow("Voronoi Diagram Tool");
	// Initialize GL
	initOpenGL(vWidth, vHeight);
	// Register callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(mouseMotion);
	glutPassiveMotionFunc(mousePassive);
	glutKeyboardFunc(keyboard);
	if (enableFrameTimeCheck) {
		glutIdleFunc(idle);
	}
	// Set options
	glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	// Initialize cell data with default member
	cells.push_back(cell(glm::vec2(0, 0), randomColor(colorDist)));
	// Start event loop, never returns
	glutMainLoop();
	// Never returns but here for convention
	return 0;
}