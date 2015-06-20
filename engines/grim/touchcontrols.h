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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef GRIM_TOUCHCONTROLS_H_
#define GRIM_TOUCHCONTROLS_H_

#include "backends/touch/touch_impl.h"

namespace Grim {

class JoystickMode : public TouchControlsImpl {
	public:
	JoystickMode(uint32 width, uint32 height);
	~JoystickMode();
	static JoystickMode *create();

	virtual void process(const Common::Event &ev) {}
	virtual void draw();
	virtual void pointerDown(uint32 pointerId, uint32 x, uint32 y);
	virtual void pointerMove(uint32 pointerId, uint32 x, uint32 y);
	virtual void pointerUp(uint32 pointerId, uint32 x, uint32 y);

	private:
	class State;
	State *_gc;
};

}
#endif
