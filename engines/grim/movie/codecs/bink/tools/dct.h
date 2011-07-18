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

// Based upon the (I)DCT code in FFmpeg
// Copyright (c) 2009 Peter Ross <pross@xvid.org>
// Copyright (c) 2010 Alex Converse <alex.converse@gmail.com>
// Copyright (c) 2010 Vitor Sessak

/** @file common/dct.h
 *  (Inverse) Discrete Cosine Transforms.
 */

#ifndef COMMON_DCT_H
#define COMMON_DCT_H

#include "common/types.h"
#include "maths.h"
#include "rdft.h"

namespace Common {

/** (Inverse) Discrete Cosine Transforms. */
class DCT {
public:
	enum TransformType {
		DCT_II,
		DCT_III,
		DCT_I,
		DST_I
	};

	DCT(int bits, TransformType trans);
	~DCT();

	void calc(float *data);

private:
	int _bits;
	TransformType _trans;

	const float *_tCos;

	float *_csc2;

	RDFT *_rdft;

	void calcDCTI  (float *data);
	void calcDCTII (float *data);
	void calcDCTIII(float *data);
	void calcDSTI  (float *data);
};

} // End of namespace Common

#endif // COMMON_DCT_H
