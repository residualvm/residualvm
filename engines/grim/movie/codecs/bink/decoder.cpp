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

/** @file graphics/video/decoder.cpp
 *  Generic video decoder interface.
 */

#include "common/system.h"
#include "common/textconsole.h"
#include "common/stream.h"
#include "common/memstream.h"

#include "decoder.h"

#include "audio/audiostream.h"
#include "audiodecoders/pcm.h"

namespace GrimGraphics {

VideoDecoder::VideoDecoder() : _started(false), _finished(false), _needCopy(false),
	_width(0), _height(0), _pitch(0), _data(0),	_soundRate(0), _soundFlags(0), _sound(0) {
}

VideoDecoder::~VideoDecoder() {
	delete[] _data;

	deinitSound();
}

void VideoDecoder::createData(uint32 width, uint32 height) {
	if (_data)
		error("BINK: VideoDecoder::createData() called twice?!?");

	_width  = width;
	_height = height;

	// The pitch of the data memory
	_pitch = _width;

	// Create and initialize the image data memory
	_data = new byte[_width * _height * 4];
	memset(_data, 0, _width * _height * 4);
}

void VideoDecoder::initSound(uint16 rate, bool stereo, bool is16) {
	deinitSound();

	_soundRate  = rate;
	_soundFlags = Audio::FLAG_LITTLE_ENDIAN;

	if (stereo)
		_soundFlags |= Audio::FLAG_STEREO;
	if (is16)
		_soundFlags |= Audio::FLAG_16BITS;

	_sound = Audio::makeQueuingAudioStream(_soundRate, stereo);
	g_system->getMixer()->playStream(Audio::Mixer::kMusicSoundType, &_soundHandle, _sound);
}
	
void VideoDecoder::queueSound(const byte *data, uint32 dataSize){
	// I think these are cleaned up correctly, but I'm not sure.
	Common::MemoryReadStream *dataStream = new Common::MemoryReadStream((const byte*)data, dataSize, DisposeAfterUse::YES);
	Audio::RewindableAudioStream *dataPCM = Audio::makePCMStream(dataStream, _soundRate, _soundFlags);
	
	queueSound(dataPCM);
}
	
void VideoDecoder::queueSound(Audio::AudioStream* stream){
	if (g_system->getMixer()->isReady()) {
		_sound->queueAudioStream(stream);
	} 
}

void VideoDecoder::deinitSound() {
	delete _sound;
}

uint32 VideoDecoder::getNumQueuedStreams() const {
	return _sound ? _sound->numQueuedStreams() : 0;
}

bool VideoDecoder::isPlaying() const {
	return !_finished;
}

void VideoDecoder::update() {
	if (!isPlaying())
		return;
	
	// Wait until we can update

	if (!isPlaying())
		return;
	
	if(_needCopy)
		return;

	processData();
}

void VideoDecoder::renderToArray(byte* data, int len){
	if (!isPlaying())
		return;
	
	if (!_started)
		return;

	int32 copyLen = _width * _height * 4;
	if(len < copyLen)
		warning("BINK: Too small an external buffer allocated!");
	if(_needCopy && !_finished){
		memcpy(data, _data, len);
		_needCopy = false;
	}

}

void VideoDecoder::abort() {
	deinitSound();

	_finished = true;
}

} // End of namespace Graphics
