
#ifndef AGL_MESHFACE_H
#define AGL_MESHFACE_H

namespace AGL {

class Texture;
class Mesh;

class MeshFace {
public:
	virtual ~MeshFace() { }

	virtual void setNormal(float x, float y, float z) = 0;
	inline void setNormal(const Math::Vector3d &n) { setNormal(n.x(), n.y(), n.z()); }

	virtual void vertex(int index) = 0;
	virtual void texture(int index) = 0;
	virtual void normal(int index) = 0;
	virtual void color(int index) = 0;

	virtual void draw(Texture *texture) = 0;

protected:
	MeshFace(Mesh *parent) { }

};

}

#endif
