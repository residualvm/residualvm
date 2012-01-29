
#ifndef AGL_PRIMITIVE_H
#define AGL_PRIMITIVE_H

#include "common/array.h"
#include "common/hashmap.h"

#include "math/vector2d.h"

#include "graphics/color.h"

namespace Graphics {
class Color;
}

namespace AGL {

class Primitive {
public:
	enum Mode {
		Points,
		Lines,
		LineLoop,
		Quads
	};

	Primitive();
	virtual ~Primitive();

	void begin(Mode mode);

	void vertex(const Math::Vector2d &vertex);
	void color(const Graphics::Color &color);

	void breakPolygon();

	virtual void end() { }

	void setColor(uint i, const Graphics::Color &c);

	virtual void setup() { }
	virtual void draw(float x, float y) = 0;

	inline Mode getMode() const { return _mode; }
	inline uint getNumVertices() const { return _numVertices; }
	inline const Math::Vector2d &getVertex(int i) const { return _vertices[i]; }
	inline const Graphics::Color &getColor(int i) const { return _colors[i]; }
	inline bool breaksAt(uint i) const { return _breaks.contains(i); }

private:
	Mode _mode;
	uint _numVertices;
	Common::Array<Math::Vector2d> _vertices;
	Common::Array<Graphics::Color> _colors;

	Common::HashMap<int, bool> _breaks;
};

}

#endif
