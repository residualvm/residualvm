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

#include "common/rect.h"
#include "common/streamdebug.h"

#include "graphics/agl/label.h"
#include "graphics/agl/font.h"

namespace AGL {

Label::Label()
	: _alignment(Center),
	  _font(NULL) {

	_maxLineWidth = 0;
}

Label::~Label() {

}

void Label::setFont(Font *font) {
	_font = font;
}

void Label::setText(const Common::String &text) {
	_text = text;
	_lines.clear();
	_lines.push_back(text);
}

void Label::setAlignment(Alignment alignment) {
	_alignment = alignment;
}

void Label::setTextColor(const Graphics::Color &color) {
	_color = color;
}

void Label::wrapWords(int maxWidth) {
	_lines.clear();
	_maxLineWidth = 0;

	Common::String msg = _text;
	Common::String message;

	// We break the message to lines not longer than maxWidth
	Common::String currLine;
	int numberLines = 1;
	int lineWidth = 0;
	int maxLineWidth = 0;
	FontMetric *metric = _font->getMetric();
	for (uint i = 0; i < msg.size(); i++) {
		lineWidth += metric->getCharWidth(msg[i]);
		if (lineWidth > maxWidth) {
			bool wordSplit = false;
			if (currLine.contains(' ')) {
				while (msg[i] != ' ' && i > 0) {
					lineWidth = metric->getCharWidth(msg[i]);
					message.deleteLastChar();
					--i;
				}
			} else if (msg[i] != ' ') { // if it is a unique word
				int dashWidth = metric->getCharWidth('-');
				while (lineWidth + dashWidth > maxWidth) {
					lineWidth -= metric->getCharWidth(msg[i]);
					message.deleteLastChar();
					--i;
				}
				message += '-';
				wordSplit = true;
			}
			message += '\n';
			currLine.clear();
			numberLines++;

			if (lineWidth > maxLineWidth) {
				maxLineWidth = lineWidth;
			}
			lineWidth = 0;

			if (wordSplit) {
				lineWidth += metric->getCharWidth(msg[i]);
			} else {
				continue; // don't add the space back
			}
		}

		if (lineWidth > maxLineWidth)
			maxLineWidth = lineWidth;

		message += msg[i];
		currLine += msg[i];
	}

	_lines.resize(numberLines);
	for (int j = 0; j < numberLines; j++) {
		int nextLinePos, cutLen;
		const char *breakPos = strchr(message.c_str(), '\n');
		if (breakPos) {
			nextLinePos = breakPos - message.c_str();
			cutLen = nextLinePos + 1;
		} else {
			nextLinePos = message.size();
			cutLen = nextLinePos;
		}
		Common::String currentLine(message.c_str(), message.c_str() + nextLinePos);
		_lines[j] = currentLine;

		int width = metric->getStringLength(currentLine);
		if (width > _maxLineWidth)
			_maxLineWidth = width;
		for (int count = 0; count < cutLen; count++)
			message.deleteChar(0);
	}
}

Common::Rect Label::getBoundingRect() const {
	return Common::Rect(0, 0, _maxLineWidth, _font->getMetric()->getHeight() * getNumLines());
}

Common::Rect Label::getLineRect(int line) const {
	int length = _font->getMetric()->getStringLength(_lines[line]);
	int height = _font->getMetric()->getHeight();
	int x = 0;
	if (_alignment == Center)
		x = -( length/ 2);
	else if (_alignment == Right)
		x = -_maxLineWidth;

	return Common::Rect(x, height * line, x + length, height * (line + 1));
}

}
