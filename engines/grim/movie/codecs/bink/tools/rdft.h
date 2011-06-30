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

// Based upon the (I)RDFT code in FFmpeg
// Copyright (c) 2009 Alex Converse <alex dot converse at gmail dot com>

/** @file common/rdft.h
 *  (Inverse) Real Discrete Fourier Transform.
 */

#ifndef COMMON_RDFT_H
#define COMMON_RDFT_H

#include "common/types.h"
#include "maths.h"
#include "fft.h"

namespace Common {

/** (Inverse) Real Discrete Fourier Transform. */
class RDFT {
public:
	enum TransformType {
		DFT_R2C,
		IDFT_C2R,
		IDFT_R2C,
		DFT_C2R
	};

	RDFT(int bits, TransformType trans);
	~RDFT();

	void calc(float *data);

private:
	int _bits;
	int _inverse;
	int _signConvention;

	const float *_tSin;
	const float *_tCos;

	FFT *_fft;
};

} // End of namespace Common

#endif // COMMON_RDFT_H
