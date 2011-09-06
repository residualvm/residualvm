/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "engines/grim/debug.h"

namespace Grim {

#define DEBUG_CHECK(level) return (gDebugLevel & (DEBUG_##level))
bool DebugNormal() { DEBUG_CHECK(NORMAL); }
bool DebugWarn() { DEBUG_CHECK(WARN); }
bool DebugError() { DEBUG_CHECK(ERROR); }
bool DebugLua() { DEBUG_CHECK(LUA); }
bool DebugBitmaps() { DEBUG_CHECK(BITMAPS); }
bool DebugModel() { DEBUG_CHECK(MODEL); }
bool DebugStub() { DEBUG_CHECK(STUB); }
bool DebugSmush() { DEBUG_CHECK(SMUSH); }
bool DebugImuse() { DEBUG_CHECK(IMUSE); }
bool DebugChores() { DEBUG_CHECK(CHORES); }
bool DebugAll() { DEBUG_CHECK(ALL); }

}
