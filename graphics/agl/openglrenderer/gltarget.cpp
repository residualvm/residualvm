
#include "common/system.h"

#include "graphics/surface.h"
#include "graphics/pixelbuffer.h"

#include "graphics/agl/openglrenderer/gltarget.h"

#if defined (SDL_BACKEND) && !defined(__amigaos4__)
#include <SDL_opengl.h>
#undef ARRAYSIZE
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

namespace AGL {

GLTarget::GLTarget(int width, int height, int bpp)
	: Target(width, height, bpp) {

}

void GLTarget::clear() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLTarget::dim(float amount) {
	uint32 *data = (uint32 *)_storedDisplay;
	for (int l = 0; l < getWidth() * getHeight(); l++) {
		uint32 pixel = data[l];
		uint8 r = (pixel & 0xFF0000) >> 16;
		uint8 g = (pixel & 0x00FF00) >> 8;
		uint8 b = (pixel & 0x0000FF);
		uint32 color = (r + g + b) * amount;
		data[l] = ((color & 0xFF) << 16) | ((color & 0xFF) << 8) | (color & 0xFF);
	}
}

void GLTarget::dimRegion(int x, int yReal, int w, int h, float level) {
	uint32 *data = new uint32[w * h];
	int y = getHeight() - yReal;

	// collect the requested area and generate the dimmed version
	glReadPixels(x, y - h, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
	for (int ly = 0; ly < h; ly++) {
		for (int lx = 0; lx < w; lx++) {
			uint32 pixel = data[ly * w + lx];
			uint8 r = (pixel & 0xFF0000) >> 16;
			uint8 g = (pixel & 0x00FF00) >> 8;
			uint8 b = (pixel & 0x0000FF);
			uint32 color = (uint32)(((r + g + b) / 3) * level);
			data[ly * w + lx] = ((color & 0xFF) << 16) | ((color & 0xFF) << 8) | (color & 0xFF);
		}
	}

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, getWidth(), getHeight(), 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	// Set the raster position and draw the bitmap
	glRasterPos2i(x, yReal + h);
	glDrawPixels(w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	delete[] data;
}


void GLTarget::storeContent() {
	glReadPixels(0, 0, getWidth(), getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, _storedDisplay);
}

void GLTarget::restoreContent() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, getWidth(), getHeight(), 0, 0, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glRasterPos2i(0, getHeight() - 1);
	glBitmap(0, 0, 0, 0, 0, -1, NULL);
	glDrawPixels(getWidth(), getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, _storedDisplay);

	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

Graphics::Surface *GLTarget::getScreenshot(const Graphics::PixelFormat &format, int width, int height) const {
	Graphics::Surface *s = new Graphics::Surface;
	s->create(width, height, format);
	Graphics::PixelBuffer buffer(format, (byte *)s->pixels);
	Graphics::PixelBuffer src(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24), getWidth() * getHeight(), DisposeAfterUse::YES);

	glReadPixels(0, 0, getWidth(), getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, src.getRawBuffer());

	int i1 = (getWidth() * width - 1) / getWidth() + 1;
	int j1 = (getHeight() * height - 1) / getHeight() + 1;

	for (int j = 0; j < j1; j++) {
		for (int i = 0; i < i1; i++) {
			int x0 = i * getWidth() / width;
			int x1 = ((i + 1) * getWidth() - 1) / width + 1;
			int y0 = j * getHeight() / height;
			int y1 = ((j + 1) * getHeight() - 1) / height + 1;
			uint32 color = 0;
			for (int y = y0; y < y1; y++) {
				for (int x = x0; x < x1; x++) {
					uint8 lr, lg, lb;
					src.getRGBAt(y * getWidth() + x, lr, lg, lb);
					color += (lr + lg + lb) / 3;
				}
			}
			color /= (x1 - x0) * (y1 - y0);
			buffer.setPixelAt((height - j - 1) * width + i, color, color, color);
		}
	}

	return s;
}

}
