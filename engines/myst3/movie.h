/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef MOVIE_H_
#define MOVIE_H_

#include "engines/myst3/gfx.h"
#include "engines/myst3/node.h"

#include "math/vector3d.h"
#include "video/bink_decoder.h"

namespace Myst3 {

struct VideoData;
class Myst3Engine;
class Texture;
class Subtitles;

class Movie : public Drawable {
public:
	Movie(Myst3Engine *vm, uint16 id);
	virtual ~Movie();

	virtual void draw();
	virtual void drawOverlay();

	uint16 getId() { return _id; }
	void setPosU(int32 v) { _posU = v; }
	void setPosV(int32 v) { _posV = v; }
	void setForce2d(bool b) { _force2d = b; }
	void setForceOpaque(bool b) { _forceOpaque = b; }
	void setStartFrame(int32 v) { _startFrame = v; }
	void setEndFrame(int32 v) { _endFrame = v; }

protected:
	Myst3Engine *_vm;

	uint16 _id;
	Subtitles *_subtitles;

	Math::Vector3d _pTopLeft;
	Math::Vector3d _pBottomLeft;
	Math::Vector3d _pBottomRight;
	Math::Vector3d _pTopRight;

	bool _force2d;
	bool _forceOpaque;
	int32 _posU;
	int32 _posV;

	Video::BinkDecoder _bink;
	Texture *_texture;

	int32 _startFrame;
	int32 _endFrame;

	void loadPosition(const VideoData &videoData);
	void drawNextFrameToTexture();

	void draw2d();
	void draw3d();
};

class ScriptedMovie : public Movie {
public:
	ScriptedMovie(Myst3Engine *vm, uint16 id);
	virtual ~ScriptedMovie();

	void draw();
	void drawOverlay();
	virtual void update();

	void setEndFrameVar(uint16 v) { _endFrameVar = v; }
	void setNextFrameReadVar(uint16 v) { _nextFrameReadVar = v; }
	void setNextFrameWriteVar(uint16 v) { _nextFrameWriteVar = v; }
	void setPlayingVar(uint16 v) { _playingVar = v; }
	void setPosUVar(uint16 v) { _posUVar = v; }
	void setPosVVar(uint16 v) { _posVVar = v; }
	void setStartFrameVar(uint16 v) { _startFrameVar = v; }
	void setCondition(int16 condition) { _condition = condition; }
	void setConditionBit(int16 cb) { _conditionBit = cb; }
	void setDisableWhenComplete(bool upd) { _disableWhenComplete = upd; }
	void setLoop(bool loop) { _loop = loop; }
	void setScriptDriven(bool b) { _scriptDriven = b; }

protected:
	bool _enabled;
	bool _loop;
	bool _disableWhenComplete;
	bool _scriptDriven;
	bool _isLastFrame;

	int16 _condition;
	uint16 _conditionBit;

	uint16 _startFrameVar;
	uint16 _endFrameVar;
	uint16 _posUVar;
	uint16 _posVVar;

	uint16 _nextFrameReadVar;
	uint16 _nextFrameWriteVar;

	uint16 _playingVar;
};

class SimpleMovie : public Movie {
public:
	SimpleMovie(Myst3Engine *vm, uint16 id);
	virtual ~SimpleMovie();

	bool update();

	void setSynchronized(bool b) { _synchronized = b; }
private:
	bool _synchronized;
	uint _startEngineFrame;
};

// Used by the projectors on J'nanin, see puzzle #14
class ProjectorMovie : public ScriptedMovie {
public:
	ProjectorMovie(Myst3Engine *vm, uint16 id, Graphics::Surface *background);
	virtual ~ProjectorMovie();

	void update();

private:
	Graphics::Surface *_background;
	Graphics::Surface *_frame;

	static const uint kBlurIterations = 30;
	uint8 _blurTableX[kBlurIterations];
	uint8 _blurTableY[kBlurIterations];
};

} /* namespace Myst3 */
#endif /* MOVIE_H_ */
