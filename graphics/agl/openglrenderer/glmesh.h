
#ifndef AGL_GLMESH_H
#define AGL_GLMESH_H

#include "math/vector3d.h"

#include "graphics/agl/mesh.h"
#include "graphics/agl/meshface.h"

namespace AGL {

class MeshFace;
class Texture;

class GLMesh : public Mesh {
public:
	void prepare(uint size);
	void vertex(float x, float y, float z);
	void texture(float u, float v);
	void normal(float x, float y, float z);

	MeshFace *createFace();

	void draw(Texture *texture);

	float *_vertices;
	float *_textures;
	float *_normals;
	int _i;
};

class GLMeshFace : public MeshFace {
public:
	GLMeshFace(GLMesh *parent);

	void prepare(uint size);
	void setNormal(float x, float y, float z);

	void vertex(int index);
	void texture(int index);
	void normal(int index);
	void vertex(float x, float y, float z);
	void texture(float u, float v);
	void normal(float x, float y, float z);

	void draw(Texture *texture);

	GLMesh *_parent;
	Math::Vector3d _normal;
// 	int *_vertices;
// 	int *_textures;
// 	int *_normals;
	float *_vertices;
	float *_textures;
	float *_normals;
	int _i;
};

}

#endif
