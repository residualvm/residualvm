
#ifndef AGL_TGLMESH_H
#define AGL_TGLMESH_H

#include "common/array.h"

#include "math/vector3d.h"

#include "graphics/agl/mesh.h"
#include "graphics/agl/meshface.h"

namespace AGL {

class MeshFace;
class Texture;

class TGLMesh : public Mesh {
public:
	MeshFace *createFace();

	void draw(Texture *texture);

	bool calculate2DBoundingBox(Common::Rect *rect) const;
};

class TGLMeshFace : public MeshFace {
public:
	TGLMeshFace(TGLMesh *parent);

	void setNormal(float x, float y, float z);

	void vertex(int index);
	void texture(int index);
	void normal(int index);
	void color(int index);

	void draw(Texture *texture);

	TGLMesh *_parent;
	Math::Vector3d _normal;
	Common::Array<int> _vertices;
	Common::Array<int> _textures;
	Common::Array<int> _normals;
	Common::Array<int> _colors;

	bool _useColors;
	bool _useTexture;
};

}

#endif
