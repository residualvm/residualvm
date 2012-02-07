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

#ifndef AGL_GLBITMAP2D_H
#define AGL_GLBITMAP2D_H

#include "graphics/agl/bitmap2d.h"

namespace AGL {

class GLBitmap2D : public Bitmap2D {
public:
	GLBitmap2D(OpenGLRenderer *rend, Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	~GLBitmap2D();

	void draw(int x, int y);

	OpenGLRenderer *_renderer;
	GLuint *_texIds;
	int _numTex;
	byte *_data;
};

}

#endif
