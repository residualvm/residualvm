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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef GRIM_BLOCKY16_H
#define GRIM_BLOCKY16_H

#include "common/scummsys.h"

namespace Grim {

struct Stream;

class Blocky16 {
private:

	int _bufSize;
	int32 _frameSize;
	int _width, _height;
	int _alignedWidth, _alignedHeight;
	uint16 *_frm0, *_frm1, *_frm2;
	uint8 *_rleBuf;
	int _pitch;
	int _npixels;
	int _blocksWidth, _blocksHeight;
	int8 _4x4glyphs[256][16];
	int8 _8x8glyphs[256][64];
	uint16 _codebook[256];
	uint16 _smallCodebook[4];

	typedef bool (Blocky16::*Subcodec)(Stream *src);
	static const Subcodec subcodecs[];

	bool subcodecNop(Stream *src);
	bool subcodec0(Stream *src);
	bool subcodec2(Stream *src);
	bool subcodec3(Stream *src);
	bool subcodec4(Stream *src);
	bool subcodec5(Stream *src);
	bool subcodec6(Stream *src);
	bool subcodec8(Stream *src);

	bool subcodec2Block(Stream *src, int cx, int cy, int size);
	bool isGoodMvec(int cx, int cy, int mx, int my, int blockSize);
	bool opcode0xf7(Stream *src, int cx, int cy, int block_size);
	bool opcode0xf8(Stream *src, int cx, int cy, int block_size);
	bool drawGlyph(uint16 *dst, int index, uint16 fg_color, uint16 bg_color, int block_size);

public:
	Blocky16();
	~Blocky16();
	void init(int width, int height);
	void deinit();
	void decode(byte *dst, byte *src, int size);
};

} // end of namespace Grim

#endif
