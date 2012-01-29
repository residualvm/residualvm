
#ifndef AGL_BITMAP2D_H
#define AGL_BITMAP2D_H

#include "common/rect.h"

namespace AGL {

class Bitmap2D {
public:
	enum Type {
		Image,
		Depth
	};
	virtual ~Bitmap2D();

	virtual void draw(int x, int y) = 0;

	inline int getWidth() const { return _width; }
	inline int getHeight() const { return _height; }
	inline Type getType() const { return _type; }

protected:
	Bitmap2D(Type type, int width, int height);

private:
	Type _type;
	int _width;
	int _height;
};

}

#endif
