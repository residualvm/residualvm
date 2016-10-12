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

#ifndef STARK_UI_GAME_WINDOW_H
#define STARK_UI_GAME_WINDOW_H

#include "engines/stark/gfx/renderentry.h"

#include "engines/stark/ui/window.h"

#include "common/scummsys.h"
#include "common/rect.h"
#include "common/array.h"

namespace Stark {

class ActionMenu;
class InventoryWindow;

class GameWindow : public Window {
public:
	GameWindow(Gfx::Driver *gfx, Cursor *cursor, ActionMenu *actionMenu, InventoryWindow *inventory);
	virtual ~GameWindow() {}

	/** Clear the location dependent state */
	void reset();

	void startShake(uint32 duration, bool slow);
	void stopShake();

protected:
	void onMouseMove(const Common::Point &pos) override;
	void onClick(const Common::Point &pos) override;
	void onRightClick(const Common::Point &pos) override;
	void onDoubleClick(const Common::Point &pos) override;
	void onRender() override;

	void checkObjectAtPos(Common::Point pos, int16 selectedInventoryItem, int16 &singlePossibleAction, bool &isDefaultAction);

	ActionMenu *_actionMenu;
	InventoryWindow *_inventory;

	Gfx::RenderEntryArray _renderEntries;
	Resources::ItemVisual *_objectUnderCursor;
	Common::Point _objectRelativePosition;

	uint32 _shakeEndMillis;
	Common::Rect _originalPosition;
	uint32 _shakeFrameCount;
	uint32 _staticFrameCount;
	uint8 _staticFrameInterval;
};

} // End of namespace Stark

#endif // STARK_UI_GAME_WINDOW_H
