#ifndef GLVIEW_HPP
#define GLVIEW_HPP

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "mesh.hpp"

class GLView : public QOpenGLWidget, protected QOpenGLFunctions_4_5_Core {
	Q_OBJECT
public:
	GLView(QWidget* parent = NULL);
	~GLView();

	QSize sizeHint() const { return { 800, 600 }; }

	// Set the mesh to draw
	void setMesh(const std::shared_ptr<Mesh>& mesh);

signals:
	void glInitialized();

public slots:
	void resetView() { initView(); update(); }

protected:
	// OpenGL callbacks
	void initializeGL();
	void resizeGL(int w, int h);
	void paintGL();

	// Event handlers
	void mousePressEvent(QMouseEvent* e);
	void mouseReleaseEvent(QMouseEvent* e);
	void mouseMoveEvent(QMouseEvent* e);
	void wheelEvent(QWheelEvent* e);

private:
	// Initialization methods
	void initShaders();
	void initView();
	void cleanup();

	// Utility methods
	GLuint compileShader(GLenum type, std::string source);
	GLuint linkProgram(std::vector<GLuint> shaders);

	// Mesh to draw
	std::shared_ptr<Mesh> mesh;

	// OpenGL state
	GLuint shader;		// Shader program
	static const GLuint viewXformLoc = 0;	// View matrix location
	static const GLuint projXformLoc = 1;	// Proj matrix location
	static const GLuint posLoc = 0;		// Position attrib location
	static const GLuint normLoc = 1;	// Normal attrib location
	static const GLuint tcLoc = 2;		// Texture coord attrib location
	static const GLuint colLoc = 3;		// Color attrib location
	// Shader sources
	static const std::string vshader;
	static const std::string fshader;

	// View state
	glm::mat4 worldMtx;			// Any pre-view transformation (centering, etc.)
	glm::mat4 viewMtx;			// Rotation, translation, and scale
	glm::mat4 incrViewMtx;		// Incremental view changes
	glm::mat4 projMtx;			// Orthographic projection
	bool rotating;				// Whether user is rotating
	bool zooming;				// Whether user is zooming
	QPoint clickedPt;			// Initial clicked point
};

#endif
