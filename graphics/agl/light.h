
#ifndef AGL_LIGHT_H
#define AGL_LIGHT_H

#include "math/vector3d.h"

#include "graphics/color.h"

namespace AGL {

class Light {
public:
	enum Type {
		Point,
		Directional,
		Spot
	};
	Light(Type type) { _type = type; }
	virtual ~Light() { }

	void setPosition(const Math::Vector3d &pos) { _position = pos; }
	void setDirection(const Math::Vector3d &dir) { _direction = dir; }
	void setColor(const Graphics::Color &color) { _color = color; }
	void setIntensity(float intensity) { _intensity = intensity; }
	void setCutoff(float cutoff) { _cutoff = cutoff; }

	virtual void enable() = 0;
	virtual void disable() = 0;

	inline Type getType() const { return _type; }
	inline const Math::Vector3d &getPosition() const { return _position; }
	inline const Math::Vector3d &getDirection() const { return _direction; }
	inline const Graphics::Color &getColor() const { return _color; }
	inline float getIntensity() const { return _intensity; }
	inline float getCutoff() const { return _cutoff; }

private:
	Type _type;
	Math::Vector3d _position;
	Math::Vector3d _direction;
	Graphics::Color _color;
	float _intensity;
	float _cutoff;
};

}

#endif