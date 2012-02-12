/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "common/rect.h"

#include "graphics/pixelbuffer.h"

#include "graphics/agl/bitmap2d.h"
#include "graphics/agl/manager.h"
#include "graphics/agl/label.h"

#include "engines/grim/debug.h"
#include "engines/grim/grim.h"
#include "engines/grim/textobject.h"
#include "engines/grim/savegame.h"
#include "engines/grim/lua.h"
#include "engines/grim/colormap.h"
#include "engines/grim/font.h"
#include "engines/grim/color.h"

namespace Grim {

TextObjectCommon::TextObjectCommon() :
	_x(0), _y(0), _fgColor(0), _justify(0), _width(0), _height(0),
	_font(NULL), _duration(0), _positioned(false) {
}

TextObject::TextObject(bool blastDraw, bool isSpeech) :
		PoolObject<TextObject, MKTAG('T', 'E', 'X', 'T')>(), TextObjectCommon() {
	_blastDraw = blastDraw;
	_isSpeech = isSpeech;
	_label = NULL;
}

TextObject::TextObject() :
	PoolObject<TextObject, MKTAG('T', 'E', 'X', 'T')>(), TextObjectCommon() {
	_label = NULL;
}

TextObject::~TextObject() {
	delete _label;
}

void TextObject::setText(const Common::String &text) {
	destroy();
	_textID = text;
	setupText();
}

void TextObject::reset() {
	destroy();
	setupText();
}

void TextObject::saveState(SaveGame *state) const {
	state->writeColor(_fgColor);

	state->writeLESint32(_x);
	state->writeLESint32(_y);
	state->writeLESint32(_width);
	state->writeLESint32(_height);
	state->writeLESint32(_justify);
// 	state->writeLESint32(_numberLines);
	state->writeLESint32(_duration);

	state->writeBool(_blastDraw);
	state->writeBool(_isSpeech);
	state->writeLESint32(_elapsedTime);

	state->writeLESint32(_font->getId());

	state->writeString(_textID);
}

bool TextObject::restoreState(SaveGame *state) {
	_fgColor = state->readColor();

	_x            = state->readLESint32();
	_y            = state->readLESint32();
	_width        = state->readLESint32();
	_height       = state->readLESint32();
	_justify      = state->readLESint32();
// 	_numberLines  = state->readLESint32();
	_duration     = state->readLESint32();

	_blastDraw    = state->readBool();
	_isSpeech     = state->readBool();
	_elapsedTime  = state->readLESint32();

	_font = Font::getPool().getObject(state->readLESint32());

	_textID = state->readString();

	setupText();

	return true;
}

void TextObject::setDefaults(TextObjectDefaults *defaults) {
	_x = defaults->getX();
	_y = defaults->getY();
	_font = defaults->getFont();
	_fgColor = defaults->getFGColor();
	_justify = defaults->getJustify();
}

int TextObject::getBitmapWidth() {
	return _label->getBoundingRect().width();
}

int TextObject::getBitmapHeight() {
	return _label->getBoundingRect().height();
}

int TextObject::getTextCharPosition(int pos) {
	int width = 0;
	Common::String msg = LuaBase::instance()->parseMsgText(_textID.c_str(), NULL);
	for (int i = 0; (msg[i] != '\0') && (i < pos); ++i) {
		width += _font->getCharWidth(msg[i]);
	}
	return width;
}

void TextObject::destroy() {
	delete _label;
	_label = NULL;
}

void TextObject::reposition() {
	// In EMI most stuff seems to be relative to the center,
	// but sometimes it is not so I catch that with _x being over 320.
	// This is probably not the corrent way to do it though.
	if (!_positioned && g_grim->getGameType() == GType_MONKEY4) {
		_positioned = true;
		if (_x == 0) {
			_x += 320;
			if (_y < 0) {
				_y = -_y;
			} else {
				_y = 240 - _y;
			}
		} else if (_x > 320) {
			_y = -_y;
		} else {
			_x += 320;
			_y = 240 - _y;
		}
	}
}

void TextObject::setFGColor(const Color &fgColor) {
	TextObjectCommon::setFGColor(fgColor);

	if (_label) {
		_label->setTextColor(fgColor);
	}
}

void TextObject::setupText() {
	Common::String msg = LuaBase::instance()->parseMsgText(_textID.c_str(), NULL);

	// remove spaces (NULL_TEXT) from the end of the string,
	// while this helps make the string unique it screws up
	// text justification
	// remove char of id 13 from the end of the string,
	int pos = msg.size() - 1;
	while (pos >= 0 && (msg[pos] == ' ' || msg[pos] == 13)) {
		msg.deleteLastChar();
		pos = msg.size() - 1;
	}

	reposition();

	// format the output message to incorporate line wrapping
	// (if necessary) for the text object
	const int SCREEN_WIDTH = _width ? _width : 640;
	const int SCREEN_MARGIN = 75;

	// If the speaker is too close to the edge of the screen we have to make
	// some room for the subtitles.
	if (_isSpeech){
		if (_x < SCREEN_MARGIN) {
			_x = SCREEN_MARGIN;
		} else if (SCREEN_WIDTH - _x < SCREEN_MARGIN) {
			_x = SCREEN_WIDTH - SCREEN_MARGIN;
		}
	}

	// The maximum width for any line of text is determined by the justification
	// mode. Note that there are no left/right margins -- this is consistent
	// with GrimE.
	int maxWidth = 0;
	if (_justify == CENTER) {
		maxWidth = 2 * MIN(_x, SCREEN_WIDTH - _x);
	} else if (_justify == LJUSTIFY) {
		maxWidth = SCREEN_WIDTH - _x;
	} else if (_justify == RJUSTIFY) {
		maxWidth = _x;
	}

	delete _label;
	_label = AGLMan.createLabel(getFont()->getFont(), msg);
	_label->setTextColor(_fgColor);
	_label->wrapWords(maxWidth);
	AGL::Label::Alignment al;
	switch (getJustify()) {
		case LJUSTIFY:
			al = AGL::Label::Left;
			break;
		case RJUSTIFY:
			al = AGL::Label::Right;
			break;
		default:
			al = AGL::Label::Center;
	}
	_label->setAlignment(al);

	// If the text object is a speech subtitle, the y parameter is the
	// coordinate of the bottom of the text block (instead of the top). It means
	// that every extra line pushes the previous lines up, instead of being
	// printed further down the screen.
	const int SCREEN_TOP_MARGIN = 16;
	if (_isSpeech) {
		_y -= _label->getBoundingRect().height();
		if (_y < SCREEN_TOP_MARGIN) {
			_y = SCREEN_TOP_MARGIN;
		}
	}

	_elapsedTime = 0;
}

void TextObject::draw() {
	if (!_label)
		return;

	if (_justify > 3 || _justify < 0)
		warning("TextObject::draw: Unknown justification code (%d)", _justify);

	int y = _y;
	if (_blastDraw)
		y = _y + 5;
	else {
		if (_font->getHeight() == 21) // talk_font,verb_font
			y = _y - 6;
		else if (_font->getHeight() == 26) // special_font
			y = _y - 12;
		else if (_font->getHeight() == 13) // computer_font
			y = _y - 6;
		else if (_font->getHeight() == 19) // pt_font
			y = _y - 9;
		else
			y = _y;
	}
	if (y < 0)
		y = 0;

	_label->draw(_x, y);
}

void TextObject::update() {
	if (!_duration) {
		return;
	}

	_elapsedTime += g_grim->getFrameTime();
	if (_elapsedTime > _duration) {
		delete this;
	}
}

} // end of namespace Grim
