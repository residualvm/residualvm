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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/textconsole.h"

#include "graphics/opengles2/framebuffer.h"

#ifdef ANDROID_DEBUG_GL
#include <android/log.h>

extern const char *android_log_tag;

#define _ANDROID_LOG(prio, fmt, args...) __android_log_print(prio, android_log_tag, fmt, ## args)
#define LOGD(fmt, args...) _ANDROID_LOG(ANDROID_LOG_DEBUG, fmt, ##args)
#define LOGI(fmt, args...) _ANDROID_LOG(ANDROID_LOG_INFO, fmt, ##args)
#define LOGW(fmt, args...) _ANDROID_LOG(ANDROID_LOG_WARN, fmt, ##args)
#define LOGE(fmt, args...) _ANDROID_LOG(ANDROID_LOG_ERROR, fmt, ##args)

extern void checkGlError(const char *expr, const char *file, int line);

#ifdef ANDROID_DEBUG_GL_CALLS
#define GLCALLLOG(x, before) \
	do { \
		if (before) \
			LOGD("calling '%s' (%s:%d)", x, __FILE__, __LINE__); \
		else \
			LOGD("returned from '%s' (%s:%d)", x, __FILE__, __LINE__); \
	} while (false)
#else
#define GLCALLLOG(x, before) do {  } while (false)
#endif

#define GLCALL(x) \
	do { \
		GLCALLLOG(#x, true); \
		(x); \
		GLCALLLOG(#x, false); \
		checkGlError(#x, __FILE__, __LINE__); \
	} while (false)

#define GLTHREADCHECK \
	do { \
		assert(pthread_self() == _main_thread); \
	} while (false)

#else
#define GLCALL(x) do { (x); } while (false)
#define GLTHREADCHECK do {  } while (false)
#endif

namespace Graphics {

FrameBuffer::FrameBuffer(GLuint texture_name, uint width, uint height) : _colorTexture(texture_name), _width(width), _height(height) {
GLCALL(	glGenFramebuffers(1, &_frameBuffer));
GLCALL(	glGenRenderbuffers(1, &_depthRenderBuffer));

GLCALL(	glBindRenderbuffer(GL_RENDERBUFFER, _depthRenderBuffer));
GLCALL(	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));
GLCALL(	glBindRenderbuffer(GL_RENDERBUFFER, 0));

GLCALL(	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer));
GLCALL(	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_name, 0));
GLCALL(	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRenderBuffer));

GLCALL(	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer));
	GLenum status=glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		error("Framebuffer is not complete! status: %d", status);

GLCALL(	glBindTexture(GL_TEXTURE_2D, 0));
GLCALL(	glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

FrameBuffer::~FrameBuffer() {
GLCALL(	glDeleteRenderbuffers(1, &_depthRenderBuffer));
GLCALL(	glDeleteFramebuffers(1, &_frameBuffer));
}

void FrameBuffer::attach(uint actual_width, uint actual_height) {
GLCALL(	glBindFramebuffer(GL_FRAMEBUFFER, _frameBuffer));
GLCALL(	glViewport(0,0, actual_width, actual_height));

GLCALL(	glClearColor(0, 0, 0, 1.0f));
GLCALL(	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::detach() {
GLCALL(	glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

}
