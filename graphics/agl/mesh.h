
#ifndef AGL_MESH_H
#define AGL_MESH_H

#include "common/list.h"

namespace Common {
class Rect;
}

namespace AGL {

class MeshFace;
class Texture;

class Mesh {
public:
	typedef Common::List<MeshFace *> Faces;

	virtual ~Mesh() { }

	virtual MeshFace *createFace() = 0;

	virtual void pushVertex(float x, float y, float z) = 0;
	virtual void pushTexVertex(float u, float v) = 0;
	virtual void pushNormal(float x, float y, float z) = 0;
	virtual void pushColor(float r, float g, float b, float a) = 0;

	virtual void draw(Texture *texture) = 0;

	inline void setUseAbsoluteTexCoords(bool useAbs) { _useAbsTexCoords = useAbs; }
	inline bool getUseAbsoluteTexCoords() const { return _useAbsTexCoords; }

	virtual bool calculate2DBoundingBox(Common::Rect *rect) const = 0;

// 	Faces::iterator facesBegin() const { return _faces.begin(); }
// 	Faces::iterator facesEnd() const { return _faces.end(); }

protected:
	Mesh() { }

	Faces _faces;
	bool _useAbsTexCoords;
};

}

#endif
