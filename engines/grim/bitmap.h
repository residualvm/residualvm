/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
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
 */

#ifndef GRIM_BITMAP_H
#define GRIM_BITMAP_H

#include "graphics/pixelformat.h"

#include "engines/grim/pool.h"

namespace Graphics {
class PixelBuffer;
}

namespace Grim {

/**
 * This BitmapData class keeps the actual bitmap data and can be shared
 * between Bitmap instances, by using getBitmapData.
 * Bitmap still keeps the data that can change between the instances
 * i.e. _x, _y and _currImage.
 * They are automatically deleted if they are not used by any bitmap anymore.
 */
class BitmapData {
public:
	BitmapData(const Common::String &fname, Common::SeekableReadStream *data);
	BitmapData(const Graphics::PixelBuffer &buf, int w, int h, const char *fname);
	BitmapData();
	~BitmapData();

	/**
	 * Loads an EMI TILE-bitmap.
	 *
	 * @param data		the data for the TILE.
	 * @param len		the length of the data.
	 */
	bool loadTile(const Common::String &fname, Common::SeekableReadStream *data);
	bool loadGrimBm(const Common::String &fname, Common::SeekableReadStream *data);
	bool loadTGA(const Common::String &fname, Common::SeekableReadStream *data);

	static BitmapData *getBitmapData(const Common::String &fname, Common::SeekableReadStream *data);
	static Common::HashMap<Common::String, BitmapData *> *_bitmaps;

	const Graphics::PixelBuffer &getImageData(int num) const;

	/**
	 * Convert a bitmap to another color-format.
	 *
	 * @param format	the format to convert to.
	 */
	void convertToColorFormat(const Graphics::PixelFormat &format);

	/**
	 * Convert a bitmap to another color-format.
	 *
	 * @param format	the format to convert to.
	 */
	void convertToColorFormat(int num, const Graphics::PixelFormat &format);

	Common::String _fname;
	int _numImages;
	int _width, _height, _x, _y;
	int _format;
	int _numTex;
	int _bpp;
	int _colorFormat;
	void *_texIds;
	bool _hasTransparency;
	char _filename[32];

	int _refCount;

private:
	Graphics::PixelBuffer *_data;
};

class Bitmap : public PoolObject<Bitmap, MKTAG('V', 'B', 'U', 'F')> {
public:
	/**
	 * Construct a bitmap from the given data.
	 *
	 * @oaram filename	the filename of the bitmap
	 * @param data		the actual data to construct from
	 * @param len		the length of the data
	 */
	Bitmap(const Common::String &filename, Common::SeekableReadStream *data);
	Bitmap(const Graphics::PixelBuffer &buf, int width, int height, const char *filename);
	Bitmap();

	const Common::String &getFilename() const { return _data->_fname; }

	void draw() const;

	/**
	 * Set which image in an animated bitmap to use
	 *
	 * @param n		the image to be selected
	 */
	void setActiveImage(int n);

	int getNumImages() const { return _data->_numImages; }
	int getActiveImage() const { return _currImage; }
	bool getHasTransparency() const { return _data->_hasTransparency; }
	int getFormat() const { return _data->_format; }
	int getWidth() const { return _data->_width; }
	int getHeight() const { return _data->_height; }
	int getX() const { return _x; }
	int getY() const { return _y; }
	void setX(int xPos) { _x = xPos; }
	void setY(int yPos) { _y = yPos; }

	const Graphics::PixelBuffer &getData(int num) const { return _data->getImageData(num); }
	const Graphics::PixelBuffer &getData() const { return getData(_currImage); }
	void *getTexIds() const { return _data->_texIds; }
	int getNumTex() const { return _data->_numTex; }
	const Graphics::PixelFormat &getPixelFormat(int num) const;

	void saveState(SaveGame *state) const;
	void restoreState(SaveGame *state);

	virtual ~Bitmap();

private:
	void freeData();

	BitmapData *_data;
	/**
	 * Specifies a one-based index to the current image in BitmapData.
	 * _currImage==0 means a null image is chosen.
	 */
	int _currImage;
	int _x, _y;
};

} // end of namespace Grim

#endif
