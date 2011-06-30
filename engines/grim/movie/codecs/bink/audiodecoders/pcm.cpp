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

/** @file sound/decoders/pcm.cpp
 *  Decoding PCM (Pulse Code Modulation).
 */

#include "common/stream.h"

#include "audio/audiostream.h"
#include "pcm.h"

namespace Audio {

// This used to be an inline template function, but
// buggy template function handling in MSVC6 forced
// us to go with the macro approach. So far this is
// the only template function that MSVC6 seemed to
// compile incorrectly. Knock on wood.
#define READ_ENDIAN_SAMPLE(is16Bit, isUnsigned, ptr, isLE) \
	((is16Bit ? (isLE ? _stream->readUint16LE() : _stream->readUint16BE()) : (_stream->readByte() << 8)) ^ (isUnsigned ? 0x8000 : 0))


/**
 * This is a stream, which allows for playing raw PCM data from a stream.
 * It also features playback of multiple blocks from a given stream.
 */
template<bool is16Bit, bool isUnsigned, bool isLE>
class PCMStream : public RewindableAudioStream {

protected:
	const int _rate;                               ///< Sample rate of stream
	const bool _isStereo;                          ///< Whether this is an stereo stream

	Common::SeekableReadStream *_stream;           ///< Stream to read data from
	const bool _disposeAfterUse;  ///< Indicates whether the stream object should be deleted when this RawStream is destructed

public:
	PCMStream(int rate, bool stereo, bool disposeStream, Common::SeekableReadStream *stream)
		: _rate(rate), _isStereo(stereo), _stream(stream), _disposeAfterUse(disposeStream) {

	}

	virtual ~PCMStream() {
		if (_disposeAfterUse)
			delete _stream;
	}

	int readBuffer(int16 *buffer, const int numSamples);
	bool isStereo() const           { return _isStereo; }
	bool endOfData() const          { return _stream->pos() >= _stream->size(); }
	int getRate() const         { return _rate; }
	bool rewind();
};

template<bool is16Bit, bool isUnsigned, bool isLE>
int PCMStream<is16Bit, isUnsigned, isLE>::readBuffer(int16 *buffer, const int numSamples) {
	int samples = numSamples;

	while (samples > 0 && !endOfData()) {
		*buffer++ = READ_ENDIAN_SAMPLE(is16Bit, isUnsigned, _ptr, isLE);
		samples--;
	}

	return numSamples - samples;
}

template<bool is16Bit, bool isUnsigned, bool isLE>
bool PCMStream<is16Bit, isUnsigned, isLE>::rewind() {
	// Easy peasy, lemon squeezee
	_stream->seek(0);
	return true;
}

/* In the following, we use preprocessor / macro tricks to simplify the code
 * which instantiates the input streams. We used to use template functions for
 * this, but MSVC6 / EVC 3-4 (used for WinCE builds) are extremely buggy when it
 * comes to this feature of C++... so as a compromise we use macros to cut down
 * on the (source) code duplication a bit.
 * So while normally macro tricks are said to make maintenance harder, in this
 * particular case it should actually help it :-)
 */

#define MAKE_RAW_STREAM(UNSIGNED) \
		if (is16Bit) { \
			if (isLE) \
				return new PCMStream<true, UNSIGNED, true>(rate, isStereo, disposeAfterUse, stream); \
			else  \
				return new PCMStream<true, UNSIGNED, false>(rate, isStereo, disposeAfterUse, stream); \
		} else \
			return new PCMStream<false, UNSIGNED, false>(rate, isStereo, disposeAfterUse, stream)


RewindableAudioStream *makePCMStream(Common::SeekableReadStream *stream,
                                   int rate, byte flags,
                                   bool disposeAfterUse) {


	const bool isStereo   = (flags & FLAG_STEREO) != 0;
	const bool is16Bit    = (flags & FLAG_16BITS) != 0;
	const bool isUnsigned = (flags & FLAG_UNSIGNED) != 0;
	const bool isLE       = (flags & FLAG_LITTLE_ENDIAN) != 0;

	if (isUnsigned) {
		MAKE_RAW_STREAM(true);
	} else {
		MAKE_RAW_STREAM(false);
	}
}

} // End of namespace Sound
