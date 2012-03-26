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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "engines/myst3/directorysubentry.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/nodeframe.h"
#include "engines/myst3/scene.h"
#include "engines/myst3/state.h"

namespace Myst3 {

NodeFrame::NodeFrame(Myst3Engine *vm, uint16 id) :
	Node(vm, id) {
	const DirectorySubEntry *jpegDesc = _vm->getFileDescription(0, id, 1, DirectorySubEntry::kFrame);

	if (!jpegDesc)
		jpegDesc = _vm->getFileDescription(0, id, 0, DirectorySubEntry::kFrame);

	if (!jpegDesc)
		jpegDesc = _vm->getFileDescription(0, id, 1, DirectorySubEntry::kMenuFrame);

	if (!jpegDesc)
		error("Frame %d does not exist", id);

	Common::MemoryReadStream *jpegStream = jpegDesc->getData();

	if (jpegStream) {
		Graphics::JPEGDecoder jpeg;
		if (!jpeg.loadStream(*jpegStream))
			error("Could not decoder Myst III JPEG");

		_faces[0] = new Face(_vm);
		_faces[0]->setTextureFromJPEG(&jpeg);
		_faces[0]->markTextureDirty();

		delete jpegStream;
	}
}

NodeFrame::~NodeFrame() {
}

void NodeFrame::draw() {
	Common::Rect screenRect;

	// Size and position of the frame
	if (_vm->_state->getViewType() == kMenu) {
		screenRect = Common::Rect(Renderer::kOriginalWidth, Renderer::kOriginalHeight);
	} else {
		screenRect = Common::Rect(Renderer::kOriginalWidth, Renderer::kFrameHeight);
		screenRect.translate(0, Renderer::kTopBorderHeight);
	}

	// Used fragment of texture
	Common::Rect textureRect = Common::Rect(screenRect.width(), screenRect.height());

	// Update the OpenGL texture if needed
	_faces[0]->uploadTexture();

	// Draw
	_vm->_gfx->drawTexturedRect2D(screenRect, textureRect, _faces[0]->_texture);
}

} /* namespace Myst3 */
