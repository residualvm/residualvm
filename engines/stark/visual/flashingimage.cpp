/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
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

#include "engines/stark/visual/flashingimage.h"

#include "common/random.h"
#include "graphics/surface.h"

#include "engines/stark/gfx/driver.h"
#include "engines/stark/gfx/surfacerenderer.h"
#include "engines/stark/gfx/texture.h"

#include "engines/stark/services/global.h"
#include "engines/stark/services/services.h"

namespace Stark {

const float VisualFlashingImage::_fadeValueMax = 0.55f;

VisualFlashingImage::VisualFlashingImage(Gfx::Driver *gfx) :
		Visual(TYPE),
		_gfx(gfx),
		_texture(nullptr),
		_surface(nullptr),
		_fadeLevelIncreasing(true),
		_fadeLevel(0),
		_flashingTimeRemaining(150 * 33) {
	_surfaceRenderer = _gfx->createSurfaceRenderer();
}

VisualFlashingImage::~VisualFlashingImage() {
	if (_surface) {
		_surface->free();
	}
	delete _surface;
	delete _texture;
	delete _surfaceRenderer;
}

void VisualFlashingImage::initFromSurface(const Graphics::Surface *surface) {
	assert(!_surface && !_texture);

	// Decode the XMG
	_surface = new Graphics::Surface();
	_surface->copyFrom(*surface);
	_texture = _gfx->createTexture(_surface);
}

void VisualFlashingImage::updateFadeLevel() {
	uint millisecondsPerGameloop = StarkGlobal->getMillisecondsPerGameloop();
	if (_flashingTimeRemaining > 0) {
		_flashingTimeRemaining -= millisecondsPerGameloop;
		if (_fadeLevelIncreasing) {
			_fadeLevel += 0.0022f * millisecondsPerGameloop;
		}
		else {
			_fadeLevel -= 0.0022f * millisecondsPerGameloop;
		}
		if (ABS(_fadeLevel) >= _fadeValueMax) {
			_fadeLevelIncreasing = !_fadeLevelIncreasing;
			_fadeLevel = CLIP(_fadeLevel, -_fadeValueMax, _fadeValueMax);
		}
	} else {
		// Fill with transparent color
		_surface->fillRect(Common::Rect(_surface->w, _surface->h), 0);
		_fadeLevel = 0;
	}
}

void VisualFlashingImage::render(const Common::Point &position) {
	updateFadeLevel();

	_surfaceRenderer->setFadeLevel(_fadeLevel);
	_surfaceRenderer->render(_texture, position);
}

} // End of namespace Stark
