
#if defined (SDL_BACKEND) && !defined(__amigaos4__)
#include <SDL_opengl.h>
#undef ARRAYSIZE
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "graphics/agl/texture.h"

#include "graphics/agl/openglrenderer/glmesh.h"

namespace AGL {


void GLMesh::pushVertex(float x, float y, float z) {
	_vertices.push_back(x);
	_vertices.push_back(y);
	_vertices.push_back(z);
}

void GLMesh::pushTexVertex(float u, float v) {
	_textures.push_back(u);
	_textures.push_back(v);
}

void GLMesh::pushNormal(float x, float y, float z) {
	_normals.push_back(x);
	_normals.push_back(y);
	_normals.push_back(z);
}

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
		Math::Vector3d v;

		for (int j = 0; j < face->_i + 1; ++j) {
			GLdouble modelView[16], projection[16];
			GLint viewPort[4];

			glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
			glGetDoublev(GL_PROJECTION_MATRIX, projection);
			glGetIntegerv(GL_VIEWPORT, viewPort);

			int n = 3 * face->_vertices[j];
			Math::Vector3d v(_vertices.begin() + n);

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

}
void GLMeshFace::prepare(uint size) {
	_vertices = new int[size];
	_textures = new int[size];
	_normals = new int[size];
	_i = -1;
}

void GLMeshFace::setNormal(float x, float y, float z) {
	_normal = Math::Vector3d(x, y, z);
}

void GLMeshFace::vertex(int index) {
	++_i;
	_vertices[_i] = index;
}
void GLMeshFace::texture(int index) {
	_textures[_i] = index;
}
void GLMeshFace::normal(int index) {
	_normals[_i] = index;
}

void GLMeshFace::draw(Texture *texture) {
	texture->bind();

	if (_parent->getUseAbsoluteTexCoords()) {
		glPushMatrix();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glScalef(1.0f / texture->getWidth(), 1.0f / texture->getHeight(), 1);
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	glEnable(GL_TEXTURE_2D);
	if (texture->hasAlpha()) {
		glAlphaFunc(GL_GREATER, 0.5);
		glEnable(GL_ALPHA_TEST);
	}

	glNormal3fv(_normal.getData());
	glBegin(GL_POLYGON);
	for (int i = 0; i < _i + 1; ++i) {
		int n = 3 * _normals[i];
		glNormal3f(_parent->_normals[n], _parent->_normals[n + 1], _parent->_normals[n + 2]);

// 			if (face->_texVertices)
			int t = 2 * _textures[i];
			glTexCoord2f(_parent->_textures[t], _parent->_textures[t + 1]);

		int v = 3 * _vertices[i];
		glVertex3f(_parent->_vertices[v], _parent->_vertices[v + 1], _parent->_vertices[v + 2]);
	}
	glEnd();

	if (texture->hasAlpha()) {
		glDisable(GL_ALPHA_TEST);
	}
	glDisable(GL_TEXTURE_2D);
}



}
