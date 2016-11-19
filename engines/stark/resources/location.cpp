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

#include "engines/stark/resources/location.h"

#include "engines/stark/formats/xrc.h"
#include "engines/stark/gfx/driver.h"

#include "engines/stark/movement/movement.h"

#include "engines/stark/resources/container.h"
#include "engines/stark/resources/item.h"
#include "engines/stark/resources/layer.h"
#include "engines/stark/resources/level.h"
#include "engines/stark/resources/scroll.h"
#include "engines/stark/resources/sound.h"

#include "engines/stark/scene.h"
#include "engines/stark/services/services.h"
#include "engines/stark/services/global.h"

#include "common/random.h"

namespace Stark {
namespace Resources {

Location::~Location() {
}

Location::Location(Object *parent, byte subType, uint16 index, const Common::String &name) :
		Object(parent, subType, index, name),
		_canScroll(false),
		_currentLayer(nullptr),
		_hasActiveScroll(false),
		_scrollFollowCharacter(false),
		_rumbleFramesRemaining(0) {
	_type = TYPE;
}

void Location::onAllLoaded() {
	Object::onAllLoaded();

	_layers = listChildren<Layer>();
}

void Location::onGameLoop() {
	Object::onGameLoop();

	if (_hasActiveScroll) {
		// Script triggered scrolling has precedence over following the character
		_scrollFollowCharacter = false;
	}

	if (_scrollFollowCharacter) {
		ModelItem *april = StarkGlobal->getCurrent()->getInteractive();
		Movement *movement = april->getMovement();

		bool scrollComplete = scrollToCharacter(april);
		if (scrollComplete && (!movement || movement->hasEnded())) {
			_scrollFollowCharacter = false;
		}
	}

	if (_rumbleFramesRemaining > 0) {
		_rumbleFramesRemaining--;
	}
}

bool Location::has3DLayer() {
	return findChildWithSubtype<Layer>(Layer::kLayer3D) != nullptr;
}

Gfx::RenderEntryArray Location::listRenderEntries() {
	Gfx::RenderEntryArray renderEntries;

	for (uint i = 0; i < _layers.size(); i++) {
		Layer *layer = _layers[i];
		if (layer->isEnabled()) {
			Common::Point baseScroll;

			if (_rumbleFramesRemaining > 0) {
				baseScroll = layer->getScroll();
				Common::Point offsetScroll = baseScroll;
				offsetScroll.x = StarkRandomSource->getRandomBit() - 1;
				offsetScroll.y = StarkRandomSource->getRandomBit() - 1;

				layer->setScroll(offsetScroll);
			}

			renderEntries.push_back(layer->listRenderEntries());

			if (_rumbleFramesRemaining > 0) {
				layer->setScroll(baseScroll);
			}
		}
	}

	return renderEntries;
}

Gfx::LightEntryArray Location::listLightEntries() {
	Gfx::LightEntry *ambient = nullptr;
	Gfx::LightEntryArray others;

	// Build a list of lights from all the layers ...
	for (uint i = 0; i < _layers.size(); i++) {
		Layer *layer = _layers[i];
		if (layer->isEnabled()) {
			Gfx::LightEntryArray layerLights = layer->listLightEntries();

			for (uint j = 0; j < layerLights.size(); j++) {
				Gfx::LightEntry *light = layerLights[j];

				// ... but store the ambient light in a separate variable ...
				if (light->type == Gfx::LightEntry::kAmbient) {
					ambient = light;
				} else {
					others.push_back(light);
				}
			}
		}
	}

	// ... so that it is first in the final light list
	Gfx::LightEntryArray lightEntries;
	lightEntries.push_back(ambient);
	lightEntries.push_back(others);
	return lightEntries;
}

void Location::initScroll(const Common::Point &maxScroll) {
	_maxScroll = maxScroll;
	_canScroll = _maxScroll.x != 0 || _maxScroll.y != 0;
}

Common::Point Location::getScrollPosition() const {
	return _scroll;
}

void Location::setScrollPosition(const Common::Point &position) {
	_scroll.x = CLIP<int16>(position.x, 0, _maxScroll.x);
	_scroll.y = CLIP<int16>(position.y, 0, _maxScroll.y);


	// Setup the layers scroll position
	for (uint i = 0; i < _layers.size(); i++) {
		_layers[i]->setScrollPosition(_scroll);
	}

	// Reconfigure the camera
	Common::Rect viewport(Gfx::Driver::kGameViewportWidth, Gfx::Driver::kGameViewportHeight);
	viewport.translate(_scroll.x, _scroll.y);
	StarkScene->scrollCamera(viewport);
}

Common::Point Location::getCharacterScrollPosition(ModelItem *item) {
	// TODO: Use April's 2D bounding box
	Common::Point position2D = StarkScene->convertPosition3DToGameScreenOriginal(item->getPosition3D());


	Common::Point newScroll;
	if (_maxScroll.x > 0) {
		newScroll.x = _scroll.x + position2D.x - Gfx::Driver::kGameViewportWidth / 2;
		newScroll.y = _scroll.y;
	} else {
		newScroll.x = _scroll.x;
		newScroll.y = _scroll.y + position2D.y - Gfx::Driver::kGameViewportHeight / 2;
	}

	return newScroll;
}

bool Location::scrollToCharacter(ModelItem *item) {
	if (!_canScroll) {
		return true;
	}

	Common::Point newScroll = getCharacterScrollPosition(item);
	if (_maxScroll.x > 0) {
		if (newScroll.x < _scroll.x - 15 || newScroll.x > _scroll.x + 15) {
			newScroll.x = CLIP<int16>(newScroll.x, 0, _maxScroll.x);
			return scrollToSmooth(newScroll, true);
		}
	} else {
		if (newScroll.y < _scroll.y - 15 || newScroll.y > _scroll.y + 15) {
			newScroll.y = CLIP<int16>(newScroll.y, 0, _maxScroll.y);
			return scrollToSmooth(newScroll, true);
		}
	}

	return false;
}

void Location::scrollToCharacterImmediate() {
	if (!_canScroll) {
		return;
	}

	ModelItem *april = StarkGlobal->getCurrent()->getInteractive();
	setScrollPosition(getCharacterScrollPosition(april));
}

uint Location::getScrollStepFollow() {
	ModelItem *april = StarkGlobal->getCurrent()->getInteractive();
	Common::Point position2D = StarkScene->convertPosition3DToGameScreenOriginal(april->getPosition3D());

	// TODO: Complete

	uint scrollStep;
	if (_maxScroll.x > 0) {
		scrollStep = abs((Gfx::Driver::kGameViewportWidth / 2 - position2D.x) / 16);
	} else {
		scrollStep = abs((Gfx::Driver::kGameViewportHeight / 2 - position2D.y) / 16);
	}

	return CLIP<uint>(scrollStep, 1, 4);
}

uint Location::getScrollStep() {
	uint scrollStep;
	if (_maxScroll.x > 0) {
		if (_scroll.x <= _maxScroll.x / 2) {
			scrollStep = _scroll.x / 16;
		} else {
			scrollStep = (_maxScroll.x - _scroll.x) / 16;
		}
	} else {
		if (_scroll.y <= _maxScroll.y / 2) {
			scrollStep = _scroll.y / 16;
		} else {
			scrollStep = (_maxScroll.y - _scroll.y) / 16;
		}
	}

	return CLIP<uint>(scrollStep, 1, 4);
}

bool Location::scrollToSmooth(const Common::Point &position, bool followCharacter) {
	uint scrollStep;
	if (followCharacter) {
		scrollStep = getScrollStepFollow();
	} else {
		scrollStep = getScrollStep();
	}

	Common::Point delta;
	if (position.x < _scroll.x) {
		delta.x = -scrollStep;
		delta.x = CLIP<int16>(delta.x, position.x - _scroll.x, 0);
	} else if (position.x > _scroll.x) {
		delta.x = scrollStep;
		delta.x = CLIP<int16>(delta.x, 0, position.x - _scroll.x);
	}

	if (position.y < _scroll.y) {
		delta.y = -scrollStep;
		delta.y = CLIP<int16>(delta.y, position.y - _scroll.y, 0);
	} else if (position.y > _scroll.y) {
		delta.y = scrollStep;
		delta.y = CLIP<int16>(delta.y, 0, position.y - _scroll.y);
	}

	if (delta.x == 0 && delta.y == 0) {
		// We already are at the target position, scrolling has completed
		return true;
	}

	setScrollPosition(_scroll + delta);
	return false;
}

bool Location::scrollToCoordinateSmooth(uint32 coordinate) {
	Common::Point newScroll = getScrollPointFromCoordinate(coordinate);
	return scrollToSmooth(newScroll, false);
}

void Location::scrollToCoordinateImmediate(uint32 coordinate) {
	Common::Point newScroll = getScrollPointFromCoordinate(coordinate);
	return setScrollPosition(newScroll);
}

Common::Point Location::getScrollPointFromCoordinate(uint32 coordinate) const {
	Common::Point newScroll = _scroll;

	if (_maxScroll.x > 0) {
		newScroll.x = coordinate;
	} else {
		newScroll.y = coordinate;
	}

	return newScroll;
}

void Location::stopFollowingCharacter() {
	_scrollFollowCharacter = false;
}

void Location::startFollowingCharacter() {
	_scrollFollowCharacter = true;
}

void Location::setHasActiveScroll() {
	_hasActiveScroll = true;
}

void Location::stopAllScrolls() {
	Common::Array<Scroll *> scrolls = listChildrenRecursive<Scroll>();
	for (uint i = 0; i < scrolls.size(); i++) {
		scrolls[i]->stop();
	}

	_hasActiveScroll = false;
}

void Location::goToLayer(Layer *layer) {
	if (_currentLayer) {
		_currentLayer->enable(false);
	}

	layer->enable(true);
	_currentLayer = layer;
}

ItemVisual *Location::getCharacterItem(int32 character) const {
	return _characterItemMap.getVal(character, nullptr);
}

void Location::registerCharacterItem(int32 character, ItemVisual *item) {
	if (character >= 0) {
		_characterItemMap[character] = item;
	}
}

void Location::printData() {
}

void Location::resetAnimationBlending() {
	Common::Array<ModelItem *> items = listChildren<ModelItem>(Item::kItemModel);
	for (uint i = 0; i < items.size(); i++) {
		items[i]->resetAnimationBlending();
	}
}

Sound *Location::findStockSound(uint32 stockSoundType) const {
	Sound *sound = findStockSound(this, stockSoundType);

	if (!sound) {
		Level *currentLevel = StarkGlobal->getCurrent()->getLevel();
		sound = findStockSound(currentLevel, stockSoundType);
	}

	if (!sound) {
		Level *globalLevel = StarkGlobal->getLevel();
		sound = findStockSound(globalLevel, stockSoundType);
	}

	return sound;
}

Sound *Location::findStockSound(const Object *parent, uint32 stockSoundType) const {
	Container *stockSoundContainer = parent->findChildWithSubtype<Container>(Container::kStockSounds);
	if (stockSoundContainer) {
		Common::Array<Sound *> stockSounds = stockSoundContainer->listChildren<Sound>(Sound::kSoundStock);

		for (uint i = 0; i < stockSounds.size(); i++) {
			Sound *sound = stockSounds[i];
			if (sound->getStockSoundType() == stockSoundType) {
				return sound;
			}
		}
	}

	return nullptr;
}

void Location::setRumbleFramesRemaining(int32 rumbleFramesRemaining) {
	_rumbleFramesRemaining = rumbleFramesRemaining;
}

} // End of namespace Resources
} // End of namespace Stark
