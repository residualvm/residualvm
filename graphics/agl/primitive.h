
#ifndef AGL_PRIMITIVE_H
#define AGL_PRIMITIVE_H

#include "common/array.h"
#include "common/hashmap.h"

#include "math/vector2d.h"

#include "graphics/color.h"

#include "graphics/agl/manager.h"

namespace Graphics {
class Color;
}

namespace AGL {

class Primitive {
public:
	Primitive();
	virtual ~Primitive();

	void begin(DrawMode mode);

	void vertex(float x, float y);
	void vertex(const Math::Vector2d &vertex);
	void color(const Graphics::Color &color);

	void newSubPolygon();

	virtual void end() { }

	void setGlobalColor(const Graphics::Color &color);

	void setVertex(uint sub, uint i, const Math::Vector2d &vertex);
	void setColor(uint sub, uint i, const Graphics::Color &c);

	virtual void draw(float x, float y) = 0;

protected:
	inline DrawMode getMode() const { return _mode; }
	inline uint getNumSubs() const { return _prims.size(); }
	inline uint getNumVertices(uint sub) const { return _prims[sub]._numVertices; }

	inline const Graphics::Color &getGlobalColor() const { return _globalColor; }
	inline bool useGlobalColor() const { return _useGlobalColor; }

	inline const float *getVertexPointer(uint sub) const { return _prims[sub]._vertices.begin(); }
	inline const byte *getColorPointer(uint sub) const { return _prims[sub]._colors.begin(); }

private:
	struct SubPrimitive {
		Common::Array<float> _vertices;
		Common::Array<byte> _colors;
		int _numVertices;
	};
	Common::Array<SubPrimitive> _prims;
	SubPrimitive *_currentPrim;
	DrawMode _mode;
	Graphics::Color _globalColor;
	bool _useGlobalColor;
};

}

#endif
