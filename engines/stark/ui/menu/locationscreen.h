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

#ifndef STARK_UI_MENU_LOCATION_SCREEN_H
#define STARK_UI_MENU_LOCATION_SCREEN_H

#include "engines/stark/ui/screen.h"
#include "engines/stark/ui/window.h"

namespace Stark {

namespace Gfx {
class RenderEntry;
}

namespace Resources {
class ItemVisual;
class Location;
class Sound;
}

class StaticLocationWidget;

/**
 * Abstract user interface screen using resources from a static Location sub-tree
 */
class StaticLocationScreen : public SingleWindowScreen {
public:
	StaticLocationScreen(Gfx::Driver *gfx, Cursor *cursor, const char *locationName, Screen::Name screenName);
	virtual ~StaticLocationScreen();

	// Screen API
	void open() override;
	void close() override;

protected:
	// Window API
	void onMouseMove(const Common::Point &pos) override;
	void onClick(const Common::Point &pos) override;
	void onRender() override;

	Common::Array<StaticLocationWidget *> _widgets;

private:
	const char *_locationName;
	Resources::Location *_location;
	int _hoveredWidgetIndex;

	void freeWidgets();
};

typedef Common::Functor0<void> WidgetOnClickCallback;
typedef Common::Functor2<StaticLocationWidget &, const Common::Point &, void> WidgetOnMouseMoveCallback;
#define CLICK_HANDLER(cls, method) new Common::Functor0Mem<void, cls>(this, &cls::method)
#define MOVE_HANDLER(cls, method) new Common::Functor2Mem<StaticLocationWidget &, const Common::Point &, void, cls>(this, &cls::method)

/**
 * User interface widget bound to a Location RenderEntry
 */
class StaticLocationWidget {
public:
	StaticLocationWidget(const char *renderEntryName, WidgetOnClickCallback *onClickCallback,
	                     WidgetOnMouseMoveCallback *onMouseMoveCallback);
	~StaticLocationWidget();

	/** Lookup sounds in the static location for use when hovering / clicking the widget */
	void setupSounds(int16 enterSound, int16 clickSound);

	/**
	 * Override the text color
	 *
	 * Only applies for widget referring to a RenderEntry for a text visual
	 */
	void setTextColor(uint32 textColor);

	/** For widget with no text visual, this function does nothing */
	void resetTextTexture();

	/** Draw the widget */
	void render();

	/** Widgets must be visible to be rendered and interactive */
	bool isVisible() const;
	void setVisible(bool visible);

	/** Is the specified point inside the widget? */
	bool isMouseInside(const Common::Point &mousePos) const;

	/** Per frame widget state update callback */
	void onGameLoop();

	/** Called when the mouse enters the widget */
	void onMouseEnter();

	/** Called when the mouse leaves the widget */
	void onMouseLeave();

	/** Called when the mouse hovers the widget */
	void onMouseMove(const Common::Point &mousePos);

	/** Called when the widget is clicked */
	void onClick();

private:
	Gfx::RenderEntry *_renderEntry;
	Resources::ItemVisual *_item;
	bool _visible;

	Resources::Sound *_soundMouseEnter;
	Resources::Sound *_soundMouseClick;

	WidgetOnClickCallback *_onClick;
	WidgetOnMouseMoveCallback *_onMouseMove;
};

} // End of namespace Stark

 #endif // STARK_UI_MENU_LOCATION_SCREEN_H
