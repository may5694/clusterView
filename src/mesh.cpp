#include "mesh.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <QImage>
#include <iostream>
using namespace std;

Mesh::Mesh(QOpenGLWidget* glView, fs::path objPath) :
	init(false),
	vao(0), vbo(0), ibo(0), npts(0),
	worldMtx(1.0f) {

	// Throw if no context
	if (!glView || !glView->context())
		throw runtime_error("Mesh::Mesh(): context not initialized!");
	makeCurrent = [=]() { glView->makeCurrent(); };
	makeCurrent();

	// Get GL function pointers
	initializeOpenGLFunctions();

	// Load the mesh from file
	loadMesh(objPath);
	init = true;

	// Setup release of resources if context is destroyed
	QObject::connect(glView->context(), &QOpenGLContext::aboutToBeDestroyed,
		[this](){ cleanup(); });
}

Mesh::~Mesh() {
	// Release any held resources
	cleanup();
}

// Render the mesh geometry - assumes the context is already current!
void Mesh::draw() {
	// Prepare to draw
	glBindVertexArray(vao);

	// Bind the texture
	glBindTexture(GL_TEXTURE_2D, tex);

	// Draw the geometry
	glDrawElements(GL_TRIANGLES, npts, GL_UNSIGNED_INT, 0);

	// Cleanup
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

// Load geometry from an OBJ file
void Mesh::loadMesh(fs::path objPath) {
	makeCurrent();

	// Read OBJ model
	vector<Vertex> vertBuf;
	vector<uint32_t> indexBuf;
	fs::path texPath;
	readObj(objPath, vertBuf, indexBuf, texPath);
	name = objPath.filename().string();

	// Create OpenGL state
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Upload geometry to GPU
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertBuf.size() * sizeof(vertBuf[0]),
		vertBuf.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuf.size() * sizeof(indexBuf[0]),
		indexBuf.data(), GL_STATIC_DRAW);
	npts = indexBuf.size();

	// Specify vertex format
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)sizeof(glm::vec3));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)(sizeof(glm::vec3)*2));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
		sizeof(Vertex), (GLvoid*)(sizeof(glm::vec3)*2+sizeof(glm::vec2)));

	if (!texPath.empty()) {
		// Load the texture image from file
		QImage texImage(QString::fromStdString(texPath.string()));
		if (texImage.isNull())
			throw runtime_error("Mesh::loadMesh(): failed to read " + texPath.string());
		texImage = texImage.convertToFormat(QImage::Format_RGBA8888);
		texImage = texImage.mirrored(false, true);

		// Upload texture to GPU
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texImage.width(), texImage.height(), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, texImage.constBits());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// Clean up state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh::readObj(fs::path objPath, vector<Vertex>& vertBuf, vector<uint32_t>& indexBuf,
	fs::path& texPath) {

	// Load the obj model
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, NULL, NULL,
		objPath.string().c_str(), objPath.parent_path().string().c_str());

	if (!loaded) throw runtime_error("Mesh::readObj(): failed to load " + objPath.string());

	// Calculate bounding box
	glm::vec3 minPos(FLT_MAX), maxPos(-FLT_MAX);

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		size_t idx_offset = 0;
		// Loop over faces
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Add face to index buffer
			for (size_t v = 2; v < fv; v++) {
				indexBuf.push_back(vertBuf.size());
				indexBuf.push_back(vertBuf.size() + v - 1);
				indexBuf.push_back(vertBuf.size() + v - 0);
			}

			// We might need to calculate the normal
			bool calcNorm = false;
			vector<int> calcNormVerts;

			// Add vertex attributes
			for (size_t v = 0; v < fv; v++) {
				tinyobj::index_t idx = shapes[s].mesh.indices[idx_offset + v];

				Vertex vert;
				// Set position
				vert.pos = {
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2]};

				// Update BBOX
				minPos = glm::min(minPos, vert.pos);
				maxPos = glm::max(maxPos, vert.pos);

				// Set normal if used
				if (idx.normal_index >= 0) {
					vert.norm = {
						attrib.normals[3 * idx.normal_index + 0],
						attrib.normals[3 * idx.normal_index + 1],
						attrib.normals[3 * idx.normal_index + 2]};
				} else {
					calcNorm = true;
					calcNormVerts.push_back(vertBuf.size());
				}

				// Set texture coords if used
				if (idx.texcoord_index >= 0) {
					vert.tc = {
						attrib.texcoords[2 * idx.texcoord_index + 0],
						attrib.texcoords[2 * idx.texcoord_index + 1]};
				} else
					vert.tc = { -1, -1 };

				// Set color based on material
				int material_id = shapes[s].mesh.material_ids[f];
				if (material_id >= 0) {
					vert.col = {
						materials[material_id].diffuse[0],
						materials[material_id].diffuse[1],
						materials[material_id].diffuse[2]};
				} else
					vert.col = { 1, 0, 0 };

				// Add vertex to buffer
				vertBuf.push_back(vert);
			}

			if (calcNorm) {
				// Calculate the normal from the first 3 verts
				glm::vec3 ab = vertBuf[calcNormVerts[1]].pos - vertBuf[calcNormVerts[0]].pos;
				glm::vec3 ac = vertBuf[calcNormVerts[2]].pos - vertBuf[calcNormVerts[0]].pos;
				glm::vec3 norm = glm::normalize(glm::cross(ab, ac));
				// Set normal for all verts in face
				for (auto i : calcNormVerts)
					vertBuf[i].norm = norm;
			}

			idx_offset += fv;
		}
	}

	// Look for any material with texture
	for (size_t m = 0; m < materials.size(); m++) {
		if (!materials[m].diffuse_texname.empty()) {
			texPath = objPath.parent_path() / materials[m].diffuse_texname;
			break;
		}
	}

	// Update world matrix transform
	worldMtx[3] = glm::vec4(-(minPos + maxPos) / glm::vec3(2.0f), 1.0);
}

// Release any OpenGL resources
void Mesh::cleanup() {
	// Don't try to cleanup if we're not init
	if (!init) return;

	// Make sure context is current
	makeCurrent();

	// Release OpenGL state
	if (vao) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	if (vbo) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if (ibo) {
		glDeleteBuffers(1, &ibo);
		ibo = 0;
	}
	npts = 0;
	if (tex) {
		glDeleteTextures(1, &tex);
		tex = 0;
	}

	// Prevent redundant cleanups
	init = false;
}
