
#ifndef AGL_MESHFACE_H
#define AGL_MESHFACE_H

namespace AGL {

class Texture;
class Mesh;

class MeshFace {
public:
	virtual ~MeshFace() { }

	virtual void prepare(uint size) { }
	virtual void setNormal(float x, float y, float z) = 0;

	virtual void vertex(int index) = 0;
	virtual void texture(int index) = 0;
	virtual void normal(int index) = 0;
	virtual void vertex(float x, float y, float z) = 0;
	virtual void texture(float u, float v) = 0;
	virtual void normal(float x, float y, float z) = 0;

	virtual void draw(Texture *texture) = 0;

protected:
	MeshFace(Mesh *parent) { }

};

}

#endif
