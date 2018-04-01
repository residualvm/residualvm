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

VisualFlashingImage::VisualFlashingImage(Gfx::Driver *gfx) :
		Visual(TYPE),
		_gfx(gfx),
		_texture(nullptr),
		_surface(nullptr),
		_originalSurface(nullptr),
		_flashingTimeRemaining(150 * 33) {
	_surfaceRenderer = _gfx->createSurfaceRenderer();
}

VisualFlashingImage::~VisualFlashingImage() {
	if (_originalSurface) {
		_originalSurface->free();
	}
	if (_surface) {
		_surface->free();
	}
	delete _originalSurface;
	delete _surface;
	delete _texture;
	delete _surfaceRenderer;
}

void VisualFlashingImage::initFromSurface(const Graphics::Surface *surface) {
	assert(!_originalSurface && !_surface && !_texture);

	_originalSurface = new Graphics::Surface();
	_originalSurface->copyFrom(*surface);
	_surface = new Graphics::Surface();
	_surface->create(_originalSurface->w, _originalSurface->h, _originalSurface->format);
	_texture = _gfx->createTexture(_surface);
}

void VisualFlashingImage::render(const Common::Point &position) {

	if (_flashingTimeRemaining > 0) {
		_flashingTimeRemaining -= StarkGlobal->getMillisecondsPerGameloop();
		_surface->copyFrom(*_originalSurface);

		// Pixel brightness is calculated by triangle waveform function from the remainig time
		// Amplitude of the wave is 278 and period is 990ms(33ms*30fps)
		int32 brightnessDifference = abs(495 - (_flashingTimeRemaining + 248) % 990) / 1.8 - 139;

		for (uint i = 0; i < _surface->h; i++) {
			for (uint j = 0; j < _surface->w; j++) {
				uint32 *src = (uint32 *)_surface->getBasePtr(j, i);
				uint8 a, r, g, b;
				_surface->format.colorToARGB(*src, a, r, g, b);
				r = CLIP(r + brightnessDifference, 0, 0xff);
				g = CLIP(g + brightnessDifference, 0, 0xff);
				b = CLIP(b + brightnessDifference, 0, 0xff);
				*src = _surface->format.ARGBToColor(a, r, g, b);
			}
		}
	} else {
		// Fill with transparent color
		_surface->fillRect(Common::Rect(_surface->w, _surface->h), 0);
	}

	_texture->update(_surface);
	_surfaceRenderer->render(_texture, position);
}

} // End of namespace Stark
