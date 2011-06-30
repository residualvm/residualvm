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

// Largely based on the PCM implementation found in ScummVM.

/** @file sound/decoders/pcm.h
 *  Decoding PCM (Pulse Code Modulation).
 */

#ifndef SOUND_DECODERS_RAW_H
#define SOUND_DECODERS_RAW_H

#include "audio/audiostream.h"

namespace Audio {

/**
 * Various flags which can be bit-ORed and then passed to
 * makeRawMemoryStream and some other AudioStream factories
 * to control their behavior.
 *
 * Engine authors are advised not to rely on a certain value or
 * order of these flags (in particular, do not store them verbatim
 * in savestates).
 */
enum PCMFlags {
	/** unsigned samples (default: signed) */
	FLAG_UNSIGNED = 1 << 0,

	/** sound is 16 bits wide (default: 8bit) */
	FLAG_16BITS = 1 << 1,

	/** samples are little endian (default: big endian) */
	FLAG_LITTLE_ENDIAN = 1 << 2,

	/** sound is in stereo (default: mono) */
	FLAG_STEREO = 1 << 3
};

/**
 * Creates an audio stream, which plays from the given stream.
 *
 * @param stream Stream object to play from.
 * @param rate   Rate of the sound data.
 * @param flags  Audio flags combination.
 * @see RawFlags
 * @param disposeAfterUse Whether to delete the stream after use.
 * @return The new SeekableAudioStream (or 0 on failure).
 */
RewindableAudioStream *makePCMStream(Common::SeekableReadStream *stream,
                                   int rate, byte flags,
                                   bool disposeAfterUse = true);

} // End of namespace Sound

#endif
