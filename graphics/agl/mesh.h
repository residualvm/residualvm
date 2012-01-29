
#ifndef AGL_MESH_H
#define AGL_MESH_H

#include "common/list.h"

namespace AGL {

class MeshFace;
class Texture;

class Mesh {
public:
	typedef Common::List<MeshFace *> Faces;

	virtual ~Mesh() { }

	virtual MeshFace *createFace() = 0;

	virtual void prepare(uint size) { }

	virtual void vertex(float x, float y, float z) = 0;
	virtual void texture(float u, float v) = 0;
	virtual void normal(float x, float y, float z) = 0;

	virtual void draw(Texture *texture) = 0;

	inline void setUseAbsoluteTexCoords(bool useAbs) { _useAbsTexCoords = useAbs; }
	inline bool getUseAbsoluteTexCoords() const { return _useAbsTexCoords; }

// 	Faces::iterator facesBegin() const { return _faces.begin(); }
// 	Faces::iterator facesEnd() const { return _faces.end(); }

protected:
	Mesh() { }

	Faces _faces;
	bool _useAbsTexCoords;
};

}

#endif
