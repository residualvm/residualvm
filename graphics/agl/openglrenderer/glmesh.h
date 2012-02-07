
#ifndef AGL_GLMESH_H
#define AGL_GLMESH_H

#include "common/array.h"

#include "math/vector3d.h"

#include "graphics/agl/mesh.h"
#include "graphics/agl/meshface.h"

namespace AGL {

class MeshFace;
class Texture;

class GLMesh : public Mesh {
public:
	MeshFace *createFace();

	void draw(Texture *texture);

	bool calculate2DBoundingBox(Common::Rect *rect) const;
};

class GLMeshFace : public MeshFace {
public:
	GLMeshFace(GLMesh *parent);

	void setNormal(float x, float y, float z);

	void vertex(int index);
	void texture(int index);
	void normal(int index);
	void color(int index);

	void draw(Texture *texture);

	GLMesh *_parent;
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
