/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#ifndef GRIM_IRIS_H
#define GRIM_IRIS_H

namespace AGL {
class Primitive;
}

namespace Grim {

class SaveGame;

class Iris {
public:
	enum Direction {
		Open = 0,
		Close = 1
	};

	Iris();
	~Iris();

	void play(Direction dir, int x, int y, int lenght);
	void draw();
	void update(int frameTime);

	void saveState(SaveGame *state) const;
	void restoreState(SaveGame *state);

private:
	bool _playing;
	Direction _direction;
	int _x1;
	int _y1;
	int _x2;
	int _y2;
	int _targetX;
	int _targetY;
	int _lenght;
	int _currTime;
	AGL::Primitive *_primitive;
};

} // end of namespace Grim

#endif
