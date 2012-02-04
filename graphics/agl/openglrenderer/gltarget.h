
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
