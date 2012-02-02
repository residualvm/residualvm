
#ifndef AGL_MODELVIEW_H
#define AGL_MODELVIEW_H

#include "math/vector3d.h"

namespace Math {
class Angle;
}

namespace AGL {

class ModelView {
public:
	static void pushMatrix();
	static void popMatrix();

	static void translate(float x, float y, float z);
	inline static void translate(const Math::Vector3d &vec) { translate(vec.x(), vec.y(), vec.z()); }

	inline static void rotate(const Math::Angle &angle, const Math::Vector3d &axis) { rotate(angle, axis.x(), axis.y(), axis.z()); }
	static void rotate(const Math::Angle &angle, float x, float y, float z);

};

}

#endif
