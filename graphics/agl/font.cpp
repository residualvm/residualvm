
#include "graphics/agl/font.h"
#include "graphics/agl/texture.h"

namespace AGL {

Font::Font() {
}

Font::~Font() {
	delete _texture;
}

void Font::setTexture(Texture *t) {
	_texture = t;
}

void Font::bind() {
	_texture->bind();
}

int Font::getStringLength(const Common::String &text) const {
	int result = 0;
	for (uint32 i = 0; i < text.size(); ++i) {
		result += getCharWidth(text[i]);
// 		result += MAX(getCharDataWidth(text[i]), getCharWidth(text[i]));
	}
	return result;
}

}
