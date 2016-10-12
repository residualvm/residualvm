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

#include "common/system.h"

#include "engines/stark/ui/gamewindow.h"

#include "engines/stark/scene.h"

#include "engines/stark/gfx/driver.h"

#include "engines/stark/resources/anim.h"
#include "engines/stark/resources/knowledgeset.h"
#include "engines/stark/resources/item.h"
#include "engines/stark/resources/location.h"

#include "engines/stark/services/global.h"
#include "engines/stark/services/services.h"
#include "engines/stark/services/staticprovider.h"
#include "engines/stark/services/gameinterface.h"
#include "engines/stark/services/userinterface.h"

#include "engines/stark/ui/actionmenu.h"
#include "engines/stark/ui/cursor.h"
#include "engines/stark/ui/inventorywindow.h"

namespace Stark {

static const int8 shakeOffsets[][2] = {
	{-1, 0},
	{0,  1},
	{0, -1},
	{1,  0},
};

GameWindow::GameWindow(Gfx::Driver *gfx, Cursor *cursor, ActionMenu *actionMenu, InventoryWindow *inventory) :
		Window(gfx, cursor),
	_actionMenu(actionMenu),
	_inventory(inventory),
	_objectUnderCursor(nullptr),
	_shakeEndMillis(0),
	_shakeFrameCount(0),
	_staticFrameCount(0),
	_staticFrameInterval(1) {
	_position = Common::Rect(Gfx::Driver::kGameViewportWidth, Gfx::Driver::kGameViewportHeight);
	_position.translate(0, Gfx::Driver::kTopBorderHeight);
	_originalPosition = _position;
	_visible = true;
}

void GameWindow::onRender() {
	// List the items to render
	Resources::Location *location = StarkGlobal->getCurrent()->getLocation();
	_renderEntries = location->listRenderEntries();
	Gfx::LightEntryArray lightEntries = location->listLightEntries();

	// Render all the scene items
	Gfx::RenderEntryArray::iterator element = _renderEntries.begin();
	while (element != _renderEntries.end()) {
		// Draw the current element
		(*element)->render(lightEntries);

		// Go for the next one
		element++;
	}

	// Setup the screen shake
	if (_shakeEndMillis) {
		uint32 currentMillis = g_system->getMillis();
		int32 remainingShakeMillis = _shakeEndMillis - currentMillis;

		if (remainingShakeMillis > 0) {
			// Hold translated position every x frames
			if (!(_shakeFrameCount % 3)) {
				_staticFrameCount++;

				// Allow shake to continue after pause
				if (!(_staticFrameCount % _staticFrameInterval)) {
					_shakeFrameCount++;
				}
			} else {
				// Translate window based on remaining time
				_shakeFrameCount++;

				uint8 shakeIndex = remainingShakeMillis % (sizeof(shakeOffsets) / 2);
				_position = _originalPosition;
				_position.grow(1);
				_position.translate(shakeOffsets[shakeIndex][0], shakeOffsets[shakeIndex][1]);
			}
		} else {
			// Toggle shake if time has elapsed
			stopShake();
		}
	}
}

void GameWindow::onMouseMove(const Common::Point &pos) {
	_renderEntries = StarkGlobal->getCurrent()->getLocation()->listRenderEntries();
	_cursor->setFading(false);

	if (!StarkUserInterface->isInteractive()) {
		_objectUnderCursor = nullptr;
		_cursor->setCursorType(Cursor::kPassive);
		_cursor->setMouseHint("");
		return;
	}

	int16 selectedInventoryItem = _inventory->getSelectedInventoryItem();
	int16 singlePossibleAction = -1;
	bool defaultAction = false;

	checkObjectAtPos(pos, selectedInventoryItem, singlePossibleAction, defaultAction);

	Common::String mouseHint;

	if (selectedInventoryItem != -1 && !defaultAction) {
		VisualImageXMG *cursorImage = StarkGameInterface->getCursorImage(selectedInventoryItem);
		_cursor->setCursorImage(cursorImage);
		_cursor->setFading(singlePossibleAction == selectedInventoryItem);
	} else if (_objectUnderCursor) {
		switch (singlePossibleAction) {
			case -1:
				_cursor->setCursorType(Cursor::kActive);
				break;
			case Resources::PATTable::kActionLook:
				_cursor->setCursorType(Cursor::kEye);
				break;
			case Resources::PATTable::kActionTalk:
				_cursor->setCursorType(Cursor::kMouth);
				break;
			case Resources::PATTable::kActionUse:
				_cursor->setCursorType(Cursor::kHand);
				break;
			default:
				VisualImageXMG *cursorImage = StarkGameInterface->getCursorImage(singlePossibleAction);
				_cursor->setCursorImage(cursorImage);
				break;
		}

		mouseHint = StarkGameInterface->getItemTitleAt(_objectUnderCursor, _objectRelativePosition);
	} else {
		// Not an object
		_cursor->setCursorType(Cursor::kDefault);
	}
	_cursor->setMouseHint(mouseHint);
}

void GameWindow::onClick(const Common::Point &pos) {
	if (!StarkUserInterface->isInteractive()) {
		return;
	}

	_actionMenu->close();

	int16 selectedInventoryItem = _inventory->getSelectedInventoryItem();
	int16 singlePossibleAction = -1;
	bool defaultAction;

	checkObjectAtPos(pos, selectedInventoryItem, singlePossibleAction, defaultAction);

	if (_objectUnderCursor) {
		if (singlePossibleAction != -1) {
			StarkGameInterface->itemDoActionAt(_objectUnderCursor, singlePossibleAction, _objectRelativePosition);
		} else if (selectedInventoryItem == -1) {
			_actionMenu->open(_objectUnderCursor, _objectRelativePosition);
		}
	} else {
		// The walk code expects unscaled absolute mouse coordinates
		StarkGameInterface->walkTo(_cursor->getMousePosition(true));
	}
}

void GameWindow::onRightClick(const Common::Point &pos) {
	if (!StarkUserInterface->isInteractive()) {
		return;
	}

	int16 selectedInventoryItem = _inventory->getSelectedInventoryItem();

	if (selectedInventoryItem == -1) {
		_inventory->open();
	} else {
		_inventory->setSelectedInventoryItem(-1);
	}
}

void GameWindow::onDoubleClick(const Common::Point &pos) {
	if (!StarkUserInterface->isInteractive()) {
		return;
	}

	if (StarkGameInterface->isAprilWalking()) {
		StarkGameInterface->setAprilRunning();
	}
}

void GameWindow::checkObjectAtPos(Common::Point pos, int16 selectedInventoryItem, int16 &singlePossibleAction, bool &isDefaultAction) {
	_objectUnderCursor = nullptr;
	singlePossibleAction = -1;
	isDefaultAction = false;

	Math::Ray ray = StarkScene->makeRayFromMouse(_cursor->getMousePosition(true));

	// Render entries are sorted from the farthest to the camera to the nearest
	// Loop in reverse order
	for (int i = _renderEntries.size() - 1; i >= 0; i--) {
		if (_renderEntries[i]->containsPoint(pos, _objectRelativePosition)
		    || _renderEntries[i]->intersectRay(ray)) {
			_objectUnderCursor = _renderEntries[i]->getOwner();
			break;
		}
	}

	if (!_objectUnderCursor || !StarkGameInterface->itemHasActionAt(_objectUnderCursor, _objectRelativePosition, -1)) {
		// Only consider items with runnable scripts
		_objectUnderCursor = nullptr;
		return;
	}

	int32 defaultAction = StarkGameInterface->itemGetDefaultActionAt(_objectUnderCursor, _objectRelativePosition);
	if (defaultAction != -1) {
		// Use the default action if there is one
		singlePossibleAction = defaultAction;
		isDefaultAction = true;
	} else if (selectedInventoryItem != -1) {
		// Use the selected inventory item if there is one
		if (StarkGameInterface->itemHasActionAt(_objectUnderCursor, _objectRelativePosition, selectedInventoryItem)) {
			singlePossibleAction = selectedInventoryItem;
		}
	} else {
		// Otherwise, use stock actions
		Resources::ActionArray actionsPossible = StarkGameInterface->listStockActionsPossibleForObjectAt(
				_objectUnderCursor, _objectRelativePosition);

		if (actionsPossible.size() == 1) {
			singlePossibleAction = actionsPossible[0];
		}
	}
}

void GameWindow::reset() {
	_renderEntries.clear();
	_objectUnderCursor = nullptr;
	_objectRelativePosition.x = 0;
	_objectRelativePosition.y = 0;

	stopShake();
}

void GameWindow::startShake(uint32 duration, bool slow) {
	uint32 currentMillis = g_system->getMillis();

	_shakeEndMillis = currentMillis + duration;
	_originalPosition = _position;
	_shakeFrameCount = 0;
	_staticFrameCount = 0;
	_staticFrameInterval = slow ? 7 : 3;
}

void GameWindow::stopShake() {
	_shakeEndMillis = 0;
	_position = _originalPosition;
}

} // End of namespace Stark
