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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if defined(WIN32) && !defined(__SYMBIAN32__)
#include <windows.h>
// winnt.h defines ARRAYSIZE, but we want our own one...
#undef ARRAYSIZE
#endif

#include "common/endian.h"
#include "common/file.h"
#include "common/str.h"
#include "common/system.h"
#include "common/textconsole.h"

#ifdef USE_OPENGL_SHADERS

#include "graphics/surface.h"
#include "graphics/pixelbuffer.h"

#include "engines/grim/actor.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/colormap.h"
#include "engines/grim/emi/modelemi.h"
#include "engines/grim/font.h"
#include "engines/grim/gfx_opengl_shaders.h"
#include "engines/grim/grim.h"
#include "engines/grim/material.h"
#include "engines/grim/model.h"
#include "engines/grim/primitives.h"
#include "engines/grim/set.h"
#include "engines/grim/sprite.h"

namespace Grim {

template<class T>
static T nextHigher2(T k) {
	if (k == 0)
		return 1;
	--k;

	for (uint i = 1; i < sizeof(T) * 8; i <<= 1)
		k = k | k >> i;

	return k + 1;
}

static float textured_quad[] = {
//	X   , Y   , S   , T
	0.0f, 0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 0.0f, 1.0f,
};

static float textured_quad_centered[] = {
//	 X   ,  Y   , Z   , S   , T
	-0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
	-0.5f, +0.5f, 0.0f, 0.0f, 0.0f,
	+0.5f, +0.5f, 0.0f, 1.0f, 0.0f,
	+0.5f, -0.5f, 0.0f, 1.0f, 1.0f,
};

static float zero_texVerts[] = { 0.0, 0.0 };

struct GrimVertex {
	GrimVertex(const float *verts, const float *texVerts, const float *normals) {
		memcpy(_position, verts, 3 * sizeof(float));
		memcpy(_texcoord, texVerts, 2 * sizeof(float));
		memcpy(_normal, normals, 3 * sizeof(float));
	}
	float _position[3];
	float _texcoord[2];
	float _normal[3];
};

struct TextUserData {
	Graphics::Shader * shader;
	uint32 characters;
	Color  color;
	GLuint texture;
};

struct FontUserData {
	int size;
	GLuint texture;
};

struct EMIModelUserData {
	Graphics::Shader *_shader;
	uint32 _texCoordsVBO;
	uint32 _colorMapVBO;
	uint32 _verticesVBO;
};

struct ModelUserData {
	Graphics::Shader *_shader;
	uint32 _meshInfoVBO;
};

struct ShadowUserData {
	uint32 _verticesVBO;
	uint32 _indicesVBO;
	uint32 _numTriangles;
};


// taken from glm
Math::Matrix4 makeLookMatrix(const Math::Vector3d& pos, const Math::Vector3d& interest, const Math::Vector3d& up) {
	Math::Vector3d f = (interest - pos).getNormalized();
	Math::Vector3d u = up.getNormalized();
	Math::Vector3d s = Math::Vector3d::crossProduct(f, u).getNormalized();
	u = Math::Vector3d::crossProduct(s, f);

	Math::Matrix4 look;
	look(0,0) = s.x();
	look(1,0) = s.y();
	look(2,0) = s.z();
	look(0,1) = u.x();
	look(1,1) = u.y();
	look(2,1) = u.z();
	look(0,2) = -f.x();
	look(1,2) = -f.y();
	look(2,2) = -f.z();
	look(3,0) = -Math::Vector3d::dotProduct(s, pos);
	look(3,1) = -Math::Vector3d::dotProduct(u, pos);
	look(3,2) =  Math::Vector3d::dotProduct(f, pos);

	look.transpose();

	return look;
}

// taken from glm
Math::Matrix4 makeProjectionMatrix(float fov, float nclip, float fclip) {
	float right = nclip * tan(fov / 2 * (LOCAL_PI / 180));
	float left = -right;
	float top = right * 0.75;
	float bottom = -right * 0.75;

	Math::Matrix4 proj;
	proj(0,0) = (2.0f * nclip) / (right - left);
	proj(1,1) = (2.0f * nclip) / (top - bottom);
	proj(2,0) = (right + left) / (right - left);
	proj(2,1) = (top + bottom) / (top - bottom);
	proj(2,2) = -(fclip + nclip) / (fclip - nclip);
	proj(2,3) = -1.0f;
	proj(3,2) = -(2.0f * fclip * nclip) / (fclip - nclip);
	proj(3,3) = 0.0f;

	return proj;
}

// taken from glm
Math::Matrix4 makeRotationMatrix(const Math::Angle& angle, Math::Vector3d axis) {
	float c = angle.getCosine();
	float s = angle.getSine();
	axis.normalize();
	Math::Vector3d temp = (1.f - c) * axis;
	Math::Matrix4 rotate;
	rotate(0, 0) = c + temp.x() * axis.x();
	rotate(0, 1) = 0 + temp.x() * axis.y() + s * axis.z();
	rotate(0, 2) = 0 + temp.x() * axis.z() - s * axis.y();
	rotate(0, 3) = 0;
	rotate(1, 0) = 0 + temp.y() * axis.x() - s * axis.z();
	rotate(1, 1) = c + temp.y() * axis.y();
	rotate(1, 2) = 0 + temp.y() * axis.z() + s * axis.x();
	rotate(1, 3) = 0;
	rotate(2, 0) = 0 + temp.z() * axis.x() + s * axis.y();
	rotate(2, 1) = 0 + temp.z() * axis.y() - s * axis.x();
	rotate(2, 2) = c + temp.z() * axis.z();
	rotate(2, 3) = 0;
	rotate(3, 0) = 0;
	rotate(3, 1) = 0;
	rotate(3, 2) = 0;
	rotate(3, 3) = 1;

	return rotate;
}

GfxBase *CreateGfxOpenGL() {
	return new GfxOpenGLS();
}

GfxOpenGLS::GfxOpenGLS() {
	_smushTexId = 0;
	_matrixStack.push(Math::Matrix4());
	_fov = -1.0;
	_nclip = -1;
	_fclip = -1;
	_selectedTexture = NULL;
	_emergTexture = 0;
	_maxLights = 8;
	_lights = new Light[_maxLights];
	_lightsEnabled = false;
}

GfxOpenGLS::~GfxOpenGLS() {
	delete[] _lights;
}

void GfxOpenGLS::setupZBuffer() {
	GLint format = GL_LUMINANCE_ALPHA;
	GLenum type = GL_UNSIGNED_BYTE;
	float width = _gameWidth;
	float height = _gameHeight;

	glGenTextures(1, (GLuint *)&_zBufTex);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _zBufTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, format, nextHigher2((int)width), nextHigher2((int)height), 0, format, type, NULL);
	glActiveTexture(GL_TEXTURE0);

	_zBufTexCrop = Math::Vector2d(width / nextHigher2((int)width), height / nextHigher2((int)height));
}

void GfxOpenGLS::setupQuadEBO() {
	// FIXME: Probably way too big...
	unsigned short quad_indices[6 * 1000];

	unsigned short start = 0;
	for (unsigned short *p = quad_indices; p < &quad_indices[6 * 1000]; p += 6) {
		p[0] = p[3] = start++;
		p[1] = start++;
		p[2] = p[4] = start++;
		p[5] = start++;
	}

	_quadEBO = Graphics::Shader::createBuffer(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_indices), quad_indices, GL_STATIC_DRAW);
}

void GfxOpenGLS::setupTexturedQuad() {
	_smushVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, sizeof(textured_quad), textured_quad, GL_STATIC_DRAW);
	_smushProgram->enableVertexAttribute("position", _smushVBO, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	_smushProgram->enableVertexAttribute("texcoord", _smushVBO, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));

	_emergProgram->enableVertexAttribute("position", _smushVBO, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	_emergProgram->enableVertexAttribute("texcoord", _smushVBO, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));

	if (g_grim->getGameType() == GType_GRIM) {
		_backgroundProgram->enableVertexAttribute("position", _smushVBO, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		_backgroundProgram->enableVertexAttribute("texcoord", _smushVBO, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));
	}
}

void GfxOpenGLS::setupTexturedCenteredQuad() {
	_spriteVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, sizeof(textured_quad_centered), textured_quad_centered, GL_STATIC_DRAW);
	_spriteProgram->enableVertexAttribute("position", _spriteVBO, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	_spriteProgram->enableVertexAttribute("texcoord", _spriteVBO, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 3 * sizeof(float));
	_spriteProgram->disableVertexAttribute("color", Math::Vector4d(1.0f, 1.0f, 1.0f, 1.0f));
}

void GfxOpenGLS::setupPrimitives() {
	uint32 numVBOs = ARRAYSIZE(_primitiveVBOs);
	glGenBuffers(numVBOs, _primitiveVBOs);
	_currentPrimitive = 0;
	for (uint32 i = 0; i < numVBOs; ++i) {
		glBindBuffer(GL_ARRAY_BUFFER, _primitiveVBOs[i]);
		glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	}

	glGenBuffers(1, &_irisVBO);
	glBindBuffer(GL_ARRAY_BUFFER, _irisVBO);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	_irisProgram->enableVertexAttribute("position", _irisVBO, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

GLuint GfxOpenGLS::nextPrimitive() {
	GLuint ret = _primitiveVBOs[_currentPrimitive];
	_currentPrimitive = (_currentPrimitive + 1) % ARRAYSIZE(_primitiveVBOs);
	return ret;
}

void GfxOpenGLS::setupShaders() {
	bool isEMI = g_grim->getGameType() == GType_MONKEY4;

	static const char* commonAttributes[] = {"position", "texcoord", NULL};
	_backgroundProgram = Graphics::Shader::fromFiles(isEMI ? "emi_background" : "grim_background", commonAttributes);
	_smushProgram = Graphics::Shader::fromFiles("smush", commonAttributes);
	_textProgram = Graphics::Shader::fromFiles("text", commonAttributes);
	_emergProgram = Graphics::Shader::fromFiles("emerg", commonAttributes);

	static const char* actorAttributes[] = {"position", "texcoord", "color", "normal", NULL};
	_actorProgram = Graphics::Shader::fromFiles(isEMI ? "emi_actor" : "grim_actor", actorAttributes);
	_spriteProgram = _actorProgram->clone();

	if (!isEMI) {
		static const char* primAttributes[] = {"position", NULL};
		_primitiveProgram = Graphics::Shader::fromFiles("grim_primitive", primAttributes);
		_irisProgram = _primitiveProgram->clone();

		_shadowPlaneProgram = Graphics::Shader::fromFiles("grim_shadowplane", primAttributes);
	}

	setupQuadEBO();
	setupTexturedQuad();
	setupTexturedCenteredQuad();

	if (!isEMI) {
		setupPrimitives();

		_blastVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, 128 * 16 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	}
}

byte *GfxOpenGLS::setupScreen(int screenW, int screenH, bool fullscreen) {
	_pixelFormat = g_system->setupScreen(screenW, screenH, fullscreen, true).getFormat();
#ifndef USE_GLES2
	GLenum err = glewInit();
	if (err != GLEW_OK) {
		error("Error: %s\n", glewGetErrorString(err));
	}
	assert(GLEW_OK == err);
#endif

	_screenWidth = screenW;
	_screenHeight = screenH;
	_scaleW = _screenWidth / (float)_gameWidth;
	_scaleH = _screenHeight / (float)_gameHeight;

	_isFullscreen = g_system->getFeatureState(OSystem::kFeatureFullscreenMode);
#ifdef USE_GLES2
	g_system->setFeatureState(OSystem::kFeatureVirtControls, true);
#endif

	g_system->showMouse(false);

	char GLDriver[1024];
	sprintf(GLDriver, "ResidualVM: %s/%s with shaders", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	g_system->setWindowCaption(GLDriver);

	setupZBuffer();
	setupShaders();

	// Load emergency built-in font
	loadEmergFont();

	_screenSize = _screenWidth * _screenHeight * 4;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (g_grim->getGameType() == GType_MONKEY4) {
		// GL_LEQUAL as glDepthFunc ensures that subsequent drawing attempts for
		// the same triangles are not ignored by the depth test.
		// That's necessary for EMI where some models have multiple faces which
		// refer to the same vertices. The first face is usually using the
		// color map and the following are using textures.
		glDepthFunc(GL_LEQUAL);
	}
	return NULL;
}

// matrix calculation based on the glm library.
void GfxOpenGLS::setupCamera(float fov, float nclip, float fclip, float roll) {
	if (_fov == fov && _nclip == nclip && _fclip == fclip)
		return;

	_fov = fov; _nclip = nclip; _fclip = fclip;

	_projMatrix = makeProjectionMatrix(fov, nclip, fclip);
}

void GfxOpenGLS::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest, float roll) {
	Math::Matrix4 projMatrix = _projMatrix;
	projMatrix.transpose();

	if (g_grim->getGameType() == GType_MONKEY4) {
		_currentPos = pos;
		_currentQuat = Math::Quaternion(interest.x(), interest.y(), interest.z(), roll);

		Math::Matrix4 invertZ;
		invertZ(2,2) = -1.0f;

		Math::Matrix4 viewMatrix = _currentQuat.toMatrix();
		viewMatrix.transpose();

		Math::Matrix4 camPos;
		camPos(0,3) = -_currentPos.x();
		camPos(1,3) = -_currentPos.y();
		camPos(2,3) = -_currentPos.z();

		_mvpMatrix = projMatrix * invertZ * viewMatrix * camPos;
	} else {
		Math::Matrix4 viewMatrix = makeRotationMatrix(Math::Angle(roll), Math::Vector3d(0, 0, 1));
		Math::Vector3d up_vec(0, 0, 1);

		if (pos.x() == interest.x() && pos.y() == interest.y())
			up_vec = Math::Vector3d(0, 1, 0);

		Math::Matrix4 lookMatrix = makeLookMatrix(pos, interest, up_vec);

		_viewMatrix = viewMatrix * lookMatrix;
		_viewMatrix.transpose();
	}
}


void GfxOpenGLS::clearScreen() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GfxOpenGLS::flipBuffer() {
	g_system->updateScreen();
}


void GfxOpenGLS::getBoundingBoxPos(const Mesh *mesh, int *x1, int *y1, int *x2, int *y2) {

}

void GfxOpenGLS::getBoundingBoxPos(const EMIModel *model, int *x1, int *y1, int *x2, int *y2) {
	if (_currentShadowArray) {
		*x1 = -1;
		*y1 = -1;
		*x2 = -1;
		*y2 = -1;
		return;
	}

	Math::Matrix4 modelMatrix = _currentActor->getFinalMatrix();
	Math::Matrix4 mvpMatrix = _mvpMatrix * modelMatrix;

	double top = 1000;
	double right = -1000;
	double left = 1000;
	double bottom = -1000;

	for (uint i = 0; i < model->_numFaces; i++) {
		int *indices = (int *)model->_faces[i]._indexes;

		for (uint j = 0; j < model->_faces[i]._faceLength * 3; j++) {
			int index = indices[j];
			const Math::Vector3d &dv = model->_drawVertices[index];

			Math::Vector4d v = Math::Vector4d(dv.x(), dv.y(), dv.z(), 1.0f);
			v = mvpMatrix * v;
			v /= v.w();

			double winX = (1 + v.x()) / 2.0f * _gameWidth;
			double winY = (1 + v.y()) / 2.0f * _gameHeight;

			if (winX > right)
				right = winX;
			if (winX < left)
				left = winX;
			if (winY < top)
				top = winY;
			if (winY > bottom)
				bottom = winY;
		}
	}

	double t = bottom;
	bottom = _gameHeight - top;
	top = _gameHeight - t;

	if (left < 0)
		left = 0;
	if (right >= _gameWidth)
		right = _gameWidth - 1;
	if (top < 0)
		top = 0;
	if (bottom >= _gameHeight)
		bottom = _gameHeight - 1;

	if (top >= _gameHeight || left >= _gameWidth || bottom < 0 || right < 0) {
		*x1 = -1;
		*y1 = -1;
		*x2 = -1;
		*y2 = -1;
		return;
	}

	*x1 = (int)left;
	*y1 = (int)(_gameHeight - bottom);
	*x2 = (int)right;
	*y2 = (int)(_gameHeight - top);
}

void GfxOpenGLS::startActorDraw(const Actor *actor) {
	_currentActor = actor;
	_actorProgram->use();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);

	const Math::Vector3d &pos = actor->getWorldPos();
	const Math::Quaternion &quat = actor->getRotationQuat();
	//const float scale = actor->getScale();
	const float alpha = actor->getEffectiveAlpha();

	if (g_grim->getGameType() == GType_MONKEY4) {
		const Math::Matrix4 &viewMatrix = _currentQuat.toMatrix();
		Math::Matrix4 modelMatrix = actor->getFinalMatrix();
		modelMatrix.transpose();

		_actorProgram->setUniform("modelMatrix", modelMatrix);
		if (actor->isInOverworld()) {
			_actorProgram->setUniform("viewMatrix", quat.toMatrix());
			_actorProgram->setUniform("projMatrix", makeProjectionMatrix(90, 1, 3276.8f));
		} else {
			_actorProgram->setUniform("viewMatrix", viewMatrix);
			_actorProgram->setUniform("projMatrix", _projMatrix);
		}

		_actorProgram->setUniform("cameraPos", _currentPos);
		_actorProgram->setUniform("actorPos", pos);
		_actorProgram->setUniform("isBillboard", GL_FALSE);
		_actorProgram->setUniform1f("alpha", alpha);
		_actorProgram->setUniform("inOverworld", actor->isInOverworld());
	} else {
		Math::Matrix4 modelMatrix = quat.toMatrix();
		bool hasZBuffer = g_grim->getCurrSet()->getCurrSetup()->_bkgndZBm;
		Math::Matrix4 extraMatrix;

		modelMatrix.transpose();
		modelMatrix.setPosition(pos);
		modelMatrix.transpose();
		_mvpMatrix = _viewMatrix * modelMatrix;
		_mvpMatrix.transpose();

		_actorProgram->setUniform("modelMatrix", modelMatrix);
		_actorProgram->setUniform("viewMatrix", _viewMatrix);
		_actorProgram->setUniform("projMatrix", _projMatrix);
		_actorProgram->setUniform("extraMatrix", extraMatrix);
		_actorProgram->setUniform("mvpMatrix", _mvpMatrix);
		_actorProgram->setUniform("tex", 0);
		_actorProgram->setUniform("texZBuf", 1);
		_actorProgram->setUniform("hasZBuffer", hasZBuffer);
		_actorProgram->setUniform("texcropZBuf", _zBufTexCrop);
		_actorProgram->setUniform("screenSize", Math::Vector2d(_screenWidth, _screenHeight));

		if (_currentShadowArray) {
			const Sector *shadowSector = _currentShadowArray->planeList.front().sector;
			const Math::Vector3d color = Math::Vector3d(_shadowColorR, _shadowColorG, _shadowColorB) / 255.f;
			Math::Vector3d normal = shadowSector->getNormal();
			if (!_currentShadowArray->dontNegate)
				normal = -normal;

			_actorProgram->setUniform("shadow._active", true);
			_actorProgram->setUniform("shadow._color", color);
			_actorProgram->setUniform("shadow._light", _currentShadowArray->pos);
			_actorProgram->setUniform("shadow._point", shadowSector->getVertices()[0]);
			_actorProgram->setUniform("shadow._normal", normal);

			glDisable(GL_DEPTH_TEST);
			glDisable(GL_BLEND);
			glEnable(GL_POLYGON_OFFSET_FILL);
		} else {
			_actorProgram->setUniform("shadow._active", false);
		}

		_actorProgram->setUniform("lightsEnabled", _lightsEnabled);
		if (_lightsEnabled) {
			for (int i = 0; i < _maxLights; ++i) {
				const Light &l = _lights[i];
				Common::String uniform;
				uniform = Common::String::format("lights[%u]._position", i);
				_actorProgram->setUniform(uniform.c_str(), _viewMatrix * l._position);

				Math::Vector4d direction = l._direction;
				direction.w() = 0.0;
				_viewMatrix.transformVector(&direction);
				direction.w() = l._direction.w();

				uniform = Common::String::format("lights[%u]._direction", i);
				_actorProgram->setUniform(uniform.c_str(), direction);

				uniform = Common::String::format("lights[%u]._color", i);
				_actorProgram->setUniform(uniform.c_str(), l._color);
			}
		}
	}
}


void GfxOpenGLS::finishActorDraw() {
	_currentActor = NULL;
	glDisable(GL_POLYGON_OFFSET_FILL);
}

void GfxOpenGLS::setShadow(Shadow *shadow) {
	_currentShadowArray = shadow;
}

void GfxOpenGLS::drawShadowPlanes() {
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	glClearStencil(~0);
	glClear(GL_STENCIL_BUFFER_BIT);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 1, (GLuint)~0);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glDisable(GL_TEXTURE_2D);

	if (!_currentShadowArray->userData) {
		uint32 numVertices = 0;
		uint32 numTriangles = 0;
		for (SectorListType::iterator i = _currentShadowArray->planeList.begin(); i != _currentShadowArray->planeList.end(); ++i) {
			numVertices += i->sector->getNumVertices();
			numTriangles += i->sector->getNumVertices() - 2;
		}

		float *vertBuf = new float[3 * numVertices];
		uint16 *idxBuf = new uint16[3 * numTriangles];

		float *vert = vertBuf;
		uint16 *idx = idxBuf;

		for (SectorListType::iterator i = _currentShadowArray->planeList.begin(); i != _currentShadowArray->planeList.end(); ++i) {
			Sector *shadowSector = i->sector;
			memcpy(vert, shadowSector->getVertices(), 3 * shadowSector->getNumVertices() * sizeof(float));
			uint16 first = (vert - vertBuf) / 3;
			for (uint16 j = 2; j < shadowSector->getNumVertices(); ++j) {
				*idx++ = first;
				*idx++ = first + j - 1;
				*idx++ = first + j;
			}
			vert += 3 * shadowSector->getNumVertices();
		}

		ShadowUserData *sud = new ShadowUserData;
		_currentShadowArray->userData = sud;
		sud->_numTriangles = numTriangles;
		sud->_verticesVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, 3 * numVertices * sizeof(float), vertBuf, GL_STATIC_DRAW);
		sud->_indicesVBO = Graphics::Shader::createBuffer(GL_ELEMENT_ARRAY_BUFFER, 3 * numTriangles * sizeof(uint16), idxBuf, GL_STATIC_DRAW);

		delete[] vertBuf;
		delete[] idxBuf;
	}

	const ShadowUserData *sud = (ShadowUserData *)_currentShadowArray->userData;
	_shadowPlaneProgram->use();
	_shadowPlaneProgram->setUniform("projMatrix", _projMatrix);
	_shadowPlaneProgram->setUniform("viewMatrix", _viewMatrix);

	glBindBuffer(GL_ARRAY_BUFFER, sud->_verticesVBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sud->_indicesVBO);
	const uint32 attribPos = _shadowPlaneProgram->getAttribute("position")._idx;
	glEnableVertexAttribArray(attribPos);
	glVertexAttribPointer(attribPos, 3, GL_FLOAT, GL_TRUE, 3 * sizeof(float), 0);
	glDrawElements(GL_TRIANGLES, 3 * sud->_numTriangles, GL_UNSIGNED_SHORT, 0);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glStencilFunc(GL_EQUAL, 1, (GLuint)~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void GfxOpenGLS::setShadowMode() {
	GfxBase::setShadowMode();
}

void GfxOpenGLS::clearShadowMode() {
	GfxBase::clearShadowMode();

	glDisable(GL_STENCIL_TEST);
	glDepthMask(GL_TRUE);
}

bool GfxOpenGLS::isShadowModeActive() {
	return false;
}

void GfxOpenGLS::setShadowColor(byte r, byte g, byte b) {
	_shadowColorR = r;
	_shadowColorG = g;
	_shadowColorB = b;
}

void GfxOpenGLS::getShadowColor(byte *r, byte *g, byte *b) {
	*r = _shadowColorR;
	*g = _shadowColorG;
	*b = _shadowColorB;
}


void GfxOpenGLS::set3DMode() {

}


void GfxOpenGLS::translateViewpointStart() {
	_matrixStack.push(_matrixStack.top());
}

void GfxOpenGLS::translateViewpoint(const Math::Vector3d &vec) {
	Math::Matrix4 temp;
	temp.setPosition(vec);
	temp.transpose();
	_matrixStack.top() = temp * _matrixStack.top();
}

void GfxOpenGLS::rotateViewpoint(const Math::Angle &angle, const Math::Vector3d &axis_) {
	Math::Matrix4 temp = makeRotationMatrix(angle, axis_) * _matrixStack.top();
	_matrixStack.top() = temp;
}

void GfxOpenGLS::translateViewpointFinish() {
	_matrixStack.pop();
}

void GfxOpenGLS::updateEMIModel(const EMIModel* model) {
	const EMIModelUserData *mud = (const EMIModelUserData *)model->_userData;
	glBindBuffer(GL_ARRAY_BUFFER, mud->_verticesVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, model->_numVertices * 3 * sizeof(float), model->_drawVertices);
}

void GfxOpenGLS::drawEMIModelFace(const EMIModel* model, const EMIMeshFace* face) {
	const EMIModelUserData *mud = (const EMIModelUserData *)model->_userData;
	mud->_shader->use();
	mud->_shader->setUniform("textured", face->_hasTexture ? GL_TRUE : GL_FALSE);
	mud->_shader->setUniform("lightsEnabled", _lightsEnabled);
	mud->_shader->setUniform("swapRandB", _selectedTexture->_colorFormat == BM_BGRA || _selectedTexture->_colorFormat == BM_BGR888);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face->_indicesEBO);

	glDrawElements(GL_TRIANGLES, 3 * face->_faceLength, GL_UNSIGNED_INT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GfxOpenGLS::drawMesh(const Mesh *mesh) {
	const ModelUserData *mud = (const ModelUserData *)mesh->_userData;
	if (!mud)
		return;
	Graphics::Shader *actorShader = mud->_shader;

	actorShader->use();
	actorShader->setUniform("extraMatrix", _matrixStack.top());

	const Material *curMaterial = NULL;
	for (int i = 0; i < mesh->_numFaces;) {
		const MeshFace *face = &mesh->_faces[i];
		if (face->getLight() == 0 && !isShadowModeActive())
			disableLights();

		curMaterial = face->getMaterial();
		curMaterial->select();

		int faces = 0;
		for (; i < mesh->_numFaces; ++i) {
			if (mesh->_faces[i].getMaterial() != curMaterial)
				break;
			faces += 3 * (mesh->_faces[i].getNumVertices() - 2);
		}

		bool textured = face->hasTexture() && !_currentShadowArray;
		actorShader->setUniform("textured", textured ? GL_TRUE : GL_FALSE);
		actorShader->setUniform("texScale", Math::Vector2d(_selectedTexture->_width, _selectedTexture->_height));

		glDrawArrays(GL_TRIANGLES, *(int *)face->_userData, faces);

		if (face->getLight() == 0 && !isShadowModeActive())
			enableLights();
	}
}

void GfxOpenGLS::drawModelFace(const Mesh *mesh, const MeshFace *face) {

}

void GfxOpenGLS::drawSprite(const Sprite *sprite) {
	glDisable(GL_DEPTH_TEST);

	_spriteProgram->use();

	const Math::Quaternion quat = Math::Quaternion::fromEuler(0, 0, _currentActor->getYaw());
	const Math::Matrix4 &rotateMatrix = quat.toMatrix();

	Math::Matrix4 extraMatrix;
	extraMatrix.setPosition(sprite->_pos);
	extraMatrix(0,0) = sprite->_width;
	extraMatrix(1,1) = sprite->_height;

	extraMatrix = extraMatrix * rotateMatrix;
	extraMatrix.transpose();
	_spriteProgram->setUniform("extraMatrix", extraMatrix);
	_spriteProgram->setUniform("textured", GL_TRUE);
	_spriteProgram->setUniform("isBillboard", GL_TRUE);
	_spriteProgram->setUniform("lightsEnabled", false);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}


void GfxOpenGLS::enableLights() {
	_lightsEnabled = true;
}

void GfxOpenGLS::disableLights() {
	_lightsEnabled = false;
}

void GfxOpenGLS::setupLight(Grim::Light *light, int lightId) {
	_lightsEnabled = true;

	if (lightId >= _maxLights) {
		return;
	}

	// Disable previous lights.
	if (lightId == 0) {
		for (int id = 0; id < _maxLights; ++id)
			_lights[id]._color.w() = 0.0;
	}

	Math::Vector4d &lightColor = _lights[lightId]._color;
	Math::Vector4d &lightPos   = _lights[lightId]._position;
	Math::Vector4d &lightDir   = _lights[lightId]._direction;

	float intensity = light->_intensity / 1.3f;
	lightColor.x() = ((float)light->_color.getRed() / 15.0f) * intensity;
	lightColor.y() = ((float)light->_color.getGreen() / 15.0f) * intensity;
	lightColor.z() = ((float)light->_color.getBlue() / 15.0f) * intensity;
	lightColor.w() = 1.0f;

	if (light->_type == Grim::Light::Omni) {
		lightPos = Math::Vector4d(light->_pos.x(), light->_pos.y(), light->_pos.z(), 1.0f);
		lightDir = Math::Vector4d(0.0f, 0.0f, 0.0f, -1.0f);
	} else if (light->_type == Grim::Light::Direct) {
		lightPos = Math::Vector4d(-light->_dir.x(), -light->_dir.y(), -light->_dir.z(), 0.0f);
		lightDir = Math::Vector4d(0.0f, 0.0f, 0.0f, -1.0f);
	} else if (light->_type == Grim::Light::Spot) {
		lightPos = Math::Vector4d(light->_pos.x(), light->_pos.y(), light->_pos.z(), 1.0f);
		lightDir = Math::Vector4d(light->_dir.x(), light->_dir.y(), light->_dir.z(), Math::Angle(light->_penumbraangle).getCosine());
	}
}

void GfxOpenGLS::turnOffLight(int lightId) {
	if (lightId >= _maxLights) {
		return;
	}

	_lights[lightId]._color = Math::Vector4d(0.0f, 0.0f, 0.0f, 0.0f);
	_lights[lightId]._position = Math::Vector4d(0.0f, 0.0f, 0.0f, 0.0f);
	_lights[lightId]._direction = Math::Vector4d(0.0f, 0.0f, 0.0f, 0.0f);
}


void GfxOpenGLS::createMaterial(Texture *material, const char *data, const CMap *cmap) {
	material->_texture = new GLuint[1];
	glGenTextures(1, (GLuint *)material->_texture);
	char *texdata = new char[material->_width * material->_height * 4];
	char *texdatapos = texdata;

	if (cmap != NULL) { // EMI doesn't have colour-maps
		for (int y = 0; y < material->_height; y++) {
			for (int x = 0; x < material->_width; x++) {
				uint8 col = *(const uint8 *)(data);
				if (col == 0) {
					memset(texdatapos, 0, 4); // transparent
					if (!material->_hasAlpha) {
						texdatapos[3] = '\xff'; // fully opaque
					}
				} else {
					memcpy(texdatapos, cmap->_colors + 3 * (col), 3);
					texdatapos[3] = '\xff'; // fully opaque
				}
				texdatapos += 4;
				data++;
			}
		}
	} else {
		memcpy(texdata, data, material->_width * material->_height * material->_bpp);
	}

	GLuint format = 0;
	GLuint internalFormat = 0;
	if (material->_colorFormat == BM_RGBA) {
		format = GL_RGBA;
		internalFormat = GL_RGBA;
	} else if (material->_colorFormat == BM_BGRA) {
#ifdef USE_GLES2
		format = GL_RGBA;
		internalFormat = GL_RGBA;
#else
		format = GL_BGRA;
		internalFormat = GL_RGBA;
#endif
	} else {	// The only other colorFormat we load right now is BGR
#ifdef USE_GLES2
		format = GL_RGB;
		internalFormat = GL_RGB;
#else
		format = GL_BGR;
		internalFormat = GL_RGBA;
#endif
	}

	GLuint *textures = (GLuint *)material->_texture;
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	//Remove darkened lines in EMI intro
	if (g_grim->getGameType() == GType_MONKEY4) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, material->_width, material->_height, 0, format, GL_UNSIGNED_BYTE, texdata);
	delete[] texdata;
}

void GfxOpenGLS::selectMaterial(const Texture *material) {
	GLuint *textures = (GLuint *)material->_texture;
	glBindTexture(GL_TEXTURE_2D, textures[0]);

	if (material->_hasAlpha && g_grim->getGameType() == GType_MONKEY4) {
		glEnable(GL_BLEND);
	}

	_selectedTexture = const_cast<Texture *>(material);
}

void GfxOpenGLS::destroyMaterial(Texture *material) {

}

void GfxOpenGLS::createBitmap(BitmapData *bitmap) {
	if (bitmap->_format != 1) {
		for (int pic = 0; pic < bitmap->_numImages; pic++) {
			uint16 *zbufPtr = reinterpret_cast<uint16 *>(bitmap->getImageData(pic).getRawBuffer());
			for (int i = 0; i < (bitmap->_width * bitmap->_height); i++) {
				uint16 val = READ_LE_UINT16(zbufPtr + i);
				// fix the value if it is incorrectly set to the bitmap transparency color
				if (val == 0xf81f) {
					val = 0;
				}
				zbufPtr[i] = 0xffff - ((uint32)val) * 0x10000 / 100 / (0x10000 - val);
			}
		}
	}

	bitmap->_hasTransparency = false;
	if (bitmap->_format == 1) {
		bitmap->_numTex = 1;
		GLuint *textures = new GLuint[bitmap->_numTex * bitmap->_numImages];
		bitmap->_texIds = textures;
		glGenTextures(bitmap->_numTex * bitmap->_numImages, textures);

		byte *texData = 0;
		byte *texOut = 0;

		GLint format = GL_RGBA;
		GLint type = GL_UNSIGNED_BYTE;
		int bytes = 4;

		glPixelStorei(GL_UNPACK_ALIGNMENT, bytes);

		for (int pic = 0; pic < bitmap->_numImages; pic++) {
			if (bitmap->_format == 1 && bitmap->_bpp == 16 && bitmap->_colorFormat != BM_RGB1555) {
				if (texData == 0)
					texData = new byte[4 * bitmap->_width * bitmap->_height];
				// Convert data to 32-bit RGBA format
				byte *texDataPtr = texData;
				uint16 *bitmapData = reinterpret_cast<uint16 *>(bitmap->getImageData(pic).getRawBuffer());
				for (int i = 0; i < bitmap->_width * bitmap->_height; i++, texDataPtr += 4, bitmapData++) {
					uint16 pixel = *bitmapData;
					int r = pixel >> 11;
					texDataPtr[0] = (r << 3) | (r >> 2);
					int g = (pixel >> 5) & 0x3f;
					texDataPtr[1] = (g << 2) | (g >> 4);
					int b = pixel & 0x1f;
					texDataPtr[2] = (b << 3) | (b >> 2);
					if (pixel == 0xf81f) { // transparent
						texDataPtr[3] = 0;
						bitmap->_hasTransparency = true;
					} else {
						texDataPtr[3] = 255;
					}
				}
				texOut = texData;
			} else if (bitmap->_format == 1 && bitmap->_colorFormat == BM_RGB1555) {
				bitmap->convertToColorFormat(pic, Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));
				texOut = (byte *)bitmap->getImageData(pic).getRawBuffer();
			} else {
				texOut = (byte *)bitmap->getImageData(pic).getRawBuffer();

				//add transparency support
				if (bitmap->_format == 1 && bitmap->_bpp == 32 && bitmap->_colorFormat == BM_RGBA) {
					byte *texDataPtr = texOut;

					for (int i = 0; i < bitmap->_width * bitmap->_height; i++, texDataPtr += sizeof(uint32)) {
						uint32 *pixel = (uint32*)texDataPtr;
						if (*pixel == 0xff00ff) {
							texDataPtr[3] = 0;
							bitmap->_hasTransparency = true;
						} else {
							texDataPtr[3] = 255;
						}

					}
				}
			}

			int actualWidth = nextHigher2(bitmap->_width);
			int actualHeight = nextHigher2(bitmap->_height);

			glBindTexture(GL_TEXTURE_2D, textures[bitmap->_numTex * pic]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, format, actualWidth, actualHeight, 0, format, type, NULL);

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bitmap->_width, bitmap->_height, format, type, texOut);
		}

		if (texData)
			delete[] texData;
		bitmap->freeData();

		Graphics::Shader *shader = _backgroundProgram->clone();
		bitmap->_userData = shader;

		if (g_grim->getGameType() == GType_MONKEY4) {
			GLuint vbo = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, bitmap->_numCoords * 4 * sizeof(float), bitmap->_texc, GL_STATIC_DRAW);
			shader->enableVertexAttribute("position", vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
			shader->enableVertexAttribute("texcoord", vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2*sizeof(float));
		}
	} else {
		bitmap->_numTex = 0;
		bitmap->_texIds = NULL;
		bitmap->_userData = NULL;
	}
}


void GfxOpenGLS::drawBitmap(const Bitmap *bitmap, int dx, int dy, uint32 layer) {
	if (g_grim->getGameType() == GType_MONKEY4 && bitmap->_data->_numImages > 1) {
		BitmapData *data = bitmap->_data;
		Graphics::Shader *shader = (Graphics::Shader *)data->_userData;
		GLuint *textures = (GLuint *)bitmap->getTexIds();

		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		shader->use();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
		uint32 offset = data->_layers[layer]._offset;
		for (uint32 i = offset; i < offset + data->_layers[layer]._numImages; ++i) {
			glBindTexture(GL_TEXTURE_2D, textures[data->_verts[i]._texid]);

			unsigned short startVertex = data->_verts[i]._pos / 4 * 6;
			unsigned short numVertices = data->_verts[i]._verts / 4 * 6;
			glDrawElements(GL_TRIANGLES, numVertices, GL_UNSIGNED_SHORT, (void *)(startVertex * sizeof(unsigned short)));
		}
		return;
	}

	int format = bitmap->getFormat();
	if ((format == 1 && !_renderBitmaps) || (format == 5 && !_renderZBitmaps)) {
		return;
	}

	if (format == 1) {
		GLuint *textures = (GLuint *)bitmap->getTexIds();
		if (bitmap->getFormat() == 1 && bitmap->getHasTransparency()) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		} else {
			glDisable(GL_BLEND);
		}

		Graphics::Shader *shader = (Graphics::Shader *)bitmap->_data->_userData;
		shader->use();
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
		int cur_tex_idx = bitmap->getNumTex() * (bitmap->getActiveImage() - 1);
		glBindTexture(GL_TEXTURE_2D, textures[cur_tex_idx]);
		float width = bitmap->getWidth();
		float height = bitmap->getHeight();
		shader->setUniform("offsetXY", Math::Vector2d(float(dx) / _gameWidth, float(dy) / _gameHeight));
		shader->setUniform("sizeWH", Math::Vector2d(width / _gameWidth, height / _gameHeight));
		shader->setUniform("texcrop", Math::Vector2d(width / nextHigher2((int)width), height / nextHigher2((int)height)));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
	} else {
		// Only draw the manual zbuffer when enabled
		if (bitmap->getActiveImage() - 1 < bitmap->getNumImages()) {
			drawDepthBitmap(dx, dy, bitmap->getWidth(), bitmap->getHeight(), (char *)bitmap->getData(bitmap->getActiveImage() - 1).getRawBuffer());
		} else {
			warning("zbuffer image has index out of bounds! %d/%d", bitmap->getActiveImage(), bitmap->getNumImages());
		}
		return;
	}
}

void GfxOpenGLS::drawDepthBitmap(int x, int y, int w, int h, char *data) {
	static int prevX = -1, prevY = -1;
	static char *prevData = NULL;

	if (prevX == x && prevY == y && data == prevData)
		return;

	prevX = x;
	prevY = y;
	prevData = data;

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, _zBufTex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glActiveTexture(GL_TEXTURE0);
}

void GfxOpenGLS::destroyBitmap(BitmapData *bitmap) {
	GLuint *textures = (GLuint *)bitmap->_texIds;
	if (textures) {
		glDeleteTextures(bitmap->_numTex * bitmap->_numImages, textures);
		delete[] textures;
		bitmap->_texIds = 0;
	}
	Graphics::Shader *shader = (Graphics::Shader *)bitmap->_userData;
	if (g_grim->getGameType() == GType_MONKEY4) {
		glDeleteBuffers(1, &shader->getAttributeAt(0)._vbo);
	}
	delete shader;

	if (bitmap->_format != 1) {
		bitmap->freeData();
	}
}

void GfxOpenGLS::createFont(Font *font) {
	const byte *bitmapData = font->getFontData();
	uint dataSize = font->getDataSize();

	uint8 bpp = 4;
	uint8 charsWide = 16;
	uint8 charsHigh = 16;

	byte *texDataPtr = new byte[dataSize * bpp];
	byte *data = texDataPtr;

	for (uint i = 0; i < dataSize; i++, texDataPtr += bpp, bitmapData++) {
		byte pixel = *bitmapData;
		if (pixel == 0x00) {
			texDataPtr[0] = texDataPtr[1] = texDataPtr[2] = texDataPtr[3] = 0;
		} else if (pixel == 0x80) {
			texDataPtr[0] = texDataPtr[1] = texDataPtr[2] = 0;
			texDataPtr[3] = 255;
		} else if (pixel == 0xFF) {
			texDataPtr[0] = texDataPtr[1] = texDataPtr[2] = texDataPtr[3] = 255;
		}
	}
	int size = 0;
	for (int i = 0; i < 256; ++i) {
		int width = font->getCharDataWidth(i), height = font->getCharDataHeight(i);
		int m = MAX(width, height);
		if (m > size)
			size = m;
	}
	assert(size < 64);
	if (size < 8)
		size = 8;
	if (size < 16)
		size = 16;
	else if (size < 32)
		size = 32;
	else if (size < 64)
		size = 64;

	uint arraySize = size * size * bpp * charsWide * charsHigh;
	byte *temp = new byte[arraySize];

	memset(temp, 0, arraySize);

	FontUserData *userData = new FontUserData;
	font->setUserData(userData);
	userData->texture = 0;
	userData->size = size;

	GLuint *texture = &(userData->texture);
	glGenTextures(1, texture);

	for (int i = 0, row = 0; i < 256; ++i) {
		int width = font->getCharDataWidth(i), height = font->getCharDataHeight(i);
		int32 d = font->getCharOffset(i);
		for (int x = 0; x < height; ++x) {
			// a is the offset to get to the correct row.
			// b is the offset to get to the correct line in the character.
			// c is the offset of the character from the start of the row.
			uint a = row * size * size * bpp * charsHigh;
			uint b = x * size * charsWide * bpp;
			uint c = 0;
			if (i != 0)
				c = ((i - 1) % 16) * size * bpp;

			uint pos = a + b + c;
			uint pos2 = d * bpp + x * width * bpp;
			assert(pos + width * bpp <= arraySize);
			assert(pos2 + width * bpp <= dataSize * bpp);
			memcpy(temp + pos, data + pos2, width * bpp);
		}
		if (i != 0 && i % charsWide == 0)
			++row;

	}
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size * charsWide, size * charsHigh, 0, GL_RGBA, GL_UNSIGNED_BYTE, temp);

	delete[] data;
	delete[] temp;
}

void GfxOpenGLS::destroyFont(Font *font) {
	const FontUserData *data = (const FontUserData *)font->getUserData();
	if (data) {
		glDeleteTextures(1, &(data->texture));
		delete data;
	}
}

void GfxOpenGLS::createTextObject(TextObject *text) {
	const Color &color = text->getFGColor();
	const Font *font = text->getFont();

	const FontUserData *userData = (const FontUserData *)font->getUserData();
	if (!userData)
		error("Could not get font userdata");
	float sizeW = float(userData->size) / _gameWidth;
	float sizeH = float(userData->size) / _gameHeight;
	const Common::String *lines = text->getLines();
	int numLines = text->getNumLines();

	int numCharacters = 0;
	for (int j = 0; j < numLines; ++j) {
		numCharacters += lines[j].size();
	}

	float * bufData = new float[numCharacters * 16];
	float * cur = bufData;

	for (int j = 0; j < numLines; ++j) {
		const Common::String &line = lines[j];
		int x = text->getLineX(j);
		int y = text->getLineY(j);
		for (uint i = 0; i < line.size(); ++i) {
			uint8 character = line[i];
			float w = y + font->getCharStartingLine(character);
			if (g_grim->getGameType() == GType_GRIM)
				w += font->getBaseOffsetY();
			float z = x + font->getCharStartingCol(character);
			z /= _gameWidth;
			w /= _gameHeight;
			float width = 1 / 16.f;
			float cx = ((character - 1) % 16) / 16.0f;
			float cy = ((character - 1) / 16) / 16.0f;

			float charData[] = {
					z, w, cx, cy,
					z + sizeW, w, cx + width, cy,
					z + sizeW, w + sizeH, cx + width, cy + width,
					z, w + sizeH, cx, cy + width
			};
			memcpy(cur, charData, 16 * sizeof(float));
			cur += 16;

			x += font->getCharWidth(character);
		}
	}
	GLuint vbo;
	if (text->isBlastDraw()) {
		vbo = _blastVBO;
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, numCharacters * 16 * sizeof(float), bufData);
	} else {
		vbo = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, numCharacters * 16 * sizeof(float), bufData, GL_STATIC_DRAW);
	}

	Graphics::Shader * textShader = _textProgram->clone();
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	textShader->enableVertexAttribute("position", vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	textShader->enableVertexAttribute("texcoord", vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float));

	TextUserData * td = new TextUserData;
	td->characters = numCharacters;
	td->shader = textShader;
	td->color = color;
	td->texture = userData->texture;
	text->setUserData(td);
	delete[] bufData;
}

void GfxOpenGLS::drawTextObject(const TextObject *text) {
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	const TextUserData * td = (const TextUserData *) text->getUserData();
	assert(td);
	td->shader->use();

	Math::Vector3d colors(float(td->color.getRed()) / 255.0f,
	                      float(td->color.getGreen()) / 255.0f,
	                      float(td->color.getBlue()) / 255.0f);
	_textProgram->setUniform("color", colors);
	glBindTexture(GL_TEXTURE_2D, td->texture);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	glDrawElements(GL_TRIANGLES, td->characters * 6, GL_UNSIGNED_SHORT, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void GfxOpenGLS::destroyTextObject(TextObject *text) {
	const TextUserData * td = (const TextUserData *) text->getUserData();
	if (!text->isBlastDraw()) {
		glDeleteBuffers(1, &td->shader->getAttributeAt(0)._vbo);
	}
	text->setUserData(NULL);
	delete td;
}


Bitmap *GfxOpenGLS::getScreenshot(int w, int h) {
	Graphics::PixelBuffer buffer = Graphics::PixelBuffer::createBuffer<565>(w * h, DisposeAfterUse::YES);
	Graphics::PixelBuffer src(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24), _screenWidth * _screenHeight, DisposeAfterUse::YES);
	glReadPixels(0, 0, _screenWidth, _screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, src.getRawBuffer());

	int i1 = (_screenWidth * w - 1) / _screenWidth + 1;
	int j1 = (_screenHeight * h - 1) / _screenHeight + 1;

	for (int j = 0; j < j1; j++) {
		for (int i = 0; i < i1; i++) {
			int x0 = i * _screenWidth / w;
			int x1 = ((i + 1) * _screenWidth - 1) / w + 1;
			int y0 = j * _screenHeight / h;
			int y1 = ((j + 1) * _screenHeight - 1) / h + 1;
			uint32 color = 0;
			for (int y = y0; y < y1; y++) {
				for (int x = x0; x < x1; x++) {
					uint8 lr, lg, lb;
					src.getRGBAt(y * _screenWidth + x, lr, lg, lb);
					color += (lr + lg + lb) / 3;
				}
			}
			color /= (x1 - x0) * (y1 - y0);
			buffer.setPixelAt((h - j - 1) * w + i, color, color, color);
		}
	}

	Bitmap *screenshot = new Bitmap(buffer, w, h, "screenshot");
	return screenshot;
}

void GfxOpenGLS::storeDisplay() {

}

void GfxOpenGLS::copyStoredToDisplay() {

}


void GfxOpenGLS::dimScreen() {

}

void GfxOpenGLS::dimRegion(int x, int y, int w, int h, float level) {

}


void GfxOpenGLS::irisAroundRegion(int x1, int y1, int x2, int y2) {
	_irisProgram->use();
	_irisProgram->setUniform("color", Math::Vector3d(0.0f, 0.0f, 0.0f));
	_irisProgram->setUniform("scaleWH", Math::Vector2d(1.f / _gameWidth, 1.f / _gameHeight));

	float fx1 = x1;
	float fx2 = x2;
	float fy1 = y1;
	float fy2 = y2;
	float width = _screenWidth;
	float height = _screenHeight;
	float points[20] = {
		0.0f, 0.0f,
		0.0f, fy1,
		width, 0.0f,
		fx2, fy1,
		width, height,
		fx2, fy2,
		0.0f, height,
		fx1, fy2,
		0.0f, fy1,
		fx1, fy1
	};

	glBindBuffer(GL_ARRAY_BUFFER, _irisVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 20 * sizeof(float), points);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 10);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
}


void GfxOpenGLS::drawEmergString(int x, int y, const char *text, const Color &fgColor) {
	if (!*text)
		return;
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBindTexture(GL_TEXTURE_2D, _emergTexture);
	_emergProgram->use();
	Math::Vector3d colors(float(fgColor.getRed()) / 255.0f,
	                      float(fgColor.getGreen()) / 255.0f,
	                      float(fgColor.getBlue()) / 255.0f);
	_emergProgram->setUniform("color", colors);
	_emergProgram->setUniform("sizeWH", Math::Vector2d(float(8) / _gameWidth, float(16) / _gameHeight));
	_emergProgram->setUniform("texScale", Math::Vector2d(float(8) / 128, float(16) / 128));

	for (; *text; ++text, x+=10) {
		int blockcol = *text & 0xf;
		int blockrow = *text / 16;
		_emergProgram->setUniform("offsetXY", Math::Vector2d(float(x) / _gameWidth, float(y) / _gameHeight));
		_emergProgram->setUniform("texOffsetXY", Math::Vector2d(float(blockcol * 8) / 128, float(blockrow * 16) / 128));
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}
}

void GfxOpenGLS::loadEmergFont() {
	uint8 *atlas = new uint8[128 * 128];
	memset(atlas, 0, 128 * 128);

	for (int c = 32; c < 128; ++c) {
		int blockrow = c / 16;
		int blockcol = c & 0xf;
		for (int row = 0; row < 13; ++row) {
			int base = 128 * (16 * blockrow + row) + 8 * blockcol;
			uint8 val = Font::emerFont[c-32][row];
			atlas[base+0] = (val & 0x80) ? 255 : 0;
			atlas[base+1] = (val & 0x40) ? 255 : 0;
			atlas[base+2] = (val & 0x20) ? 255 : 0;
			atlas[base+3] = (val & 0x10) ? 255 : 0;
			atlas[base+4] = (val & 0x08) ? 255 : 0;
			atlas[base+5] = (val & 0x04) ? 255 : 0;
			atlas[base+6] = (val & 0x02) ? 255 : 0;
			atlas[base+7] = (val & 0x01) ? 255 : 0;
		}
	}

	glGenTextures(1, &_emergTexture);
	glBindTexture(GL_TEXTURE_2D, _emergTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 128, 128, 0, GL_ALPHA, GL_UNSIGNED_BYTE, atlas);

	delete[] atlas;
}

void GfxOpenGLS::drawGenericPrimitive(const float *vertices, uint32 numVertices, const PrimitiveObject *primitive) {
	const Color color(primitive->getColor());
	const Math::Vector3d colorV =
	  Math::Vector3d(color.getRed(), color.getGreen(), color.getBlue()) / 255.f;

	GLuint prim = nextPrimitive();
	glBindBuffer(GL_ARRAY_BUFFER, prim);
	glBufferSubData(GL_ARRAY_BUFFER, 0, numVertices * sizeof(float), vertices);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	_primitiveProgram->enableVertexAttribute("position", prim, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	_primitiveProgram->use(true);
	_primitiveProgram->setUniform("color", colorV);
	_primitiveProgram->setUniform("scaleWH", Math::Vector2d(1.f / _gameWidth, 1.f / _gameHeight));

	switch (primitive->getType()) {
		case PrimitiveObject::RectangleType:
			if (primitive->isFilled()) {
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			} else {
				glDrawArrays(GL_LINE_LOOP, 0, 4);
			}
			break;
		case PrimitiveObject::LineType:
			glDrawArrays(GL_LINES, 0, 2);
			break;
		case PrimitiveObject::PolygonType:
			glDrawArrays(GL_LINES, 0, 4);
			break;
		default:
			/* Impossible */
			break;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

void GfxOpenGLS::drawRectangle(const PrimitiveObject *primitive) {
	float x1 = primitive->getP1().x * _scaleW;
	float y1 = primitive->getP1().y * _scaleH;
	float x2 = primitive->getP2().x * _scaleW;
	float y2 = primitive->getP2().y * _scaleH;

	float data[] = { x1, y1, x2, y1, x2, y2, x1, y2 };

	drawGenericPrimitive(data, 8, primitive);
}

void GfxOpenGLS::drawLine(const PrimitiveObject *primitive) {
	float x1 = primitive->getP1().x * _scaleW;
	float y1 = primitive->getP1().y * _scaleH;
	float x2 = primitive->getP2().x * _scaleW;
	float y2 = primitive->getP2().y * _scaleH;

	float data[] = { x1, y1, x2, y2 };

	drawGenericPrimitive(data, 4, primitive);
}

void GfxOpenGLS::drawPolygon(const PrimitiveObject *primitive) {
	float x1 = primitive->getP1().x * _scaleW;
	float y1 = primitive->getP1().y * _scaleH;
	float x2 = primitive->getP2().x * _scaleW;
	float y2 = primitive->getP2().y * _scaleH;
	float x3 = primitive->getP3().x * _scaleW;
	float y3 = primitive->getP3().y * _scaleH;
	float x4 = primitive->getP4().x * _scaleW;
	float y4 = primitive->getP4().y * _scaleH;

	const float data[] = { x1, y1, x2, y2, x3, y3, x4, y4 };

	drawGenericPrimitive(data, 8, primitive);
}

void GfxOpenGLS::prepareMovieFrame(Graphics::Surface* frame) {
	int width = frame->w;
	int height = frame->h;
	const byte *bitmap = (const byte *)frame->getPixels();

	GLenum frameType, frameFormat;

	switch (frame->format.bytesPerPixel) {
	case 2:
		frameType = GL_UNSIGNED_SHORT_5_6_5;
		frameFormat = GL_RGB;
		_smushSwizzle = false;
		break;
	case 4:
		frameType = GL_UNSIGNED_BYTE;
		frameFormat = GL_RGBA;
		_smushSwizzle = true;
		break;
	default:
		error("Video decoder returned invalid pixel format!");
		return;
	}

	// create texture
	if (_smushTexId == 0) {
		glGenTextures(1, &_smushTexId);
	}
	glBindTexture(GL_TEXTURE_2D, _smushTexId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, frameFormat, nextHigher2(width), nextHigher2(height), 0, frameFormat, frameType, NULL);

	glPixelStorei(GL_UNPACK_ALIGNMENT, frame->format.bytesPerPixel);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, frameFormat, frameType, bitmap);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	_smushWidth = (int)(width);
	_smushHeight = (int)(height);
}

void GfxOpenGLS::drawMovieFrame(int offsetX, int offsetY) {
	_smushProgram->use();
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _quadEBO);
	_smushProgram->setUniform("texcrop", Math::Vector2d(float(_smushWidth) / nextHigher2(_smushWidth), float(_smushHeight) / nextHigher2(_smushHeight)));
	_smushProgram->setUniform("scale", Math::Vector2d(float(_smushWidth)/ float(_gameWidth), float(_smushHeight) / float(_gameHeight)));
	_smushProgram->setUniform("offset", Math::Vector2d(float(offsetX) / float(_gameWidth), float(offsetY) / float(_gameHeight)));
	_smushProgram->setUniform("swizzle", _smushSwizzle);
	glBindTexture(GL_TEXTURE_2D, _smushTexId);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}


void GfxOpenGLS::releaseMovieFrame() {
	if (_smushTexId > 0) {
		glDeleteTextures(1, &_smushTexId);
		_smushTexId = 0;
	}
}


const char *GfxOpenGLS::getVideoDeviceName() {
	return "OpenGLS Renderer";
}

void GfxOpenGLS::renderBitmaps(bool render) {

}

void GfxOpenGLS::renderZBitmaps(bool render) {

}

static void readPixels(int x, int y, int width, int height, char *buffer) {
	char *p = buffer;
	for (int i = y; i < y + height; i++) {
		glReadPixels(x, 479 - i, width, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);
		p += width * 4;
	}
}

void GfxOpenGLS::createSpecialtyTextures() {
	//make a buffer big enough to hold any of the textures
	char *buffer = new char[256*256*4];

	readPixels(0, 0, 256, 256, buffer);
	_specialty[0].create(buffer, 256, 256);

	readPixels(256, 0, 256, 256, buffer);
	_specialty[1].create(buffer, 256, 256);

	readPixels(512, 0, 128, 128, buffer);
	_specialty[2].create(buffer, 128, 128);

	readPixels(512, 128, 128, 128, buffer);
	_specialty[3].create(buffer, 128, 128);

	readPixels(0, 256, 256, 256, buffer);
	_specialty[4].create(buffer, 256, 256);

	readPixels(256, 256, 256, 256, buffer);
	_specialty[5].create(buffer, 256, 256);

	readPixels(512, 256, 128, 128, buffer);
	_specialty[6].create(buffer, 128, 128);

	readPixels(512, 384, 128, 128, buffer);
	_specialty[7].create(buffer, 128, 128);

	delete[] buffer;
}

void GfxOpenGLS::createEMIModel(EMIModel *model) {
	EMIModelUserData *mud = new EMIModelUserData;
	model->_userData = mud;
	mud->_verticesVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, model->_numVertices * 3 * sizeof(float), model->_vertices, GL_STREAM_DRAW);

//	model->_normalsVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, model->_numVertices * 3 * sizeof(float), model->_normals, GL_STATIC_DRAW);;

	mud->_texCoordsVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, model->_numVertices * 2 * sizeof(float), model->_texVerts, GL_STATIC_DRAW);

	mud->_colorMapVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, model->_numVertices * 4 * sizeof(byte), model->_colorMap, GL_STATIC_DRAW);

	Graphics::Shader * actorShader = _actorProgram->clone();
	actorShader->enableVertexAttribute("position", mud->_verticesVBO, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	actorShader->enableVertexAttribute("texcoord", mud->_texCoordsVBO, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
	actorShader->enableVertexAttribute("color", mud->_colorMapVBO, 4, GL_UNSIGNED_BYTE, GL_TRUE, 4 * sizeof(byte), 0);
	mud->_shader = actorShader;

	for (uint32 i = 0; i < model->_numFaces; ++i) {
		EMIMeshFace * face = &model->_faces[i];
		face->_indicesEBO = Graphics::Shader::createBuffer(GL_ELEMENT_ARRAY_BUFFER, face->_faceLength * 3 * sizeof(uint32), face->_indexes, GL_STATIC_DRAW);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GfxOpenGLS::createModel(Mesh *mesh) {
	Common::Array<GrimVertex> meshInfo;
	meshInfo.reserve(mesh->_numVertices * 5);
	for (int i = 0; i < mesh->_numFaces; ++i) {
		MeshFace *face = &mesh->_faces[i];
		face->_userData = new uint32;
		*(uint32 *)face->_userData = meshInfo.size();

		if (face->getNumVertices() < 3)
			continue;

#define VERT(j) (&mesh->_vertices[3 * face->getVertex(j)])
#define TEXVERT(j) (face->hasTexture() ? &mesh->_textureVerts[2 * face->getTextureVertex(j)] : zero_texVerts)
#define NORMAL(j) (&mesh->_vertNormals[3 * face->getVertex(j)])

		for (int j = 2; j < face->getNumVertices(); ++j) {
			meshInfo.push_back(GrimVertex(VERT(0), TEXVERT(0), NORMAL(0)));
			meshInfo.push_back(GrimVertex(VERT(j-1), TEXVERT(j-1), NORMAL(j-1)));
			meshInfo.push_back(GrimVertex(VERT(j), TEXVERT(j), NORMAL(j)));
		}

#undef VERT
#undef TEXVERT
#undef NORMAL

	}

	if (meshInfo.empty()) {
		mesh->_userData = NULL;
		return;
	}

	ModelUserData *mud = new ModelUserData;
	mesh->_userData = mud;

	mud->_meshInfoVBO = Graphics::Shader::createBuffer(GL_ARRAY_BUFFER, meshInfo.size() * sizeof(GrimVertex), &meshInfo[0], GL_STATIC_DRAW);

	Graphics::Shader *shader = _actorProgram->clone();
	mud->_shader = shader;
	shader->enableVertexAttribute("position", mud->_meshInfoVBO, 3, GL_FLOAT, GL_FALSE, sizeof(GrimVertex), 0);
	shader->enableVertexAttribute("texcoord", mud->_meshInfoVBO, 2, GL_FLOAT, GL_FALSE, sizeof(GrimVertex), 3 * sizeof(float));
	shader->enableVertexAttribute("normal", mud->_meshInfoVBO, 3, GL_FLOAT, GL_FALSE, sizeof(GrimVertex), 5 * sizeof(float));
	shader->disableVertexAttribute("color", Math::Vector4d(1.f, 1.f, 1.f, 1.f));
}


}

#endif
