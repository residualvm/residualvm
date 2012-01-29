
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


void GLMesh::prepare(uint size) {
	_vertices = new float[3 * size];
	_textures = new float[2 * size];
	_normals = new float[3 * size];
	_i = -1;
}

void GLMesh::vertex(float x, float y, float z) {
	++_i;
	_vertices[3 * _i] = x;
	_vertices[3 * _i + 1] = y;
	_vertices[3 * _i + 2] = z;
}

void GLMesh::texture(float u, float v) {
	_textures[2 * _i] = u;
	_textures[2 * _i + 1] = v;
}

void GLMesh::normal(float x, float y, float z) {
	_normals[3 * _i] = x;
	_normals[3 * _i + 1] = y;
	_normals[3 * _i + 2] = z;
}

MeshFace *GLMesh::createFace() {
	return new GLMeshFace(this);
}

void GLMesh::draw(Texture *texture) {

}




GLMeshFace::GLMeshFace(GLMesh *parent)
	: MeshFace(parent),
		_parent(parent) {

}
void GLMeshFace::prepare(uint size) {
	_vertices = new float[size*3];
	_textures = new float[size*2];
	_normals = new float[size*3];
// 	_vertices = new int[size];
// 	_textures = new int[size];
// 	_normals = new int[size];
	_i = -1;
}

void GLMeshFace::setNormal(float x, float y, float z) {
	_normal = Math::Vector3d(x, y, z);
}

void GLMeshFace::vertex(float x, float y, float z) {
	++_i;
	_vertices[3 * _i] = x;
	_vertices[3 * _i + 1] = y;
	_vertices[3 * _i + 2] = z;
}

void GLMeshFace::texture(float u, float v) {
	_textures[2 * _i] = u;
	_textures[2 * _i + 1] = v;
}

void GLMeshFace::normal(float x, float y, float z) {
	_normals[3 * _i] = x;
	_normals[3 * _i + 1] = y;
	_normals[3 * _i + 2] = z;
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
// 		glNormal3fv(_parent->_normals + 3 * _normals[i]);
// 		glNormal3fv(_normals + 3 * i);

// 			if (face->_texVertices)
			glTexCoord2fv(_textures + 2 * i);
// 			glTexCoord2fv(_parent->_textures + 2 *_textures[i]);

		glVertex3fv(_vertices + 3 * i);
// 		glVertex3fv(_parent->_vertices + 3 *_vertices[i]);
	}
	glEnd();

	if (texture->hasAlpha()) {
		glDisable(GL_ALPHA_TEST);
	}
	glDisable(GL_TEXTURE_2D);
}



}
