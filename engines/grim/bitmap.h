/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
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
 * $URL$
 * $Id$
 *
 */

#ifndef GRIM_BITMAP_H
#define GRIM_BITMAP_H

#include "engines/grim/resource.h"
#include "engines/grim/object.h"

namespace Grim {

class Bitmap : public Object {
public:
	// Construct a bitmap from the given data.
	Bitmap(const char *filename, const char *data, int len);
	Bitmap(const char *data, int width, int height, int bpp, const char *filename);
	Bitmap();

	bool loadTile(const char *filename, const char *data, int len);

	const char *getFilename() const { return _fname.c_str(); }

	void draw() const;

	// Set which image in an animated bitmap to use
	void setNumber(int n) { if ((n - 1) >= _numImages) warning("Bitmap::setNumber: no anim image: %d", n); else _currImage = n; }

	int getNumImages() const { return _numImages; }
	int getCurrentImage() const { return _currImage; }

	int getWidth() const { return _width; }
	int getHeight() const { return _height; }
	int getX() const { return _x; }
	int getY() const { return _y; }
	void setX(int xPos) { _x = xPos; }
	void setY(int yPos) { _y = yPos; }

	char *getData() { return _data[_currImage]; }

	virtual ~Bitmap();

//private:
	Common::String _fname;
	char **_data;
	int _numImages, _currImage;
	int _width, _height, _x, _y;
	int _format;
	int _numTex;
	int _bpp;
	void *_texIds;
	bool _hasTransparency;
	char _filename[32];
};

} // end of namespace Grim

#endif
