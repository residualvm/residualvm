
#include "graphics/tinygl/zbuffer.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/target.h"

#include "graphics/agl/tinyglrenderer/tglbitmap2d.h"
#include "graphics/agl/tinyglrenderer/tinyglrenderer.h"

namespace AGL {

/**
 * This class is used for blitting bitmaps with transparent pixels.
 * Instead of checking every pixel for transparency, it creates a list of 'lines'.
 * A line is, well, a line of non trasparent pixels, and it stores a pointer to the
 * first pixel, and the position of it, which can be used to memcpy the entire line
 * to the destination buffer.
 */
class BlitImage {
public:
	BlitImage(const Graphics::PixelFormat &format) {
		_format = format;
		_lines = NULL;
		_last = NULL;
	}
	~BlitImage() {
		Line *temp = _lines;
		while (temp != NULL) {
			_lines = temp->next;
			delete temp;
			temp = _lines;
		}
	}

	void create(const Graphics::PixelBuffer &buf, int width, int height) {
		Graphics::PixelBuffer srcBuf = buf;
		// A line of pixels can not wrap more that one line of the image, since it would break
		// blitting of bitmaps with a non-zero x position.
		for (int l = 0; l < height; l++) {
			int start = -1;

			for (int r = 0; r < width; ++r) {
				uint8 a, red, g, b;
				srcBuf.getARGBAt(r, a, red, g, b);
				bool transparent = a < 50; //FIXME is 50 ok?
				// We found a transparent pixel, so save a line from 'start' to the pixel before this.
				if (transparent && start >= 0) {
					newLine(start, l, r - start, srcBuf.shiftedBy(start));

					start = -1;
				} else if (!transparent && start == -1) {
					start = r;
				}
			}
			// end of the bitmap line. if start is an actual pixel save the line.
			if (start >= 0) {
				newLine(start, l, width - start, srcBuf.shiftedBy(start));
			}

			srcBuf.shiftBy(width);
		}
	}

	void newLine(int x, int y, int length, const Graphics::PixelBuffer &buf) {
		if (length < 1) {
			return;
		}

		Line *line = new Line;

		Graphics::PixelBuffer b(_format, length, DisposeAfterUse::NO);
		b.copyBuffer(0, length, buf);

		line->x = x;
		line->y = y;
		line->length = length;
		line->pixels = b.getRawBuffer();
		line->next = NULL;

		if (_last) {
			_last->next = line;
		}
		if (!_lines) {
			_lines = line;
		}
		_last = line;
	}

	struct Line {
		int x;
		int y;
		int length;
		byte *pixels;

		Line *next;
	};

	Graphics::PixelFormat _format;
	Line *_lines;
	Line *_last;
};


TGLBitmap2D::TGLBitmap2D(TinyGLRenderer *renderer, Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height)
	: Bitmap2D(type, width, height),
	  _renderer(renderer),
	  _size(width * height) {

	if (type == Bitmap2D::Image) {
		_img = new BlitImage(_renderer->_zb->cmode);
		_img->create(buf, width, height);
		_zimg = NULL;
	} else {
		_zimg = new uint16[_size];
		uint16 *p = (uint16 *)buf.getRawBuffer();
		for (int i = 0; i < _size; ++i) {
			_zimg[i] = 0xffff - p[i];
		}
		_img = NULL;
	}
}

TGLBitmap2D::~TGLBitmap2D() {
	delete[] _zimg;
	delete _img;
}

void TGLBitmap2D::draw(int x, int y) {
	const int width = AGLMan.getTarget()->getWidth();
	if (getType() == Bitmap2D::Image) {
		BlitImage::Line *l = _img->_lines;
		while (l) {
			memcpy(_renderer->_zb->pbuf.getRawBuffer((y + l->y) * width + x + l->x), l->pixels, l->length * _img->_format.bytesPerPixel);
			l = l->next;
		}
	} else {
		for (int l = 0; l < getHeight(); ++l) {
			uint16 *dst = _renderer->_zb->zbuf + (y + l) * width + x;
			memcpy(dst, _zimg + l * getWidth(), getWidth() * 2);
		}
	}
}

}
