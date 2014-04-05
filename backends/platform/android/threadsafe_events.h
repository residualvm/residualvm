/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef ANDROID_THREADSAFE_EVENTS_H
#define ANDROID_THREADSAFE_EVENTS_H

#include "common/events.h"
#include "common/system.h"
#include "backends/events/default/default-events.h"

class ThreadsafeEventManager : public DefaultEventManager {
	public:
	ThreadsafeEventManager(Common::EventSource *boss) : DefaultEventManager(boss) {}
	~ThreadsafeEventManager() { g_system->deleteMutex(_mut); }

	void init() {
		DefaultEventManager::init();
		_mut = g_system->createMutex();
	}

	bool notifyEvent(const Common::Event &event) {
		g_system->lockMutex(_mut);
		bool ret = DefaultEventManager::notifyEvent(event);
		g_system->unlockMutex(_mut);
		return ret;
	}

	bool pollEvent(Common::Event &event) {
		g_system->lockMutex(_mut);
		bool ret = DefaultEventManager::pollEvent(event);
		g_system->unlockMutex(_mut);
		return ret;
	}

	void pushEvent(const Common::Event &event) {
		g_system->lockMutex(_mut);
		DefaultEventManager::pushEvent(event);
		g_system->unlockMutex(_mut);
	}

	private:
	OSystem::MutexRef _mut;
};

#endif
