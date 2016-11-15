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

#ifndef STARK_RESOURCES_LOCATION_H
#define STARK_RESOURCES_LOCATION_H

#include "common/hashmap.h"
#include "common/rect.h"
#include "common/str.h"

#include "engines/stark/gfx/renderentry.h"
#include "engines/stark/resources/object.h"

namespace Stark {

namespace Formats {
class XRCReadStream;
}

namespace Resources {

class ItemVisual;
class Layer;
class ModelItem;
class Sound;

/**
 * A location is a scene of the game
 *
 * Locations contain layers. The game engine retrieves the list of renderable
 * items from the current location.
 */
class Location : public Object {
public:
	static const Type::ResourceType TYPE = Type::kLocation;

	Location(Object *parent, byte subType, uint16 index, const Common::String &name);
	virtual ~Location();

	// Resource API
	void onAllLoaded() override;
	void onGameLoop() override;

	/** Does the location have a 3D layer ? */
	bool has3DLayer();

	/** Obtain a list of render entries for all the items in the location */
	Gfx::RenderEntryArray listRenderEntries();

	/** Obtain a list of render entries for all the lights in the location */
	Gfx::LightEntryArray listLightEntries();

	/** Initialize scrolling from Camera data */
	void initScroll(const Common::Point &maxScroll);

	/** Obtain the current scroll position */
	Common::Point getScrollPosition() const;

	/** Scroll the location to the specified position if possible */
	void setScrollPosition(const Common::Point &position);

	/** Smoothly scroll to a position in 2D world coordinates */
	bool scrollToCoordinateSmooth(uint32 coordinate);

	/** Immediatly scroll the character location */
	void scrollToCharacterImmediate();

	/** Replace the currently active layer */
	void goToLayer(Layer *layer);

	/**
	 * Indicate on script driven scroll is active.
	 *
	 * This means that the location should not follow the character
	 */
	void setHasActiveScroll();

	/**
	 * Stop all script driven scrolls
	 */
	void stopAllScrolls();

	/** Tell the location to scroll to follow the character */
	void startFollowingCharacter();

	/** Tell the location not to scroll to follow the character */
	void stopFollowingCharacter();

	void scrollToCoordinateImmediate(uint32 coordinate);

	/** Get an item from its character index */
	ItemVisual *getCharacterItem(int32 character) const;

	/** Register an item as a character to the location */
	void registerCharacterItem(int32 character, ItemVisual *item);

	/** Reset animation blending for all the items in the location */
	void resetAnimationBlending();

	/** Find a stock sound by its type in the location, the level, or the global level */
	Sound *findStockSound(uint32 stockSoundType) const;

	/** Set remaining frames to rumble on this lcation */
	void setRumbleFramesRemaining(int32 rumbleFramesRemaining);

protected:
	void printData() override;

private:
	bool scrollToSmooth(const Common::Point &position, bool followCharacter);
	bool scrollToCharacter(ModelItem *item);
	Common::Point getCharacterScrollPosition(ModelItem *item);
	uint getScrollStepFollow();
	Common::Point getScrollPointFromCoordinate(uint32 coordinate) const;

	Sound *findStockSound(const Object *parent, uint32 stockSoundType) const;

	Common::Array<Layer *> _layers;
	Layer *_currentLayer;

	bool _canScroll;
	bool _hasActiveScroll;
	bool _scrollFollowCharacter;
	Common::Point _scroll;
	Common::Point _maxScroll;

	Common::HashMap<int32, ItemVisual *> _characterItemMap;

	int32 _rumbleFramesRemaining;

	uint getScrollStep();
};

} // End of namespace Resources
} // End of namespace Stark

#endif // STARK_RESOURCES_LOCATION_H
