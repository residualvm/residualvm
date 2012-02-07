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

#ifndef AGL_RENDERER_H
#define AGL_RENDERER_H

#include "base/plugins.h"

#include "math/mathfwd.h"

#include "graphics/agl/bitmap2d.h"
#include "graphics/agl/light.h"
#include "graphics/agl/primitive.h"

namespace Common {
class String;
}

namespace Graphics {
class PixelBuffer;
}

namespace AGL {

class Target;
class Texture;
class Mesh;
class ShadowPlane;
class Label;
class Font;
class FontMetric;
class Sprite;

class Renderer {
public:
	Renderer();
	virtual ~Renderer();

	virtual Target *setupScreen(int screenW, int screenH, bool fullscreen, int bpp) = 0;
	virtual void setupCamera(float fov, float nclip, float fclip, float roll) = 0;
	virtual void positionCamera(const Math::Matrix3x3 &worldRot, const Math::Vector3d &pos, const Math::Vector3d &interest) = 0;

	virtual void enableLighting() = 0;
	virtual void disableLighting() = 0;

	virtual Bitmap2D *createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height) = 0;
	virtual Texture *createTexture(const Graphics::PixelBuffer &buf, int width, int height) = 0;
	virtual Mesh *createMesh() = 0;
	virtual Light *createLight(Light::Type type) = 0;
	virtual Primitive *createPrimitive() = 0;
	virtual ShadowPlane *createShadowPlane() = 0;
	virtual Font *createFont(FontMetric *metric, const Graphics::PixelBuffer &buf, int width, int height) = 0;
	virtual Label *createLabel() = 0;
	virtual Sprite *createSprite(float width, float height) = 0;

	virtual void pushMatrix() = 0;
	virtual void translate(float x, float y, float z) = 0;
	virtual void rotate(float deg, float x, float y, float z) = 0;
	virtual void scale(float x, float y, float z) = 0;
	virtual void popMatrix() = 0;

	virtual Common::String prettyName() const = 0;
	virtual Common::String getName() const = 0;
	virtual bool isHardwareAccelerated() const = 0;

};

class RendererPluginObject : public PluginObject {
public:
	virtual Renderer *createInstance() = 0;
};

}

#endif
