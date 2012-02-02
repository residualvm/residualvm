
#ifndef AGL_TARGET_H
#define AGL_TARGET_H

namespace AGL {

class Target {
public:
	virtual ~Target();

	virtual void clear() = 0;
	virtual void dim(float amount) = 0;

	virtual void storeContent() = 0;
	virtual void restoreContent() = 0;

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
