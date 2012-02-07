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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#if defined (SDL_BACKEND) && !defined(__amigaos4__)
#include <SDL_opengl.h>
#undef ARRAYSIZE
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "graphics/agl/texture.h"

#include "graphics/agl/openglrenderer/openglrenderer.h"
#include "graphics/agl/openglrenderer/glmesh.h"

namespace AGL {

MeshFace *GLMesh::createFace() {
	GLMeshFace *f = new GLMeshFace(this);
	_faces.push_back(f);

	return f;
}

void GLMesh::draw(Texture *texture) {

}

bool GLMesh::calculate2DBoundingBox(Common::Rect *rect) const {
	GLdouble top = 1000;
	GLdouble right = -1000;
	GLdouble left = 1000;
	GLdouble bottom = -1000;
	GLdouble winX, winY, winZ;

	for (Faces::const_iterator i = _faces.begin(); i != _faces.end(); ++i) {
		GLMeshFace *face = static_cast<GLMeshFace *>(*i);

		int num = face->_vertices.size();
		for (int j = 0; j < num; ++j) {
			GLdouble modelView[16], projection[16];
			GLint viewPort[4];

			glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv(GL_VIEWPORT, viewPort);

			int n = 3 * face->_vertices[j];
			Math::Vector3d v(getVerticesArray() + n);

			gluProject(v.x(), v.y(), v.z(), modelView, projection, viewPort, &winX, &winY, &winZ);

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

	const int _gameHeight = 480;
	const int _gameWidth = 640;

	double tt = bottom;
	bottom = _gameHeight - top;
	top = _gameHeight - tt;

	if (left < 0)
		left = 0;
	if (right >= _gameWidth)
		right = _gameWidth - 1;
	if (top < 0)
		top = 0;
	if (bottom >= _gameHeight)
		bottom = _gameHeight - 1;

	if (top >= _gameHeight || left >= _gameWidth || bottom < 0 || right < 0) {
		return false;
	}

	rect->left = left;
	rect->top = top;
	rect->right = right;
	rect->bottom = bottom;
	return true;
}

// -- GLMeshFace --

GLMeshFace::GLMeshFace(GLMesh *parent)
	: MeshFace(parent),
		_parent(parent) {

	_useColors = false;
	_useTexture = false;
}

void GLMeshFace::setNormal(float x, float y, float z) {
	_normal = Math::Vector3d(x, y, z);
}

void GLMeshFace::vertex(int index) {
	_vertices.push_back(index);
}
void GLMeshFace::texture(int index) {
	_textures.push_back(index);
	_useTexture = true;
}

void GLMeshFace::normal(int index) {
	_normals.push_back(index);
}

void GLMeshFace::color(int index) {
	_colors.push_back(index);
	_useColors = true;
}

void GLMeshFace::draw(Texture *tex) {
	if (tex) {
		tex->bind();

		if (_parent->getUseAbsoluteTexCoords()) {
			glPushMatrix();
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glScalef(1.0f / tex->getWidth(), 1.0f / tex->getHeight(), 1);
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}

		glEnable(GL_TEXTURE_2D);
		if (tex->hasAlpha()) {
			glAlphaFunc(GL_GREATER, 0.5);
			glEnable(GL_ALPHA_TEST);
		}
	}

	GLenum mode = OpenGLRenderer::drawMode(_parent->getDrawMode());

	glNormal3fv(_normal.getData());
	glBegin(mode);
	int num = _vertices.size();
	for (int i = 0; i < num; ++i) {
		int n = 3 * _normals[i];
		glNormal3fv(_parent->getNormalsArray() + n);

		if (_useTexture) {
			int t = 2 * _textures[i];
			glTexCoord2fv(_parent->getTexCoordsArray() + t);
		}
		if (_useColors) {
			int c = 4 * _colors[i];
			glColor4fv(_parent->getColorsArray() + c);
		}

		int v = 3 * _vertices[i];
		glVertex3fv(_parent->getVerticesArray() + v);
	}
	glEnd();

	if (tex) {
		if (tex->hasAlpha()) {
			glDisable(GL_ALPHA_TEST);
		}
		glDisable(GL_TEXTURE_2D);
	}
}



}
