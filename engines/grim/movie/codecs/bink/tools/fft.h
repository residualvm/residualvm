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

// Based upon the (I)FFT code in FFmpeg
// Copyright (c) 2008 Loren Merritt
// Copyright (c) 2002 Fabrice Bellard
// Partly based on libdjbfft by D. J. Bernstein

/** @file common/fft.h
 *  (Inverse) Fast Fourier Transform.
 */

#ifndef COMMON_FFT_H
#define COMMON_FFT_H

#include "common/types.h"
#include "maths.h"

namespace Common {

/** (Inverse) Fast Fourier Transform. */
class FFT {
public:
	FFT(int bits, int inverse);
	~FFT();

	/** Do the permutation needed BEFORE calling calc(). */
	void permute(Complex *z);

	/** Do a complex FFT.
	 *
	 *  The input data must be permuted before.
	 *  No 1.0/sqrt(n) normalization is done.
	 */
	void calc(Complex *z);

private:
	int _bits;
	int _inverse;

	uint16 *_revTab;

	Complex *_expTab;
	Complex *_tmpBuf;

	const float *_tSin;
	const float *_tCos;

	int _splitRadix;
	int _permutation;

	static int splitRadixPermutation(int i, int n, int inverse);
};

} // End of namespace Common

#endif // COMMON_FFT_H
