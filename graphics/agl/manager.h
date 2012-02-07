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

#ifndef AGL_MANAGER_H
#define AGL_MANAGER_H

#include "common/singleton.h"

#include "math/mathfwd.h"

#include "graphics/agl/bitmap2d.h"
#include "graphics/agl/light.h"
#include "graphics/agl/shadowplane.h"

namespace Common {
class String;
}

namespace Graphics {
class PixelBuffer;
struct Surface;
}

namespace AGL {

class Renderer;
class Target;
class Mesh;
class Texture;
class Primitive;
class ShadowPlane;
class Label;
class Font;
class FontMetric;
class Sprite;

enum DrawMode {
	Points,
	Lines,
	LineLoop,
	Triangles,
	Quads,
	Polygon
};

class Manager : public Common::Singleton<Manager> {
public:
	Manager();
	void init(const Common::String &renderer);

	Target *setupScreen(int screenW, int screenH, bool fullscreen, int bpp);
	Target *getTarget() const { return _target; }

	bool isFullscreen() const;
	bool isHardwareAccelerated() const;
	Common::String getRendererName() const;

	void setupCamera(float fov, float nclip, float fclip, float roll);
	void positionCamera(const Math::Matrix3x3 &worldRot, const Math::Vector3d &pos, const Math::Vector3d &interest);
	void flipBuffer();

	void enableLighting();
	void disableLighting();

	Bitmap2D *createBitmap2D(Bitmap2D::Type type, const Graphics::PixelBuffer &buf, int width, int height);
	Bitmap2D *createBitmap2D(Graphics::Surface *surface);
	Texture *createTexture(const Graphics::PixelBuffer &buf, int width, int height);
	Texture *createTexture(Graphics::Surface *surface);
	Mesh *createMesh();
	Light *createLight(Light::Type type);
	Primitive *createPrimitive();
	ShadowPlane *createShadowPlane();
	Font *createFont(FontMetric *metric, const Graphics::PixelBuffer &buf, int width, int height);
	Label *createLabel(Font *font, const Common::String &string);
	Label *createLabel(Font *font);
	Sprite *createSprite(float width, float height);

private:
	Renderer *_renderer;
	Target *_target;
	bool _fullscreen;

	friend class ModelView;
};

#define AGLMan AGL::Manager::instance()

}

#endif
