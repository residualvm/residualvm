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


/* This implementation is heavily based on FFmpeg's sanm.c file,
 * its license header is below. */

/*
 * LucasArts Smush video decoder
 * Copyright (c) 2006 Cyril Zorin
 * Copyright (c) 2011 Konstantin Shishkov
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common/endian.h"
#include "common/util.h"
#include "common/textconsole.h"

#include "engines/grim/movie/codecs/blocky16.h"

namespace Grim {

#if defined(SCUMM_NEED_ALIGNMENT)

#define COPY_4X1_LINE(dst, src)			\
	do {					\
		(dst)[0] = (src)[0];	\
		(dst)[1] = (src)[1];	\
		(dst)[2] = (src)[2];	\
		(dst)[3] = (src)[3];	\
	} while (0)

#if defined(SCUMM_BIG_ENDIAN)

#define WRITE_2X1_LINE(dst, v)		\
	do {				\
		(dst)[0] = (byte)((v >> 8) & 0xFF);	\
		(dst)[1] = (byte)((v >> 0) & 0xFF);	\
	} while (0)

#define WRITE_4X1_LINE(dst, v)		\
	do {				\
		(dst)[0] = (byte)((v >> 24) & 0xFF);	\
		(dst)[1] = (byte)((v >> 16) & 0XFF);	\
		(dst)[2] = (byte)((v >>  8) & 0xFF);	\
		(dst)[3] = (byte)((v >>  0) & 0xFF);	\
	} while (0)

#else /* SCUMM_BIG_ENDIAN */

#define WRITE_2X1_LINE(dst, v)		\
	do {				\
		(dst)[0] = (byte)((v >> 0) & 0xFF);	\
		(dst)[1] = (byte)((v >> 8) & 0xFF);	\
	} while (0)

#define WRITE_4X1_LINE(dst, v)		\
	do {				\
		(dst)[0] = (byte)((v >>  0) & 0xFF);	\
		(dst)[1] = (byte)((v >>  8) & 0XFF);	\
		(dst)[2] = (byte)((v >> 16) & 0xFF);	\
		(dst)[3] = (byte)((v >> 24) & 0xFF);	\
	} while (0)

#endif

#else /* SCUMM_NEED_ALIGNMENT */

#define COPY_4X1_LINE(dst, src)			\
	*(uint32 *)(dst) = *(const uint32 *)(src);

#define WRITE_2X1_LINE(dst, v)		\
	*(uint16 *)(dst) = v;

#define WRITE_4X1_LINE(dst, v)		\
	*(uint32 *)(dst) = v;

#endif

static int8 xvector4[] = {
	0, 1, 2, 3, 3, 3, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1,
};

static int8 yvector4[] = {
	0, 0, 0, 0, 1, 2, 3, 3, 3, 3, 2, 1, 1, 1, 2, 2,
};

static int8 xvector8[] = {
	0, 2, 5, 7, 7, 7, 7, 7, 7, 5, 2, 0, 0, 0, 0, 0,
};

static int8 yvector8[] = {
	0, 0, 0, 0, 1, 3, 4, 6, 7, 7, 7, 7, 6, 4, 3, 1,
};

static int8 motion_vectors[256][2] = {
{  0,   0}, { -1, -43}, {  6, -43}, { -9, -42}, { 13, -41},
{-16, -40}, { 19, -39}, {-23, -36}, { 26, -34}, { -2, -33},
{  4, -33}, {-29, -32}, { -9, -32}, { 11, -31}, {-16, -29},
{ 32, -29}, { 18, -28}, {-34, -26}, {-22, -25}, { -1, -25},
{  3, -25}, { -7, -24}, {  8, -24}, { 24, -23}, { 36, -23},
{-12, -22}, { 13, -21}, {-38, -20}, {  0, -20}, {-27, -19},
{ -4, -19}, {  4, -19}, {-17, -18}, { -8, -17}, {  8, -17},
{ 18, -17}, { 28, -17}, { 39, -17}, {-12, -15}, { 12, -15},
{-21, -14}, { -1, -14}, {  1, -14}, {-41, -13}, { -5, -13},
{  5, -13}, { 21, -13}, {-31, -12}, {-15, -11}, { -8, -11},
{  8, -11}, { 15, -11}, { -2, -10}, {  1, -10}, { 31, -10},
{-23,  -9}, {-11,  -9}, { -5,  -9}, {  4,  -9}, { 11,  -9},
{ 42,  -9}, {  6,  -8}, { 24,  -8}, {-18,  -7}, { -7,  -7},
{ -3,  -7}, { -1,  -7}, {  2,  -7}, { 18,  -7}, {-43,  -6},
{-13,  -6}, { -4,  -6}, {  4,  -6}, {  8,  -6}, {-33,  -5},
{ -9,  -5}, { -2,  -5}, {  0,  -5}, {  2,  -5}, {  5,  -5},
{ 13,  -5}, {-25,  -4}, { -6,  -4}, { -3,  -4}, {  3,  -4},
{  9,  -4}, {-19,  -3}, { -7,  -3}, { -4,  -3}, { -2,  -3},
{ -1,  -3}, {  0,  -3}, {  1,  -3}, {  2,  -3}, {  4,  -3},
{  6,  -3}, { 33,  -3}, {-14,  -2}, {-10,  -2}, { -5,  -2},
{ -3,  -2}, { -2,  -2}, { -1,  -2}, {  0,  -2}, {  1,  -2},
{  2,  -2}, {  3,  -2}, {  5,  -2}, {  7,  -2}, { 14,  -2},
{ 19,  -2}, { 25,  -2}, { 43,  -2}, { -7,  -1}, { -3,  -1},
{ -2,  -1}, { -1,  -1}, {  0,  -1}, {  1,  -1}, {  2,  -1},
{  3,  -1}, { 10,  -1}, { -5,   0}, { -3,   0}, { -2,   0},
{ -1,   0}, {  1,   0}, {  2,   0}, {  3,   0}, {  5,   0},
{  7,   0}, {-10,   1}, { -7,   1}, { -3,   1}, { -2,   1},
{ -1,   1}, {  0,   1}, {  1,   1}, {  2,   1}, {  3,   1},
{-43,   2}, {-25,   2}, {-19,   2}, {-14,   2}, { -5,   2},
{ -3,   2}, { -2,   2}, { -1,   2}, {  0,   2}, {  1,   2},
{  2,   2}, {  3,   2}, {  5,   2}, {  7,   2}, { 10,   2},
{ 14,   2}, {-33,   3}, { -6,   3}, { -4,   3}, { -2,   3},
{ -1,   3}, {  0,   3}, {  1,   3}, {  2,   3}, {  4,   3},
{ 19,   3}, { -9,   4}, { -3,   4}, {  3,   4}, {  7,   4},
{ 25,   4}, {-13,   5}, { -5,   5}, { -2,   5}, {  0,   5},
{  2,   5}, {  5,   5}, {  9,   5}, { 33,   5}, { -8,   6},
{ -4,   6}, {  4,   6}, { 13,   6}, { 43,   6}, {-18,   7},
{ -2,   7}, {  0,   7}, {  2,   7}, {  7,   7}, { 18,   7},
{-24,   8}, { -6,   8}, {-42,   9}, {-11,   9}, { -4,   9},
{  5,   9}, { 11,   9}, { 23,   9}, {-31,  10}, { -1,  10},
{  2,  10}, {-15,  11}, { -8,  11}, {  8,  11}, { 15,  11},
{ 31,  12}, {-21,  13}, { -5,  13}, {  5,  13}, { 41,  13},
{ -1,  14}, {  1,  14}, { 21,  14}, {-12,  15}, { 12,  15},
{-39,  17}, {-28,  17}, {-18,  17}, { -8,  17}, {  8,  17},
{ 17,  18}, { -4,  19}, {  0,  19}, {  4,  19}, { 27,  19},
{ 38,  20}, {-13,  21}, { 12,  22}, {-36,  23}, {-24,  23},
{ -8,  24}, {  7,  24}, { -3,  25}, {  1,  25}, { 22,  25},
{ 34,  26}, {-18,  28}, {-32,  29}, { 16,  29}, {-11,  31},
{  9,  32}, { 29,  32}, { -4,  33}, {  2,  33}, {-26,  34},
{ 23,  36}, {-19,  39}, { 16,  40}, {-13,  41}, {  9,  42},
{ -6,  43}, {  1,  43}, {  0,   0}, {  0,   0}, {  0,   0}
};

#define ALIGN(x, a) (((x)+(a)-1)&~((a)-1))

struct Stream {
	byte *data;
	byte *end;

	Stream(byte *src, int size)
		: data(src)
		, end(src + size)
	{
	}

	inline int bytesLeft() const { return end - data; }
	inline void skip(int n) { data += n; }

	inline uint32 readLEU32() {
		uint32 v = READ_LE_UINT32(data);
		data += 4;
		return v;
	}

	inline uint16 readLEU16() {
		uint16 v = READ_LE_UINT16(data);
		data += 2;
		return v;
	}

	inline uint8 readLEU8() {
		uint8 v = *data;
		data += 1;
		return v;
	}

	inline uint32 readBuffer(byte *dst, uint32 size) {
		memcpy(dst, data, size);
		data += size;
		return size;
	}
};

enum GlyphEdge {
	LeftEdge,
	TopEdge,
	RightEdge,
	BottomEdge,
	NoEdge
};

enum GlyphDir {
	LeftDir,
	UpDir,
	RightDir,
	DownDir,
	NoDir
};

/**
* Return enum GlyphEdge of box where point (x, y) lies.
*
* @param x x point coordinate
* @param y y point coordinate
* @param edgeSize box width/height.
*/
static GlyphEdge whichEdge(int x, int y, int edgeSize) {
	const int edgeMax = edgeSize - 1;

	if (!y)
		return BottomEdge;
	else if (y == edgeMax)
		return TopEdge;
	else if (!x)
		return LeftEdge;
	else if (x == edgeMax)
		return RightEdge;
	else
		return NoEdge;
}

static GlyphDir whichDirection(GlyphEdge edge0, GlyphEdge edge1) {
	if ((edge0 == LeftEdge && edge1 == RightEdge) ||
		(edge1 == LeftEdge && edge0 == RightEdge) ||
		(edge0 == BottomEdge && edge1 != TopEdge) ||
		(edge1 == BottomEdge && edge0 != TopEdge))
		return UpDir;
	else if ((edge0 == TopEdge && edge1 != BottomEdge) ||
			(edge1 == TopEdge && edge0 != BottomEdge))
		return DownDir;
	else if ((edge0 == LeftEdge && edge1 != RightEdge) ||
			(edge1 == LeftEdge && edge0 != RightEdge))
		return LeftDir;
	else if ((edge0 == TopEdge && edge1 == BottomEdge) ||
			(edge1 == TopEdge && edge0 == BottomEdge) ||
			(edge0 == RightEdge && edge1 != LeftEdge) ||
			(edge1 == RightEdge && edge0 != LeftEdge))
		return RightDir;

	return NoDir;
}

/* Interpolate two points. */
static void interpolatePoint(int8 *points, int x0, int y0, int x1, int y1, int pos, int npoints) {
	if (npoints) {
		points[0] = (x0 * pos + x1 * (npoints - pos) + (npoints >> 1)) / npoints;
		points[1] = (y0 * pos + y1 * (npoints - pos) + (npoints >> 1)) / npoints;
	} else {
		points[0] = x0;
		points[1] = y0;
	}
}

/**
* Construct glyphs by iterating through vector coordinates.
*
* @param pglyphs pointer to table where glyphs are stored
* @param xvec pointer to x component of vector coordinates
* @param yvec pointer to y component of vector coordinates
* @param sideLength glyph width/height.
*/
static void makeGlyphs(int8 *glyphs, int8 *xvec, int8 *yvec, int sideLength) {
	const int glyphSize = sideLength * sideLength;
	const int GLYPH_COORD_VECT_SIZE = 16;

	for (int i = 0; i < GLYPH_COORD_VECT_SIZE; i++) {
		int x0 = xvec[i];
		int y0 = yvec[i];
		GlyphEdge edge0 = whichEdge(x0, y0, sideLength);

		for (int j = 0; j < GLYPH_COORD_VECT_SIZE; j++, glyphs += glyphSize) {
			int x1 = xvec[j];
			int y1 = yvec[j];
			GlyphEdge edge1 = whichEdge(x1, y1, sideLength);
			GlyphDir dir = whichDirection(edge0, edge1);
			int npoints = MAX(ABS(x1 - x0), ABS(y1 - y0));

			for (int ipoint = 0; ipoint <= npoints; ipoint++) {
				int8 point[2];

				interpolatePoint(point, x0, y0, x1, y1, ipoint, npoints);

				switch (dir) {
				case UpDir:
					for (int irow = point[1]; irow >= 0; irow--) {
						glyphs[point[0] + irow * sideLength] = 1;
					}
					break;

				case DownDir:
					for (int irow = point[1]; irow < sideLength; irow++) {
						glyphs[point[0] + irow * sideLength] = 1;
					}
					break;

				case LeftDir:
					for (int icol = point[0]; icol >= 0; icol--) {
						glyphs[icol + point[1] * sideLength] = 1;
					}
					break;

				case RightDir:
					for (int icol = point[0]; icol < sideLength; icol++) {
						glyphs[icol + point[1] * sideLength] = 1;
					}
					break;
				default:
					break;
				}
			}
		}
	}
}

Blocky16::Blocky16()
		: _frm0(nullptr)
		, _frm1(nullptr)
		, _frm2(nullptr) {
	memset(_4x4glyphs[0], 0, sizeof(_4x4glyphs));
	memset(_8x8glyphs[0], 0, sizeof(_8x8glyphs));
	makeGlyphs(_4x4glyphs[0], xvector4, yvector4, 4);
	makeGlyphs(_8x8glyphs[0], xvector8, yvector8, 8);
	_height = _width = 0;
	_frameSize = 0;
}

Blocky16::~Blocky16() {
	deinit();
}

void Blocky16::init(int width, int height) {
	deinit();

	_width = width;
	_height = height;
	_npixels = width * height;
	_alignedWidth = ALIGN(width, 8);
	_alignedHeight = ALIGN(height, 8);
	_bufSize = _alignedWidth * _alignedHeight * sizeof(_frm0[0]);
	_pitch = width;
	_frameSize = _width * _height * 2;

	_frm0 = new uint16[_bufSize];
	_frm1 = new uint16[_bufSize];
	_frm2 = new uint16[_bufSize];
}

void Blocky16::deinit() {
	delete[] _frm0;
	delete[] _frm1;
	delete[] _frm2;
	_frm0 = _frm1 = _frm2 = nullptr;
}

static int rleDecode(Stream *src, uint8 *dst, const int out_size) {
	int left = out_size;

	while (left > 0) {
		int opcode = src->readLEU8();
		int run_len = (opcode >> 1) + 1;
		if (run_len > left || src->bytesLeft() <= 0) {
			return false;
		}

		if (opcode & 1) {
			int color = src->readLEU8();
			memset(dst, color, run_len);
		} else {
			if (src->bytesLeft() < run_len) {
				return false;
			}
			src->readBuffer(dst, run_len);
		}

		dst  += run_len;
		left -= run_len;
	}

	return true;
}

static void fillFrame(uint16 *pbuf, int size, uint16 color) {
	while (size--) {
		*pbuf++ = color;
	}
}

static inline void copyBlock2(uint8 *dst, const uint8 *src, int stride) {
	for (int i = 0; i < 2; i++) {
		COPY_4X1_LINE(dst +  0, src +  0);
		dst += stride;
		src += stride;
	}
}

static inline void copyBlock4(uint8 *dst, const uint8 *src, int stride) {
	for (int i = 0; i < 4; i++) {
		COPY_4X1_LINE(dst +  0, src +  0);
		COPY_4X1_LINE(dst +  4, src +  4);
		dst += stride;
		src += stride;
	}
}

static inline void copyBlock8(uint8 *dst, const uint8 *src, int stride) {
	for (int i = 0; i < 8; i++) {
		COPY_4X1_LINE(dst +  0, src +  0);
		COPY_4X1_LINE(dst +  4, src +  4);
		COPY_4X1_LINE(dst +  8, src +  8);
		COPY_4X1_LINE(dst + 12, src + 12);
		dst += stride;
		src += stride;
	}
}

static void copyBlock(uint16 *pdest, uint16 *psrc, int block_size, int pitch) {
	uint8 *dst = (uint8 *)pdest;
	uint8 *src = (uint8 *)psrc;
	int stride = pitch * 2;

	switch (block_size) {
	case 2:
		copyBlock2(dst, src, stride);
		break;
	case 4:
		copyBlock4(dst, src, stride);
		break;
	case 8:
		copyBlock8(dst, src, stride);
		break;
	}
}

static void fillBlock(uint16 *pdest, uint16 color, int block_size, int pitch) {
	pitch -= block_size;
	for (int y = 0; y < block_size; y++, pdest += pitch) {
		for (int x = 0; x < block_size; x++) {
			*pdest++ = color;
		}
	}
}

bool Blocky16::isGoodMvec(int cx, int cy, int mx, int my, int blockSize) {
	int startPos = cx + mx + (cy + my) * _pitch;
	int endPos = startPos + (blockSize - 1) * (_pitch + 1);

	bool good = startPos >= 0 && endPos < (_bufSize >> 1);

	if (!good) {
		warning("Blocky16, subcodec 2: ignoring invalid motion vector (%i, %i)->(%u, %u), block size = %u",
			cx + mx, cy + my, cx, cy, blockSize);
	}
	return good;
}

bool Blocky16::drawGlyph(uint16 *dst, int index, uint16 fg_color, uint16 bg_color, int block_size) {
	if (index >= 256) {
		warning("Ignoring nonexistent glyph #%u", index);
		return false;
	}

	int8 *pglyph = block_size == 8 ? _8x8glyphs[index] : _4x4glyphs[index];
	int pitch = _pitch - block_size;

	uint16 colors[2] = { fg_color, bg_color };
	for (int y = 0; y < block_size; y++, dst += pitch) {
		for (int x = 0; x < block_size; x++) {
			*dst++ = colors[*pglyph++];
		}
	}
	return true;
}

bool Blocky16::opcode0xf7(Stream *src, int cx, int cy, int block_size) {
	uint16 *dst = _frm0 + cx + cy * _pitch;

	if (block_size == 2) {
		if (src->bytesLeft() < 4) {
			warning("Blocky16, subcodec 2: invalid data");
			return false;
		}

		uint32 indices = src->readLEU32();
		dst[0]         = _codebook[indices & 0xFF];
		indices      >>= 8;
		dst[1]         = _codebook[indices & 0xFF];
		indices      >>= 8;
		dst[_pitch]     = _codebook[indices & 0xFF];
		indices      >>= 8;
		dst[_pitch + 1] = _codebook[indices & 0xFF];
	} else {
		if (src->bytesLeft() < 3) {
			warning("Blocky16, subcodec 2: invalid data");
			return false;
		}

		uint8 glyph = src->readLEU8();
		uint16 bgcolor = _codebook[src->readLEU8()];
		uint16 fgcolor = _codebook[src->readLEU8()];

		drawGlyph(dst, glyph, fgcolor, bgcolor, block_size);
	}
	return true;
}

bool Blocky16::opcode0xf8(Stream *src, int cx, int cy, int block_size) {
	uint16 *dst = _frm0 + cx + cy * _pitch;

	if (block_size == 2) {
		if (src->bytesLeft() < 8) {
			warning("Blocky16, subcodec 2: invalid data");
			return false;
		}

		dst[0]         = src->readLEU16();
		dst[1]         = src->readLEU16();
		dst[_pitch]     = src->readLEU16();
		dst[_pitch + 1] = src->readLEU16();
	} else {
		if (src->bytesLeft() < 5) {
			warning("Blocky16, subcodec 2: invalid data");
			return false;
		}

		uint8 glyph   = src->readLEU8();
		uint16 bgcolor = src->readLEU16();
		uint16 fgcolor = src->readLEU16();

		drawGlyph(dst, glyph, fgcolor, bgcolor, block_size);
	}
	return true;
}

bool Blocky16::subcodec2Block(Stream *src, int cx, int cy, int blockSize) {
	if (src->bytesLeft() < 1) {
		warning("Blocky16, subcodec 2: invalid data");
		return false;
	}

	uint8 opcode = src->readLEU8();
	switch (opcode) {
		default: {
			if (opcode > 0xF4) {
				warning("Blocky16, subcodec 2: unknown opcode %d", opcode);
				return false;
			}
			int16 mx = motion_vectors[opcode][0];
			int16 my = motion_vectors[opcode][1];
			if (isGoodMvec(cx, cy, mx, my, blockSize)) {
				copyBlock(_frm0 + cx + _pitch * cy, _frm2 + cx + mx + _pitch * (cy + my), blockSize, _pitch);
			}
		} break;
		case 0xF5: {
			if (src->bytesLeft() < 2) {
				warning("Blocky16, subcodec 2: invalid data");
				return false;
			}
			int16 index = src->readLEU16();

			int16 mx = index % _width;
			int16 my = index / _width;

			if (isGoodMvec(cx, cy, mx, my, blockSize)) {
				copyBlock(_frm0 + cx + _pitch * cy, _frm2 + cx + mx + _pitch * (cy + my), blockSize, _pitch);
			}
		} break;
		case 0xF6:
			copyBlock(_frm0 + cx + _pitch * cy, _frm1 + cx + _pitch * cy, blockSize, _pitch);
			break;
		case 0xF7:
			opcode0xf7(src, cx, cy, blockSize);
			break;
		case 0xF8:
			opcode0xf8(src, cx, cy, blockSize);
			break;
		case 0xF9:
		case 0xFA:
		case 0xFB:
		case 0xFC:
			fillBlock(_frm0 + cx + cy * _pitch, _smallCodebook[opcode - 0xf9], blockSize, _pitch);
			break;
		case 0xFD:
			if (src->bytesLeft() < 1) {
				warning("Blocky16, subcodec 2: invalid data");
				return false;
			}
			fillBlock(_frm0 + cx + cy * _pitch, _codebook[src->readLEU8()], blockSize, _pitch);
			break;
		case 0xFE:
			if (src->bytesLeft() < 2) {
				warning("Blocky16, subcodec 2: invalid data");
				return false;
			}
			fillBlock(_frm0 + cx + cy * _pitch, src->readLEU16(), blockSize, _pitch);
			break;
		case 0xFF:
			if (blockSize == 2) {
				opcode0xf8(src, cx, cy, blockSize);
			} else {
				blockSize >>= 1;
				if (!subcodec2Block(src, cx, cy, blockSize)) {
					return false;
				}
				if (!subcodec2Block(src, cx + blockSize, cy, blockSize)) {
					return false;
				}
				if (!subcodec2Block(src, cx, cy + blockSize, blockSize)) {
					return false;
				}
				if (!subcodec2Block(src, cx + blockSize, cy + blockSize, blockSize)) {
					return false;
				}
			}
			break;
	}

	return true;
}

bool Blocky16::subcodecNop(Stream *) {
	return false;
}

bool Blocky16::subcodec0(Stream *src) {
	if (src->bytesLeft() < _width * _height * 2) {
		warning("Blocky16, subcodec 0: insufficient data for raw frame");
		return false;
	}

	uint16 *frm = _frm0;
	for (int y = 0; y < _height; y++) {
		for (int x = 0; x < _width; x++) {
			frm[x] = src->readLEU16();
		}
		frm += _pitch;
	}
	return true;
}

bool Blocky16::subcodec2(Stream *src) {
	for (int cy = 0; cy < _alignedHeight; cy += 8) {
		for (int cx = 0; cx < _alignedWidth; cx += 8) {
			if (!subcodec2Block(src, cx, cy, 8)) {
				return false;
			}
		}
	}
	return true;
}

bool Blocky16::subcodec3(Stream *) {
	memcpy(_frm0, _frm2, _bufSize);
	return true;
}

bool Blocky16::subcodec4(Stream *) {
	memcpy(_frm0, _frm1, _bufSize);
	return true;
}

bool Blocky16::subcodec5(Stream *src) {
	uint8 *dst = (uint8*)_frm0;

	if (!rleDecode(src, dst, _bufSize)) {
		return false;
	}

#if SCUMM_BIG_ENDIAN
	int npixels = _npixels;
	uint16 *frm = _frm0;
	while (npixels--) {
		uint16 v = *frm;
		*frm = (v >> 8) | (v << 8);
		frm++;
	}
#endif

	return true;
}

bool Blocky16::subcodec6(Stream *src) {
	if (src->bytesLeft() < _npixels) {
		warning("Blocky16, subcodec 6: insufficient data for frame");
		return false;
	}

	int npixels = _npixels;
	uint16 *frm = _frm0;
	while (npixels--) {
		*frm++ = _codebook[src->readLEU8()];
	}

	return true;
}

bool Blocky16::subcodec8(Stream *src) {
	_rleBuf = new uint8[_npixels];
	if (!rleDecode(src, _rleBuf, _npixels)) {
		return false;
	}

	uint16 *pdest = _frm0;
	long npixels = _npixels;
	uint8 *rsrc = _rleBuf;
	while (npixels--) {
		*pdest++ = _codebook[*rsrc++];
	}

	return true;
}

const Blocky16::Subcodec Blocky16::subcodecs[] = {
	&Blocky16::subcodec0,
	&Blocky16::subcodecNop,
	&Blocky16::subcodec2,
	&Blocky16::subcodec3,
	&Blocky16::subcodec4,
	&Blocky16::subcodec5,
	&Blocky16::subcodec6,
	&Blocky16::subcodecNop,
	&Blocky16::subcodec8
};

void Blocky16::decode(byte *dst, byte *data, int size) {
	if (size < 560) {
		warning("Block16: input frame too short (%d bytes)", size);
		return;
	}

	Stream src(data, size);

	src.skip(8); //skip padding

	uint32 width = src.readLEU32();
	uint32 height = src.readLEU32();
	if ((int)width != _width || (int)height != _height) {
		warning("Blocky16: frame size different than expected");
		return;
	}

	uint16 seq_nb = src.readLEU16();
	uint8 codec = src.readLEU8();
	uint8 rotateCode = src.readLEU8();

	src.skip(4); //padding

	for (int i = 0; i < 4; i++) {
		_smallCodebook[i] = src.readLEU16();
	}
	uint16 bgColor = src.readLEU16();

	src.skip(2); //padding

	/*uint32 rleOutputSize =*/ src.readLEU32(); //unused
	for (int i = 0; i < 256; i++) {
		_codebook[i] = src.readLEU16();
	}

	src.skip(8); //padding;

	if (seq_nb == 0) { //keyframe
		fillFrame(_frm1, _npixels, bgColor);
		fillFrame(_frm2, _npixels, bgColor);
	}

	if (codec < sizeof(subcodecs) / sizeof(subcodecs[0])) {
		if (!(this->*subcodecs[codec])(&src)) {
			warning("Blocky16: subcodec %d: error decoding frame", codec);
			return;
		}
	} else {
		warning("Blocky16: unknown subcodec code %d", codec);
		return;
	}

	memcpy(dst, _frm0, _frameSize);

	if (rotateCode) {
		if (rotateCode == 2) {
			uint16 *tmp = _frm2;
			_frm2 = _frm1;
			_frm1 = tmp;
		}
		uint16 *tmp = _frm0;
		_frm0 = _frm2;
		_frm2 = tmp;
	}
}

} // end of namespace Grim
