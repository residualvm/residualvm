
#ifndef AGL_LABEL_H
#define AGL_LABEL_H

#include "common/array.h"
#include "common/str.h"

#include "graphics/color.h"

namespace AGL {

class Font;

class Label {
public:
	enum Alignment {
		Left,
		Right,
		Center
	};

	virtual ~Label();

	void setFont(Font *font);
	void setText(const Common::String &text);
	void setAlignment(Alignment alignment);
	void setTextColor(const Graphics::Color &color);

	void wrapWords(int maxWidth);

	virtual void draw(int x, int y) const = 0;

	Common::Rect getBoundingRect() const;

protected:
	Label();

	inline Font *getFont() const { return _font; }
	inline const Common::String &getText() const { return _text; }
	inline const Graphics::Color &getTextColor() const { return _color; }

	inline int getNumLines() const { return _lines.size(); }
	inline const Common::String &getLine(int i) const { return _lines[i]; }

	Common::Rect getLineRect(int line) const;

private:
	Alignment _alignment;
	Font *_font;
	Graphics::Color _color;
	Common::String _text;
	Common::Array<Common::String> _lines;
	int _maxLineWidth;
};

}

#endif
