/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

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
