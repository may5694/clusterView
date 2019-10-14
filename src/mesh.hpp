#ifndef MESH_HPP
#define MESH_HPP

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_5_Core>
#include <vector>
#include <functional>
#include <filesystem>
#include <glm/glm.hpp>
namespace fs = std::filesystem;

// Mesh of triangles
class Mesh : public QOpenGLFunctions_4_5_Core {
public:
	// Constructor / destructor
	Mesh(QOpenGLWidget* glView, fs::path objPath);
	~Mesh();
	// Disable copy and move
	Mesh(const Mesh& other) = delete;
	Mesh(Mesh&& other) = delete;
	Mesh& operator=(const Mesh& other) = delete;
	Mesh& operator=(Mesh&& other) = delete;

	// Draw the mesh
	void draw();

	// Public state
	glm::mat4 worldMtx;		// Model to world matrix
	std::string name;		// Name of the mesh

private:
	struct Vertex;

	// Initialization methods
	void loadMesh(fs::path objPath);
	void readObj(fs::path objPath, std::vector<Vertex>& vertBuf,
		std::vector<uint32_t>& indexBuf, fs::path& texPath);
	void cleanup();

	// OpenGL state
	bool init;
	std::function<void()> makeCurrent;	// Make context current
	GLuint vao;		// Vertex array object
	GLuint vbo;		// Vertex buffer
	GLuint ibo;		// Index buffer
	GLsizei npts;	// Number of indices to draw
	GLuint tex;		// Texture

	// Vertex structure
	struct Vertex {
		glm::vec3 pos;		// Position
		glm::vec3 norm;		// Normal
		glm::vec2 tc;		// Texture coord
		glm::vec3 col;		// Color
	};
};

#endif
