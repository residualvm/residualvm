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

/** @file graphics/video/decoder.h
 *  Generic video decoder interface.
 */

#ifndef GRAPHICS_VIDEO_DECODER_H
#define GRAPHICS_VIDEO_DECODER_H

#include "audio/audiostream.h"
#include "audio/mixer.h"
#include "common/types.h"

namespace GrimGraphics {

/** A generic interface for video decoders. */
class VideoDecoder {
public:
	VideoDecoder();
	~VideoDecoder();

	/** Is the video currently playing? */
	bool isPlaying() const;

	/** Update the video. */
	void update();

	/** Render the video to an Array for showing through other means. */
	void renderToArray(byte* data, int len);

	/** Abort the playing of the video. */
	void abort();

	/** Is there enough time to spare to sleep for 10ms? */
	virtual bool hasTime() const = 0;

protected:
	volatile bool _started;  ///< Has playback started?
	volatile bool _finished; ///< Has playback finished?
	volatile bool _needCopy; ///< Is new frame content available that needs to by copied?

	uint32 _width;  ///< The video's width.
	uint32 _height; ///< The video's height.
	uint32 _pitch;  ///< The pitch of the video surface, in pixels.

	byte *_data; ///< The video surface's data.

	/** Create a data area for a video of these dimensions.
	 *
	 *  Since the data will be copied into the graphics card memory, the surface
	 *  memory always has power-of-two dimensions. The actual surface width, in
	 *  pixels, will be stored into _pitch.
	 *
	 *  The surface's pixel format is always BGRA8888.
	 */
	void createData(uint32 width, uint32 height);

	void initSound(uint16 rate, bool stereo, bool is16);
	void deinitSound();
	
	void queueSound(const byte *data, uint32 dataSize);
	void queueSound(Audio::AudioStream *stream);

	uint32 getNumQueuedStreams() const;

	/** Process the video's image and sound data further. */
	virtual void processData() = 0;

	// GLContainer
	void doRebuild();
	void doDestroy();

	uint16                     _soundRate;
	byte                       _soundFlags;
	
private:
	Audio::QueuingAudioStream* _sound;		///< AudioStream for playback.
	Audio::SoundHandle _soundHandle;		///< SoundHandle for controlling playback.

	/** Copy the video image data to the texture. */
	void copyData();
};

} // End of namespace Graphics

#endif // GRAPHICS_VIDEO_DECODER_H
