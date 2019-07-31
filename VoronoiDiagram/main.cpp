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

namespace VoronoiTool {
	class Constants {
	public:
		static constexpr double PI = 3.14159265358979323846264338327950288;
	};
	Constants constants;

	class Input {
	public:
		static constexpr char clear = 'c';
		static constexpr char random = 'r';
		static constexpr char grow = 'g';
		static constexpr char minRad = 'm';
		static constexpr char maxRad = 'n';
	};
	Input input;

	class Cell {
	public:
		glm::vec2 pos;
		glm::vec3 color;
		Cell(glm::vec2 t_pos, glm::vec3 t_color) : pos(t_pos), color(t_color) {}
	};
	std::vector<Cell> cells;

	class CircleFan {
	private:
		GLuint vboID, iboID;
		std::vector<GLfloat> vertices;
		std::vector<GLuint> indices;
	public:
		~CircleFan() {
			glDeleteBuffers(1, &vboID);
			glDeleteBuffers(1, &iboID);
		}
		void build(double centerHeight, double depth, int points) {
			// Generate bottom ring points
			// Bottom not filled in because it is never visible
			double delta = 2 * constants.PI / points;
			double angle = 0;
			for (size_t i = 0; i < points; i++) {
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
			for (size_t i = 0; i < points - 1; i++) {
				indices.push_back(points);
				indices.push_back(i);
				indices.push_back(i + 1);
			}
			// Last connection
			indices.push_back(points);
			indices.push_back(points - 1);
			indices.push_back(0);
			// Generate IDs for VBO and IBO
			glGenBuffers(1, &vboID);
			glGenBuffers(1, &iboID);
			// Copy vertex data
			glBindBuffer(GL_ARRAY_BUFFER, vboID);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), 
				vertices.data(), GL_DYNAMIC_DRAW);
			// Copy indices
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), 
				indices.data(), GL_DYNAMIC_DRAW);
			// Deselect
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		void render() const {
			glBindBuffer(GL_ARRAY_BUFFER, vboID);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
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
	double coneRadius = 2.2, coneHeight = 1, coneDepth = -1, minConeRadius = 0.03, maxConeRadius = 2.2;
	double circleBaseRadius = 0.01, circleHoverRadius = 0.025;
	unsigned int coneRes = 64, circleRes = 16;
	int selection = -1;
	// Input flags
	bool mouseDown = false, growing = false;
	// Matrices and viewport
	glm::mat4 identity = glm::mat4(1.0);
	glm::mat4 projectionMatrix, viewMatrix;
	glm::vec4 viewport;
	// Mouse position in the world, init as non-zero
	glm::dvec2 mouseWorldPos(1, 1);
	// Random Number Generation
	std::uniform_real_distribution<double> colorDist(0.0, 1.0), viewDistX, viewDistY;
	std::random_device rd;
	std::mt19937 gen(rd());
	unsigned int randomAmount = 40;
	// Shader data
	unsigned int shader;
	GLint colorLoc;
	GLint projViewMatLoc;
	GLint modelMatLoc;
	// Grow
	int growStartTime;
	double growStartRadius;
	double growDuration = 7000; // In milliseconds

	template <typename T>
	T lerp(T a, T b, T t) {
		return a + t * (b - a);
	}

	// Random vec3 color
	glm::vec3 randomColor(std::uniform_real_distribution<double> range) {
		return glm::vec3(range(gen), range(gen), range(gen));
	}
	// Random vec2 position
	glm::vec2 randomPos(std::uniform_real_distribution<double> x, 
		std::uniform_real_distribution<double> y) {
		return glm::vec2(x(gen), y(gen));
	}

	// Get the worldspace coordinate of the mouse cursor position
	void mouseToWorld(int mouseX, int mouseY) {
		mouseWorldPos = glm::unProject(glm::vec3((GLfloat)mouseX, 
			(GLfloat)(viewport.w - mouseY), 0), identity, projectionMatrix, viewport);
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
		// Clear screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Growing
		if (growing) {
			double delta = glutGet(GLUT_ELAPSED_TIME) - growStartTime;
			delta /= growDuration;
			if (delta > 1) {
				delta = 1;
				growing = false;
			}
			coneRadius = lerp(growStartRadius, maxConeRadius, delta);
		}
		// Reset selection if we're not clicking
		if (!mouseDown) {
			selection = -1;
		}
		// Draw each cell
		for (size_t i = 0; i < cells.size(); i++) {
			// Draw a circle at the centre of the cone
			glm::mat4 modelCircle = glm::translate(identity, glm::vec3(cells[i].pos, -1));
			glm::mat4 modelCone = modelCircle;
			// If we're already selecting a cell
			if (mouseDown && i == selection) {
				modelCircle = glm::scale(modelCircle, glm::vec3(glm::vec2(circleHoverRadius), 1));
				glUniform4f(colorLoc, 1, 1, 1, 1);
			} else if (selection < 0 && pointInCircle(cells[i].pos, circleHoverRadius, mouseWorldPos)) {
				// If we're not selecting a cell but we can, select it
				selection = i;
				modelCircle = glm::scale(modelCircle, glm::vec3(glm::vec2(circleHoverRadius), 1));
				glUniform4f(colorLoc, 1, 1, 1, 1);
			} else {
				// If we're not selecting a cell
				modelCircle = glm::scale(modelCircle, glm::vec3(circleBaseRadius));
				glUniform4f(colorLoc, 0, 0, 0, 1);
			}
			glUniformMatrix4fv(modelMatLoc, 1, GL_FALSE, glm::value_ptr(modelCircle));
			circle.render();
			// Draw a cone with a random color
			modelCone = glm::scale(modelCone, glm::vec3(glm::vec2(coneRadius), 1));
			glUniform4f(colorLoc, cells[i].color.r, cells[i].color.g, cells[i].color.b, 1);
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
			projectionMatrix = glm::ortho(-orthoScale * aspect, orthoScale * aspect, 
				-orthoScale, orthoScale, 0.1, 100.0);
			// Update distribution to fit viewport
			viewDistX = std::uniform_real_distribution<double>(-orthoScale * aspect, orthoScale * aspect);
			viewDistY = std::uniform_real_distribution<double>(-orthoScale, orthoScale);
		} else {
			// Aspect < 1, set height larger
			projectionMatrix = glm::ortho(-orthoScale, orthoScale, 
				-orthoScale / aspect, orthoScale / aspect, 0.1, 100.0);
			// Update distribution to fit viewport
			viewDistX = std::uniform_real_distribution<double>(-orthoScale, orthoScale);
			viewDistY = std::uniform_real_distribution<double>(-orthoScale / aspect, orthoScale / aspect);
		}
		// Set up the camera at position looking at the origin along the -z axis
		viewMatrix = glm::lookAt(glm::vec3(0.0), glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, 1.0, 0.0));
		// Send projection matrix * view matrix to shader
		glUniformMatrix4fv(projViewMatLoc, 1, GL_FALSE, glm::value_ptr(projectionMatrix * viewMatrix));
	}

	// Mouse buttons callback
	void mouse(int button, int state, int mouseX, int mouseY) {
		switch (button) {
		case GLUT_LEFT_BUTTON:
			if (state == GLUT_DOWN) {
				if (selection < 0) {
					mouseToWorld(mouseX, mouseY);
					cells.push_back(Cell(mouseWorldPos, randomColor(colorDist)));
				}
				mouseDown = true;
			} else {
				mouseDown = false;
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
		if (mouseDown && selection >= 0) {
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
		case input.clear:
			cells.clear();
			break;
		case input.random:
			cells.clear();
			for (size_t i = 0; i < randomAmount; i++) {
				cells.push_back(Cell(randomPos(viewDistX, viewDistY), randomColor(colorDist)));
			}
			break;
		case input.minRad:
			coneRadius = minConeRadius;
			growing = false;
			break;
		case input.maxRad:
			coneRadius = maxConeRadius;
			growing = false;
			break;
		case input.grow:
			growing = !growing;
			if (growing) {
				growStartRadius = coneRadius;
				growStartTime = glutGet(GLUT_ELAPSED_TIME);
			}
			break;
		}
		// Redraw
		glutPostRedisplay();
	}

	void idle() {
		glutPostRedisplay();
	}

	// Initialize OpenGL
	void initOpenGL() {
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE_ARB);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClearDepth(1.0f);
		// Initialize GLEW
		glewInit();
		// Get shader
		shader = createShader(parseFile("c.vert"), parseFile("c.frag"));
		glUseProgram(shader);
		colorLoc = glGetUniformLocation(shader, "color");
		projViewMatLoc = glGetUniformLocation(shader, "projViewMatrix");
		modelMatLoc = glGetUniformLocation(shader, "modelMatrix");
		// Create VBOs
		cone.build(coneHeight, coneDepth, coneRes);
		circle.build(0, coneDepth + coneHeight, circleRes);
	}

	void initGLUT(int *argc, char **argv) {
		// Initialize GLUT
		glutInit(argc, argv);
		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
		glutInitWindowSize(vWidth, vHeight);
		// Create window in centre of screen
		glutInitWindowPosition((glutGet(GLUT_SCREEN_WIDTH) - vWidth) / 2,
			(glutGet(GLUT_SCREEN_HEIGHT) - vHeight) / 2);
		glutCreateWindow("Voronoi Diagram Tool");
		// Register callbacks
		glutDisplayFunc(display);
		glutReshapeFunc(reshape);
		glutMouseFunc(mouse);
		glutMotionFunc(mouseMotion);
		glutPassiveMotionFunc(mousePassive);
		glutKeyboardFunc(keyboard);
		glutIdleFunc(idle);
		// Set options
		glutSetKeyRepeat(GLUT_KEY_REPEAT_OFF);
	}

	void run(int *argc, char **argv) {
		initGLUT(argc, argv);
		initOpenGL();
		// Initialize cell data with default member
		cells.push_back(Cell(glm::vec2(0, 0), randomColor(colorDist)));
		// Start event loop, never returns
		glutMainLoop();
	}

}

// Program entry
int main(int argc, char **argv) {
	VoronoiTool::run(&argc, argv);
	return 0;
}