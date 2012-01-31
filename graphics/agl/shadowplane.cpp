
#include "graphics/agl/shadowplane.h"

namespace AGL {

ShadowPlane::ShadowPlane() {
}

ShadowPlane::~ShadowPlane() {

}

void ShadowPlane::addSector(const ShadowPlane::Vertices &vert) {
	Sector s = { vert };
	_sectors.push_back(s);
}

}
