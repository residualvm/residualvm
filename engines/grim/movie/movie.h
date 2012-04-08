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

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#ifndef GRIM_MOVIE_PLAYER_H
#define GRIM_MOVIE_PLAYER_H

#include "common/mutex.h"
#include "common/system.h"

#include "video/video_decoder.h"

namespace Grim {

class SaveGame;

class MoviePlayer {
protected:
	Common::String _fname;
	Common::Mutex _frameMutex;
	Video::VideoDecoder *_videoDecoder;		//< Initialize this to your needed subclass of VideoDecoder in the constructor
	const Graphics::Surface *_internalSurface;
	Graphics::Surface *_externalSurface;
	int32 _frame;
	bool _updateNeeded;
	float _movieTime;
	int _channels;
	int _freq;
	bool _videoFinished;
	bool _videoPause;
	bool _videoLooping;
	bool _timerStarted;
	int _x, _y;

public:
	MoviePlayer();
	virtual ~MoviePlayer();

	/**
	 * Loads a file for playing, and starts playing it.
	 * the default implementation calls init()/deinit() to handle
	 * any necessary setup.
	 *
	 * @param filename		the file to open
	 * @param looping		true if we want the video to loop, false otherwise
	 * @param x				the x-coordinate for the draw-position
	 * @param y				the y-coordinate for the draw-position
	 * @see	init
	 * @see stop
	 */
	virtual bool play(Common::String filename, bool looping, int x, int y);
	virtual void stop();
	virtual void pause(bool p);
	virtual bool isPlaying() { return !_videoFinished; }
	virtual bool isUpdateNeeded() { return _updateNeeded; }
	virtual Graphics::Surface *getDstSurface();
	virtual int getX() { return _x; }
	virtual int getY() { return _y; }
	virtual int getFrame() { return _frame; }
	virtual void clearUpdateNeeded() { _updateNeeded = false; }
	virtual int32 getMovieTime() { return (int32)_movieTime; }

	/**
	 * Saves the state of the video to a savegame
	 *
	 * If you overload this in a subclass, call this first thing in the
	 * overloaded function
	 *
	 * @param state			the state to save to
	 */
	virtual void saveState(SaveGame *state);
	virtual void restoreState(SaveGame *state);

protected:
	static void timerCallback(void *ptr);
	/**
	 * Handles basic stuff per frame, like copying the latest frame to
	 * _externalBuffer, and updating the frame-counters.
	 *
	 * @return false if a frame wasnt drawn to _externalBuffer, true otherwise.
	 * @see handleFrame
	 */
	virtual bool prepareFrame();

	/**
	 * Frame-handling function.
	 *
	 * Perform any codec-specific per-frame operations before the decoder
	 * decodes the next frame.
	 *
	 * @see prepareFrame
	 * @see clearUpdateNeeded
	 * @see isUpdateNeeded
	 */
	virtual void handleFrame() {};

	/**
	 * Frame-handling function.
	 *
	 * Perform any codec-specific per-frame operations after prepareFrame has been
	 * run, this function is called whenever prepareFrame returns true.
	 *
	 * @see prepareFrame
	 * @see clearUpdateNeeded
	 * @see isUpdateNeeded
	 */
	virtual void postHandleFrame() {};

	/**
	 * Initialization of buffers
	 * This function is called by the default-implementation of play,
	 * and is expected to get the necessary datastructures set up for
	 * playback, as well as initializing the callback.
	 *
	 * @see deinit
	 */
	virtual void init();

	/**
	 * Closes any file/codec-handles, and resets the movie-state to
	 * a blank MoviePlayer.
	 *
	 * @see init
	 */
	virtual void deinit();

	/**
	 * Loads a file for playback, any additional setup is not done here, but in
	 * the play-function. This function is supposed to handle any specifics w.r.t.
	 * files vs containers (i.e. load from LAB vs load from file).
	 *
	 * @see play
	 * @param filename		The filename to be handled.
	 */
	virtual bool loadFile(Common::String filename);
};


// Factory-like functions:

MoviePlayer *CreateMpegPlayer();
MoviePlayer *CreateSmushPlayer(bool demo);
MoviePlayer *CreateBinkPlayer(bool demo);
extern MoviePlayer *g_movie;

} // end of namespace Grim

#endif
