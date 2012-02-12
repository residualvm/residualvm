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
	void positionCamera(const Math::Matrix3x3 &worldRot, const Math::Vector3d &pos, const Math::Vector3d &interest);

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
	void scale(float x, float y, float z);
	void popMatrix();

	Common::String prettyName() const;
	Common::String getName() const;
	bool isHardwareAccelerated() const;

	static GLenum drawMode(DrawMode mode);

	bool _shadowActive;

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
