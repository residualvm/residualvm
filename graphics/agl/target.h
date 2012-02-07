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

#ifndef AGL_TARGET_H
#define AGL_TARGET_H

namespace Graphics {
class PixelFormat;
class Surface;
}

namespace AGL {

class Target {
public:
	virtual ~Target();

	virtual void clear() = 0;
	virtual void dim(float amount) = 0;
	virtual void dimRegion(int x, int y, int w, int h, float level) = 0;

	virtual void storeContent() = 0;
	virtual void restoreContent() = 0;

	virtual Graphics::Surface *getScreenshot(const Graphics::PixelFormat &format, int width, int height) const = 0;

	inline int getWidth() const { return _width; }
	inline int getHeight() const { return _height; }

protected:
	Target(int width, int height, int bpp);

private:
	int _width;
	int _height;
};

}

#endif
