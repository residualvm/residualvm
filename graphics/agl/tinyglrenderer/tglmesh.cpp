
#include "graphics/tinygl/gl.h"

#include "graphics/agl/texture.h"

#include "graphics/agl/tinyglrenderer/tglmesh.h"

namespace AGL {

void TGLMesh::pushVertex(float x, float y, float z) {
	_vertices.push_back(x);
	_vertices.push_back(y);
	_vertices.push_back(z);
}

void TGLMesh::pushTexVertex(float u, float v) {
	_textures.push_back(u);
	_textures.push_back(v);
}

void TGLMesh::pushNormal(float x, float y, float z) {
	_normals.push_back(x);
	_normals.push_back(y);
	_normals.push_back(z);
}

void TGLMesh::pushColor(float r, float g, float b, float a) {
	_colors.push_back(r);
	_colors.push_back(g);
	_colors.push_back(b);
	_colors.push_back(a);
}

MeshFace *TGLMesh::createFace() {
	TGLMeshFace *f = new TGLMeshFace(this);
	_faces.push_back(f);

	return f;
}

void TGLMesh::draw(Texture *texture) {

}

static void transformPoint(TGLfloat out[4], const TGLfloat m[16], const TGLfloat in[4]) {
#define M(row,col)  m[col * 4 + row]
	out[0] = M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
	out[1] = M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
	out[2] = M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
	out[3] = M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

TGLint tgluProject(TGLfloat objx, TGLfloat objy, TGLfloat objz, const TGLfloat model[16], const TGLfloat proj[16],
		const TGLint viewport[4], TGLfloat *winx, TGLfloat *winy, TGLfloat *winz) {
	TGLfloat in[4], out[4];

	in[0] = objx;
	in[1] = objy;
	in[2] = objz;
	in[3] = 1.0f;
	transformPoint(out, model, in);
	transformPoint(in, proj, out);

	if (in[3] == 0.0)
		return TGL_FALSE;

	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];

	*winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;
	*winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;
	*winz = (1 + in[2]) / 2;

	return TGL_TRUE;
}

bool TGLMesh::calculate2DBoundingBox(Common::Rect *rect) const {
	TGLfloat top = 1000;
	TGLfloat right = -1000;
	TGLfloat left = 1000;
	TGLfloat bottom = -1000;
	TGLfloat winX, winY, winZ;

	for (Faces::const_iterator i = _faces.begin(); i != _faces.end(); ++i) {
		TGLMeshFace *face = static_cast<TGLMeshFace *>(*i);

		int num = face->_vertices.size();
		for (int j = 0; j < num; ++j) {
			TGLfloat modelView[16], projection[16];
			TGLint viewPort[4];

			tglGetFloatv(TGL_MODELVIEW_MATRIX, modelView);
			tglGetFloatv(TGL_PROJECTION_MATRIX, projection);
			tglGetIntegerv(TGL_VIEWPORT, viewPort);

			int n = 3 * face->_vertices[j];
			Math::Vector3d v(_vertices.begin() + n);

			tgluProject(v.x(), v.y(), v.z(), modelView, projection, viewPort, &winX, &winY, &winZ);

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

TGLMeshFace::TGLMeshFace(TGLMesh *parent)
	: MeshFace(parent),
		_parent(parent) {

	_useTexture = false;
	_useColors = false;
}

void TGLMeshFace::setNormal(float x, float y, float z) {
	_normal = Math::Vector3d(x, y, z);
}

void TGLMeshFace::vertex(int index) {
	_vertices.push_back(index);
}

void TGLMeshFace::texture(int index) {
	_textures.push_back(index);
	_useTexture = true;
}

void TGLMeshFace::normal(int index) {
	_normals.push_back(index);
}

void TGLMeshFace::color(int index) {
	_colors.push_back(index);
	_useColors = true;
}

void TGLMeshFace::draw(Texture *tex) {
	tex->bind();

	if (_parent->getUseAbsoluteTexCoords()) {
		tglPushMatrix();
		tglMatrixMode(TGL_TEXTURE);
		tglLoadIdentity();
		tglScalef(1.0f / tex->getWidth(), 1.0f / tex->getHeight(), 1);
		tglMatrixMode(TGL_MODELVIEW);
		tglPopMatrix();
	}

	tglEnable(TGL_TEXTURE_2D);
	if (tex->hasAlpha()) {
		tglEnable(TGL_ALPHA_TEST);
	}

	tglNormal3fv(_normal.getData());
	tglBegin(TGL_POLYGON);
	int num = _vertices.size();
	for (int i = 0; i < num; ++i) {
		int n = 3 * _normals[i];
		tglNormal3f(_parent->_normals[n], _parent->_normals[n + 1], _parent->_normals[n + 2]);

		if (_useTexture) {
			int t = 2 * _textures[i];
			tglTexCoord2fv(_parent->_textures.begin() + t);
		}
		if (_useColors) {
			int c = 4 * _colors[i];
			tglColor4fv(_parent->_colors.begin() + c);
		}

		int v = 3 * _vertices[i];
		tglVertex3f(_parent->_vertices[v], _parent->_vertices[v + 1], _parent->_vertices[v + 2]);
	}
	tglEnd();

	if (tex->hasAlpha()) {
		tglDisable(TGL_ALPHA_TEST);
	}
	tglDisable(TGL_TEXTURE_2D);
}



}
