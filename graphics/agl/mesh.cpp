
#include "graphics/agl/mesh.h"

namespace AGL {

Mesh::Mesh()
	: _drawMode(Triangles) {
}

void Mesh::pushVertex(float x, float y, float z) {
	_vertices.push_back(x);
	_vertices.push_back(y);
	_vertices.push_back(z);
}

void Mesh::pushTexVertex(float u, float v) {
	_textures.push_back(u);
	_textures.push_back(v);
}

void Mesh::pushNormal(float x, float y, float z) {
	_normals.push_back(x);
	_normals.push_back(y);
	_normals.push_back(z);
}

void Mesh::pushColor(float r, float g, float b, float a) {
	_colors.push_back(r);
	_colors.push_back(g);
	_colors.push_back(b);
	_colors.push_back(a);
}

void Mesh::setUseAbsoluteTexCoords(bool useAbs) {
	_useAbsTexCoords = useAbs;
}

void Mesh::setDrawMode(DrawMode mode) {
	_drawMode = mode;
}

float *Mesh::getVerticesArray() {
	return _vertices.begin();
}

float *Mesh::getTexCoordsArray() {
	return _textures.begin();
}

float *Mesh::getNormalsArray() {
	return _normals.begin();
}

float *Mesh::getColorsArray() {
	return _colors.begin();
}

void Mesh::setVerticesArray(float *array, int n) {
	_vertices.clear(),
	_vertices = Common::Array<float>(array, n);
}

}
