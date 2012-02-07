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

#ifndef AGL_GLTARGET_H
#define AGL_GLTARGET_H

#include "graphics/agl/target.h"

namespace AGL {

class GLTarget : public Target {
public:
	GLTarget(int width, int height, int bpp);

	void clear();
	void dim(float amount);
	void dimRegion(int x, int y, int w, int h, float level);

	void storeContent();
	void restoreContent();

	Graphics::Surface *getScreenshot(const Graphics::PixelFormat &format, int width, int height) const;

	byte *_storedDisplay;
};

}

#endif
