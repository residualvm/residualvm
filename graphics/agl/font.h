
#ifndef AGL_FONT_H
#define AGL_FONT_H

namespace Common {
class String;
}

namespace Math {
class Rect2d;
}

namespace AGL {

class Texture;

class Font {
public:
	Font();
	virtual ~Font();

	void setTexture(Texture *texture);
	void bind();

	int getStringLength(const Common::String &text) const;

	virtual Math::Rect2d getCharTextureRect(unsigned char c) const = 0;
	virtual Math::Rect2d getCharQuadRect(unsigned char c) const = 0;
	virtual int getCharWidth(unsigned char c) const = 0;
	virtual int getHeight() const = 0;

private:
	Texture *_texture;
};

}

#endif
