
#ifndef AGL_FONT_H
#define AGL_FONT_H

namespace Common {
class String;
}

namespace Math {
class Rect2d;
}

namespace Graphics {
class PixelBuffer;
}

namespace AGL {

class FontMetric {
public:
	virtual ~FontMetric();

	int getStringLength(const Common::String &text) const;

	virtual Math::Rect2d getCharTextureRect(unsigned char c) const = 0;
	virtual Math::Rect2d getCharQuadRect(unsigned char c) const = 0;
	virtual int getCharWidth(unsigned char c) const = 0;
	virtual int getHeight() const = 0;
};

class Font {
public:
	Font(FontMetric *metric);

	inline FontMetric *getMetric() const { return _metric; }

private:
	FontMetric *_metric;
};

}

#endif
