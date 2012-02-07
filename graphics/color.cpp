
#include "graphics/color.h"

namespace Graphics {

Color::Color() {
	memset(_vals, 0, 4);
}

Color::Color(uint8 r, uint8 g, uint8 b, uint8 a) {
	set(r, g, b, a);
}

Color::Color(const Color &c) {
	*this = c;
}

void Color::setRed(uint8 r) {
	_vals[0] = r;
}

void Color::setGreen(uint8 g) {
	_vals[1] = g;
}

void Color::setBlue(uint8 b) {
	_vals[2] = b;
}

void Color::setAlpha(uint8 a) {
	_vals[3] = a;
}

void Color::set(uint8 r, uint8 g, uint8 b, uint8 a) {
	_vals[0] = r;
	_vals[1] = g;
	_vals[2] = b;
	_vals[3] = a;
}

Color &Color::operator=(const Color &c) {
	memcpy(_vals, c._vals, 4);

	return *this;
}

}

Common::Debug &operator<<(Common::Debug dbg, const Graphics::Color &c) {
	dbg.nospace() << "Color(" << c.getRed() << ", " << c.getGreen() << ", " << c. getBlue() << ", " << c.getAlpha() << ")";

	return dbg.space();
}
