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

#ifndef GRAPHICS_TINYGL_ZBLIT_H_
#define GRAPHICS_TINYGL_ZBLIT_H_

#include "graphics/surface.h"
#include "common/rect.h"

namespace Graphics {

struct BlitTransform {
	BlitTransform(int dstX, int dstY) : _rotation(0), _originX(0), _originY(0), _aTint(1.0f),
				_rTint(1.0f), _gTint(1.0f), _bTint(1.0), _flipHorizontally(false),
				_flipVertically(false) {
		_destinationRectangle.translate(dstX,dstY);
	}

	void sourceRectangle(int srcX, int srcY, int srcWidth, int srcHeight) {
		_sourceRectangle.left = srcX;
		_sourceRectangle.top = srcY;
		_sourceRectangle.setWidth(srcWidth);
		_sourceRectangle.setHeight(srcHeight);
	}

	void tint(float aTint, float rTint = 1.0f, float gTint = 1.0f, float bTint = 1.0f) {
		_aTint = aTint;
		_rTint = rTint;
		_gTint = gTint;
		_bTint = bTint;
	}

	void scale(int width, int height) {
		_destinationRectangle.setWidth(width);
		_destinationRectangle.setHeight(height);
	}

	void rotate(int rotation, int originX, int originY) {
		_rotation = rotation;
		_originX = originX;
		_originY = originY;
	}

	void flip(bool verticalFlip, bool horizontalFlip) {
		_flipVertically = verticalFlip;
		_flipHorizontally = horizontalFlip;
	}

	bool operator==(const BlitTransform &other) const {
		return _sourceRectangle == other._sourceRectangle && _destinationRectangle == other._destinationRectangle &&
			_rotation == other._rotation && _originX == other._originX && _originY == other._originY &&
			_aTint == other._aTint && _rTint == other._rTint && _gTint == other._gTint && _bTint == other._bTint &&
			_flipHorizontally == other._flipHorizontally && _flipVertically == other._flipVertically;
	}

	Common::Rect _sourceRectangle;
	Common::Rect _destinationRectangle;
	int _rotation;
	int _originX, _originY;
	float _aTint, _rTint, _gTint, _bTint;
	bool _flipHorizontally, _flipVertically;
};

struct BlitImage;

BlitImage *tglGenBlitImage();
void tglUploadBlitImage(BlitImage *blitImage, const Graphics::Surface &surface, uint32 colorKey, bool applyColorKey);
void tglDeleteBlitImage(BlitImage *blitImage);
void tglGetBlitImageSize(BlitImage *blitImage, int &width, int &height);
int tglGetBlitImageVersion(BlitImage *blitImage);

void tglBlit(BlitImage *blitImage, const BlitTransform &transform);
void tglBlit(BlitImage *blitImage, int x, int y);

// Disables blending explicitly.
void tglBlitNoBlend(BlitImage *blitImage, const BlitTransform &transform);

// Disables blending, transforms and tinting.
void tglBlitFast(BlitImage *blitImage, int x, int y);

// Blits onto the zbuffer
void tglBlitZBuffer(BlitImage *blitImage, int x, int y);


namespace Internal {
	void tglCleanupImages(); // This function checks if any blit image is to be cleaned up and deletes it.

	void tglBlit(BlitImage *blitImage, const BlitTransform &transform);

	// Disables blending explicitly.
	void tglBlitNoBlend(BlitImage *blitImage, const BlitTransform &transform);

	// Disables blending, transforms and tinting.
	void tglBlitFast(BlitImage *blitImage, int x, int y);

	void tglBlitZBuffer(BlitImage *blitImage, int x, int y);

	// Setup a clipping rectangle for blits.
	void tglBlitScissorRect(int left, int top, int right, int bottom);
} // end of namespace Internal

} // end of namespace Graphics

#endif // GRAPHICS_TINYGL_ZBLIT_H_