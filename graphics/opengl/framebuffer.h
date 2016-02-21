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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef GRAPHICS_FRAMEBUFFER_H
#define GRAPHICS_FRAMEBUFFER_H

#include "graphics/opengl/system_headers.h"
#include "graphics/opengl/texture.h"

namespace Graphics {

class FrameBuffer : public Texture {
public:
	FrameBuffer(uint width, uint height);
	FrameBuffer(GLuint texture_name, uint width, uint height, uint texture_width, uint texture_height);
#ifdef AMIGAOS
	virtual ~FrameBuffer() {}

	void attach() {}
	void detach() {}
#else
	virtual ~FrameBuffer();

	virtual void attach();
	virtual void detach();
#endif

protected:
	GLuint getFrameBufferName() const { return _frameBuffer; }

private:
	void init();
	GLuint _renderBuffers[2];
	GLuint _frameBuffer;
};

#if defined(SDL_BACKEND) && !defined(AMIGAOS)
class MultiSampleFrameBuffer : public FrameBuffer {
public:
	MultiSampleFrameBuffer(uint width, uint height);
	~MultiSampleFrameBuffer();

	virtual void attach();
	virtual void detach();

private:
	void init();
	GLuint _msFrameBufferId;
	GLuint _msColorId;
	GLuint _msDepthId;
};
#endif

}

#endif
