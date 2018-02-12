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

#ifndef STARK_UI_ACTIONMENU_H
#define STARK_UI_ACTIONMENU_H

#include "engines/stark/ui/world/gamewindow.h"
#include "engines/stark/ui/window.h"

namespace Stark {

class Cursor;
class VisualImageXMG;
class InventoryWindow;

namespace Resources {
class ItemVisual;
}

class ActionMenu : public Window {
public:
	ActionMenu(Gfx::Driver *gfx, Cursor *cursor);
	~ActionMenu();

	void setGameWindow(GameWindow *gameWindow);

	void setInventory(InventoryWindow *inventory);

	void open(Resources::ItemVisual *item, const Common::Point &itemRelativePos);
	void close();

protected:
	Common::Rect getPosition(const Common::Point &pos);

	void onMouseMove(const Common::Point &pos) override;
	void onClick(const Common::Point &pos) override;
	void onRender() override;

private:
	enum ActionMenuType {
		kActionHand  = 0,
		kActionEye   = 1,
		kActionMouth = 2
	};

	struct ActionButton {
		bool enabled;
		uint32 action;
		Common::Rect rect;
	};

	GameWindow *_gameWindow;

	bool _fromInventory;
	ActionButton _buttons[3];

	VisualImageXMG *_background;

	Common::Point _itemRelativePos;
	Resources::ItemVisual *_item;

	InventoryWindow *_inventory;

	void clearActions();
	void enableAction(uint32 action);
};

} // End of namespace Stark

#endif
