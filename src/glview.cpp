#include "glview.hpp"
#include <QApplication>
#include <QTimer>
#include <QMouseEvent>
#include <iostream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;

GLView::GLView(QWidget* parent) : QOpenGLWidget(parent),
	init(false), shader(0),
	viewMtx(1.0f), incrViewMtx(1.0f), projMtx(1.0f),
	rotating(false), zooming(false) {

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	setFocusPolicy(Qt::StrongFocus);
}

GLView::~GLView() {
	// Release any held resources upon destruction
	cleanup();
}

// Set which mesh to draw
void GLView::setMesh(const shared_ptr<Mesh>& mesh) {
	// Update the mesh pointer and redraw
	this->mesh = mesh;
	update();
}

// Called once the context is initialized
void GLView::initializeGL() {
	// Initialize OpenGL functions
	initializeOpenGLFunctions();

	// Setup release of resources before context is destroyed
	connect(context(), &QOpenGLContext::aboutToBeDestroyed,
		this, &GLView::cleanup);

	// Enable OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Set background color
	glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

	// Attempt to initialize OpenGL state
	try {
		initShaders();
		initView();

	// If initialization fails, print error and exit app
	} catch (const exception& e) {
		cerr << e.what() << endl;
		// Exit once the event loop starts
		QTimer::singleShot(0, QApplication::instance(), &QApplication::quit);
	}

	// Signal to app that OpenGL is initialized
	init = true;
	emit glInitialized();
}

// Called when the widget is resized
void GLView::resizeGL(int w, int h) {
	// Resize the viewport
	glViewport(0, 0, w, h);

	// Fix aspect ratio
	projMtx[0][0] = min((float)h / (float)w, 1.0f);
	projMtx[1][1] = min((float)w / (float)h, 1.0f);
	// Flip Z for correct depth testing
	projMtx[2][2] = -0.01f;
}

// Called when the widget needs repainting
void GLView::paintGL() {
	// Clear the buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Don't draw if we don't have a mesh
	if (!mesh) return;

	// Setup for drawing
	glUseProgram(shader);

	// Set transformation matrices
	glm::mat4 viewXform = incrViewMtx * viewMtx * mesh->worldMtx;
	glm::mat4 projXform = projMtx;
	glUniformMatrix4fv(viewXformLoc, 1, GL_FALSE, glm::value_ptr(viewXform));
	glUniformMatrix4fv(projXformLoc, 1, GL_FALSE, glm::value_ptr(projXform));

	// Draw the mesh
	mesh->draw();

	// Clean up
	glUseProgram(0);
}

// Called when a mouse button is clicked
void GLView::mousePressEvent(QMouseEvent* e) {
	// Don't do anything if manipulating view
	if (rotating || zooming) return;

	if (e->button() == Qt::LeftButton) {
		// Get inital point
		clickedPt = e->pos();

		// Turn on rotation
		rotating = true;

	} else if (e->button() == Qt::RightButton) {
		// Get initial point
		clickedPt = e->pos();

		// Turn on zooming
		zooming = true;
	}
}

// Called when a mouse button is released
void GLView::mouseReleaseEvent(QMouseEvent* e) {
	if (!rotating && !zooming) return;

	if (e->button() == Qt::LeftButton) {
		// Turn off rotation
		rotating = false;

		// Apply current rotation to view matrix
		viewMtx = incrViewMtx * viewMtx;
		incrViewMtx = glm::mat4(1.0f);

		update();

	} else if (e->button() == Qt::RightButton) {
		// Turn off zoom
		zooming = false;

		// Apply current zoom level to view matrix
		viewMtx = incrViewMtx * viewMtx;
		incrViewMtx = glm::mat4(1.0f);

		update();
	}
}

// Called when the mouse moves
void GLView::mouseMoveEvent(QMouseEvent* e) {
	if (!rotating && !zooming) return;

	// No view manipulation if mouse at clicked point
	QPoint diffPt = e->pos() - clickedPt;
	if (diffPt.x() == 0 && diffPt.y() == 0)
		incrViewMtx = glm::mat4(1.0);

	else if (rotating) {
		// Rotate about world Z
		glm::vec3 axisZ(0.0, 0.0, 1.0);
		axisZ = glm::vec3(viewMtx * glm::vec4(axisZ, 0.0));
		axisZ = glm::normalize(axisZ);
		// Flip if upside-down
		if (glm::dot(axisZ, glm::vec3(0.0, 1.0, 0.0)) < 0.0f)
			axisZ = -axisZ;
		float angleZ = (float)diffPt.x() / width() * 4.0f * M_PI;
		incrViewMtx = glm::rotate(glm::mat4(1.0), angleZ, axisZ);

		// Rotate about view X
		glm::vec3 axisX(1.0, 0.0, 0.0);
		float angleX = (float)diffPt.y() / height() * 2.0f * M_PI;
		incrViewMtx = glm::rotate(glm::mat4(1.0), angleX, axisX) * incrViewMtx;

	} else if (zooming) {
		// Scale by 2^-dy
		float scale = pow(2.0f, -diffPt.y() / 100.0f);
		incrViewMtx = glm::scale(glm::mat4(1.0), glm::vec3(scale));
	}

	update();
}

// Called when the mouse wheel moves
void GLView::wheelEvent(QWheelEvent* e) {
	// Scale the view matrix
	float scale = pow(2.0f, e->angleDelta().y() / 400.0f);
	glm::mat4 scaleMtx = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
	viewMtx = scaleMtx * viewMtx;

	update();
}

void GLView::initShaders() {
	// Compile and link shaders
	vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, vshader));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, fshader));
	shader = linkProgram(shaders);
	// Clean up shader sources
	for (auto s : shaders)
		glDeleteShader(s);
	shaders.clear();

	glUseProgram(shader);
	GLuint samplerLoc = glGetUniformLocation(shader, "tex");
	glUniform1i(samplerLoc, 0);
	glUseProgram(0);
}

// Initializes the view matrix
void GLView::initView() {
	// Zoom out
	glm::mat4 scaleMtx(0.01f);
	scaleMtx[3][3] = 1.0f;

	// Look along 1,1,-1 (right, forward, down)
	glm::mat4 rotMtx(1.0f);
	glm::vec3 lookDir = glm::normalize(glm::vec3(1.0f, 1.0f, -1.0f));
	glm::vec3 upDir(0.0f, 0.0f, 1.0f);	// +z is up
	glm::vec3 rightDir = glm::normalize(glm::cross(lookDir, upDir));
	upDir = glm::normalize(glm::cross(rightDir, lookDir));
	rotMtx[0] = glm::vec4(rightDir, 0.0);
	rotMtx[1] = glm::vec4(upDir, 0.0);
	rotMtx[2] = glm::vec4(-lookDir, 0.0);
	rotMtx = glm::transpose(rotMtx);

	// Combine scale -> rotate
	viewMtx = rotMtx * scaleMtx;
}

// Release any OpenGL resources held
void GLView::cleanup() {
	makeCurrent();
	// Delete shader program
	if (shader) {
		glDeleteProgram(shader);
		shader = 0;
	}
	init = false;
}

// Compiles and returns the shader given by the source string.
// Throws an exception if compilation fails.
GLuint GLView::compileShader(GLenum type, string source) {
	const char* source_cstr = source.c_str();
	GLint length = source.length();

	// Compile the shader
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source_cstr, &length);
	glCompileShader(shader);

	// Make sure compilation succeeded
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		// Compilation failed, get the info log
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		vector<GLchar> logText(logLength);
		glGetShaderInfoLog(shader, logLength, NULL, logText.data());

		// Construct an error message with the compile log
		stringstream ss;
		string typeStr = "";
		switch (type) {
		case GL_VERTEX_SHADER:
			typeStr = "vertex"; break;
		case GL_FRAGMENT_SHADER:
			typeStr = "fragment"; break;
		default:
			typeStr = "unknown"; break;
		}
		ss << "Error compiling " + typeStr + " shader!" << endl << endl << logText.data() << endl;

		// Cleanup shader and throw an exception
		glDeleteShader(shader);
		throw runtime_error(ss.str());
	}

	return shader;
}

// Links together the shader objects given in the vector argument.
// Throws an exception if linking fails.
GLuint GLView::linkProgram(vector<GLuint> shaders) {
	GLuint program = glCreateProgram();

	// Attach the shaders and link the program
	for (auto s : shaders)
		glAttachShader(program, s);
	glLinkProgram(program);

	// Detach shaders
	for (auto s : shaders)
		glDetachShader(program, s);

	// Make sure link succeeded
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		// Link failed, get the info log
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		vector<GLchar> logText(logLength);
		glGetProgramInfoLog(program, logLength, NULL, logText.data());

		// Construct an error message with the compile log
		stringstream ss;
		ss << "Error linking program!" << endl << endl << logText.data() << endl;

		// Cleanup program and throw an exception
		glDeleteProgram(program);
		throw runtime_error(ss.str());
	}

	return program;
}

// Vertex shader source
const string GLView::vshader = R"(
#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tc;
layout(location = 3) in vec3 col;

layout(location = 0) uniform mat4 viewXform;
layout(location = 1) uniform mat4 projXform;

smooth out vec2 fragTC;
smooth out vec3 fragCol;

const vec3 lightDir = normalize(vec3(3.0, -1.0, -10.0));

void main() {
	gl_Position = projXform * viewXform * vec4(pos, 1.0);
	vec3 viewNorm = normalize(vec3(viewXform * vec4(norm, 0.0)));
	fragTC = tc;
	fragCol = col * max(dot(-lightDir, viewNorm), 0.4);
})";

// Fragment shader source
const string GLView::fshader = R"(
#version 450

smooth in vec2 fragTC;
smooth in vec3 fragCol;

uniform sampler2D tex;

out vec4 outCol;

void main() {
	if (fragTC.x < 0 && fragTC.y < 0)
		outCol = vec4(fragCol, 1.0);
	else
		outCol = vec4(fragCol, 1.0) * texture(tex, fragTC);
})";
