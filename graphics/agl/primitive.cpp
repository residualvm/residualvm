
#include "graphics/agl/primitive.h"

namespace AGL {

Primitive::Primitive()
	: _useGlobalColor(false) {
	newSubPolygon();
}

Primitive::~Primitive() {

}

void Primitive::begin(DrawMode mode) {
	_mode = mode;
}

void Primitive::vertex(float x, float y) {
	_currentPrim->_vertices.push_back(x);
	_currentPrim->_vertices.push_back(y);
	++_currentPrim->_numVertices;
}

void Primitive::vertex(const Math::Vector2d &v) {
	_currentPrim->_vertices.push_back(v.getX());
	_currentPrim->_vertices.push_back(v.getY());
	++_currentPrim->_numVertices;
}

void Primitive::color(const Graphics::Color &c) {
	_currentPrim->_colors.push_back(c.getRed());
	_currentPrim->_colors.push_back(c.getGreen());
	_currentPrim->_colors.push_back(c.getBlue());
	_currentPrim->_colors.push_back(c.getAlpha());
}

void Primitive::newSubPolygon() {
	_prims.push_back(SubPrimitive());
	_currentPrim = &_prims.back();
	_currentPrim->_numVertices = 0;
}

void Primitive::setGlobalColor(const Graphics::Color &c) {
	_globalColor = c;
	_useGlobalColor = true;
}

void Primitive::setVertex(uint sub, uint i, const Math::Vector2d &v) {
	memcpy(_prims[sub]._vertices.begin() + 2 * i, v.getData(),  2 * sizeof(float));
}

void Primitive::setColor(uint sub, uint i, const Graphics::Color &c) {
	memcpy(_prims[sub]._colors.begin() + 4 * i, c.getData(), 4);
}

}
