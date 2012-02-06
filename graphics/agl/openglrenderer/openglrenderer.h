
#ifndef AGL_OPENGLRENDERER_H
#define AGL_OPENGLRENDERER_H

#include "graphics/agl/renderer.h"

#ifdef USE_OPENGL

#if defined (SDL_BACKEND) && !defined(__amigaos4__)
#include <SDL_opengl.h>
#undef ARRAYSIZE
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

namespace AGL {

class GLBitmap2D;

class OpenGLRenderer : public Renderer {
public:
	Target *setupScreen(int screenW, int screenH, bool fullscreen, int bpp);
	void setupCamera(float fov, float nclip, float fclip, float roll);
	void positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest);

	void enableLighting();
	void disableLighting();

	Bitmap2D *createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	Texture *createTexture(const Graphics::PixelBuffer &buf, int width, int height);
	Mesh *createMesh();
	Light *createLight(Light::Type type);
	Primitive *createPrimitive();
	ShadowPlane *createShadowPlane();
	Font *createFont(FontMetric *metric, const Graphics::PixelBuffer &buf, int width, int height);
	Label *createLabel();
	Sprite *createSprite(float width, float height);

	void pushMatrix();
	void translate(float x, float y, float z);
	void rotate(float deg, float x, float y, float z);
	void popMatrix();


	const char *prettyString() const;

private:
	void initExtensions();

	GLuint _emergFont;
	int _smushNumTex;
	GLuint *_smushTexIds;
	int _smushWidth;
	int _smushHeight;
	bool _useDepthShader;
	GLuint _fragmentProgram;

	friend class GLBitmap2D;
};

}

#endif

#endif
