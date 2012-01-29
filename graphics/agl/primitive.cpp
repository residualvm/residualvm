
#include "graphics/agl/primitive.h"

namespace AGL {

Primitive::Primitive()
	: _numVertices(0) {

}

Primitive::~Primitive() {

}

void Primitive::begin(Mode mode) {
	_mode = mode;
}

void Primitive::vertex(const Math::Vector2d &v) {
	_vertices.push_back(v);
	++_numVertices;
}

void Primitive::color(const Graphics::Color &c) {
	_colors.push_back(c);
}

void Primitive::breakPolygon() {
	_breaks[_numVertices - 1] = true;
}

void Primitive::setColor(uint i, const Graphics::Color &color) {
	_colors[i] = color;
}

}
