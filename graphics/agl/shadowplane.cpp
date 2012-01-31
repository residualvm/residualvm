
#include "graphics/agl/shadowplane.h"

namespace AGL {

ShadowPlane::ShadowPlane() {
}

ShadowPlane::~ShadowPlane() {

}

void ShadowPlane::addSector(const ShadowPlane::Vertices &vert) {
	Sector s;
	s._vertices = vert;

	assert(vert.size() > 2);
	s._normal = Math::Vector3d::crossProduct(vert[1] - vert[0], vert[vert.size() - 1] - vert[0]);

	_sectors.push_back(s);
}

}
