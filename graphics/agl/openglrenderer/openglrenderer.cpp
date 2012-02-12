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

#include "common/system.h"
#include "common/endian.h"
#include "common/streamdebug.h"
#include "common/foreach.h"

#include "math/vector3d.h"
#include "math/rect2d.h"

#include "graphics/pixelbuffer.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/texture.h"
#include "graphics/agl/light.h"
#include "graphics/agl/primitive.h"
#include "graphics/agl/shadowplane.h"
#include "graphics/agl/label.h"
#include "graphics/agl/font.h"
#include "graphics/agl/sprite.h"

#include "graphics/agl/openglrenderer/openglrenderer.h"
#include "graphics/agl/openglrenderer/gltarget.h"
#include "graphics/agl/openglrenderer/glmesh.h"
#include "graphics/agl/openglrenderer/glbitmap2d.h"

#ifdef USE_OPENGL

#if defined (SDL_BACKEND) && defined(GL_ARB_fragment_program)

// We need SDL.h for SDL_GL_GetProcAddress.
#include "backends/platform/sdl/sdl-sys.h"

// Extension functions needed for fragment programs.
PFNGLGENPROGRAMSARBPROC glGenProgramsARB;
PFNGLBINDPROGRAMARBPROC glBindProgramARB;
PFNGLPROGRAMSTRINGARBPROC glProgramStringARB;
PFNGLDELETEPROGRAMSARBPROC glDeleteProgramsARB;

#endif

namespace AGL {

class GLSprite: public Sprite {
public:
	GLSprite(float width, float height)
		: Sprite(width, height) {

	}

	void draw(Texture *tex, float x, float y, float z) const {
		if (_renderer->_shadowActive) {
			return;
		}

		tex->bind();

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(x, y, z);

		GLdouble modelview[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

		// We want screen-aligned sprites so reset the rotation part of the matrix.
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				if (i == j) {
					modelview[i * 4 + j] = 1.0f;
				} else {
					modelview[i * 4 + j] = 0.0f;
				}
			}
		}
		glLoadMatrixd(modelview);

		glEnable(GL_TEXTURE_2D);

		glAlphaFunc(GL_GREATER, 0.5);
		glEnable(GL_ALPHA_TEST);
		glDisable(GL_LIGHTING);

		const float w = getWidth() / 2.f;
		const float h = getHeight();

		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(w, h, 0.0f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(w, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(-w, 0.0f, 0.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(-w, h, 0.0f);
		glEnd();

		glEnable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_TEXTURE_2D);

		glPopMatrix();
	}

	OpenGLRenderer *_renderer;
};

class GLFont : public Font {
public:
	GLFont(FontMetric *metric, const Graphics::PixelBuffer &buf, int width, int height)
		: Font(metric) {
		_texture = AGLMan.createTexture(buf, width, height);
	}

	~GLFont() {
		delete _texture;
	}

	void bind() {
		_texture->bind();
	}

	Texture *_texture;
};

class GLLabel : public Label {
public:
	GLLabel()
		: Label() {

	}

	void draw(int x, int y) const {
		const int screenWidth = AGLMan.getTarget()->getWidth();
		const int screenHeight = AGLMan.getTarget()->getHeight();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, screenWidth, screenHeight, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glDepthMask(GL_FALSE);

		GLFont *font = static_cast<GLFont *>(getFont());

		glColor3ubv(getTextColor().getData());

		font->bind();
		FontMetric *metric = font->getMetric();

		int numLines = getNumLines();
		for (int j = 0; j < numLines; ++j) {
			const Common::String &line = getLine(j);
			Common::Rect lineRect = getLineRect(j);

			int lx = x + lineRect.left;
			int ly = y + lineRect.top;

			for (uint i = 0; i < line.size(); ++i) {
				uint8 character = line[i];
				Math::Rect2d texrect = metric->getCharTextureRect(character);
				Math::Rect2d quadrect = metric->getCharQuadRect(character);

				quadrect.translate(Math::Vector2d(lx, ly));

				glBegin(GL_QUADS);
				glTexCoord2fv(texrect.getTopLeft().getData());
				glVertex2fv(quadrect.getTopLeft().getData());

				glTexCoord2fv(texrect.getTopRight().getData());
				glVertex2fv(quadrect.getTopRight().getData());

				glTexCoord2fv(texrect.getBottomRight().getData());
				glVertex2fv(quadrect.getBottomRight().getData());

				glTexCoord2fv(texrect.getBottomLeft().getData());
				glVertex2fv(quadrect.getBottomLeft().getData());
				glEnd();

				lx += metric->getCharWidth(character);
			}
		}

		glColor3f(1, 1, 1);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
		glDepthMask(GL_TRUE);
	}
};

class GLPrimitive : public Primitive {
public:
	GLPrimitive()
		: Primitive() {

	}

	void draw(float x, float y) {
		const int screenWidth = AGLMan.getTarget()->getWidth();
		const int screenHeight = AGLMan.getTarget()->getHeight();

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, screenWidth, screenHeight, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(x, y, 0);

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		GLenum mode = OpenGLRenderer::drawMode(getMode());

		const bool globalColor = useGlobalColor();
		if (globalColor) {
			glColor4ubv(getGlobalColor().getData());
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		uint num = getNumSubs();
		for (uint i = 0; i < num; ++i) {
			if (!globalColor) {
				glColorPointer(4, GL_UNSIGNED_BYTE, 0, getColorPointer(i));
			}
			glVertexPointer(2, GL_FLOAT, 0, getVertexPointer(i));
			glDrawArrays(mode, 0, getNumVertices(i));
		}
		glDisableClientState(GL_VERTEX_ARRAY);

		glColor3f(1.0f, 1.0f, 1.0f);

		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LIGHTING);
	}
};

class GLLight : public Light {
public:
	GLLight(Light::Type type)
		: Light(type),
		  _id(-1) { }

	void enable() {
		if (_id == -1) {
			// Find a free id.
			int max;
			glGetIntegerv(GL_MAX_LIGHTS, &max);
			for (int i = 0; i < max; ++i) {
				if (!glIsEnabled(GL_LIGHT0 + i)) {
					_id = i;
					break;
				}
			}
		}

		if (_id == -1) {
			warning("Cannot init light.");
			return;
		}
// 		assert(_id > -1);

		glEnable(GL_LIGHTING);
		float lightColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float lightDir[] = { 0.0f, 0.0f, -1.0f };

		float intensity = getIntensity() / 1.3f;
		lightColor[0] = ((float)getColor().getRed() / 15.0f) * intensity;
		lightColor[1] = ((float)getColor().getGreen() / 15.0f) * intensity;
		lightColor[2] = ((float)getColor().getBlue() / 15.0f) * intensity;

		if (getType() == Light::Point) {
			memcpy(lightPos, getPosition().getData(), 3 * sizeof(float));
		} else if (getType() == Light::Directional) {
			lightPos[0] = -getDirection().x();
			lightPos[1] = -getDirection().y();
			lightPos[2] = -getDirection().z();
			lightPos[3] = 0;
		} else if (getType() == Light::Spot) {
			memcpy(lightPos, getPosition().getData(), 3 * sizeof(float));
			memcpy(lightDir, getDirection().getData(), 3 * sizeof(float));
		}

		glDisable(GL_LIGHT0 + _id);
		glLightfv(GL_LIGHT0 + _id, GL_DIFFUSE, lightColor);
		glLightfv(GL_LIGHT0 + _id, GL_POSITION, lightPos);
		glLightfv(GL_LIGHT0 + _id, GL_SPOT_DIRECTION, lightDir);
		glLightf(GL_LIGHT0 + _id, GL_SPOT_CUTOFF, getCutoff());
		glEnable(GL_LIGHT0 + _id);
	}
	void disable() {
		if (_id < 0)
			return;

		glDisable(GL_LIGHT0 + _id);
		_id = -1;
	}

	int _id;
};

static void glShadowProjection(const Math::Vector3d &light, const Math::Vector3d &plane, const Math::Vector3d &normal, bool dontNegate) {
	// Based on GPL shadow projection example by
	// (c) 2002-2003 Phaetos <phaetos@gaffga.de>
	float d, c;
	float mat[16];
	float nx, ny, nz, lx, ly, lz, px, py, pz;

	nx = normal.x();
	ny = normal.y();
	nz = normal.z();
	// for some unknown for me reason normal need negation
	if (!dontNegate) {
		nx = -nx;
		ny = -ny;
		nz = -nz;
	}
	lx = light.x();
	ly = light.y();
	lz = light.z();
	px = plane.x();
	py = plane.y();
	pz = plane.z();

	d = nx * lx + ny * ly + nz * lz;
	c = px * nx + py * ny + pz * nz - d;

	mat[0] = lx * nx + c;
	mat[4] = ny * lx;
	mat[8] = nz * lx;
	mat[12] = -lx * c - lx * d;

	mat[1] = nx * ly;
	mat[5] = ly * ny + c;
	mat[9] = nz * ly;
	mat[13] = -ly * c - ly * d;

	mat[2] = nx * lz;
	mat[6] = ny * lz;
	mat[10] = lz * nz + c;
	mat[14] = -lz * c - lz * d;

	mat[3] = nx;
	mat[7] = ny;
	mat[11] = nz;
	mat[15] = -d;

	glMultMatrixf((GLfloat *)mat);
}

class GLShadowPlane : public ShadowPlane {
public:
	GLShadowPlane()
		: ShadowPlane() { }

	void enable(const Math::Vector3d &pos, const Graphics::Color &color) {
		_renderer->_shadowActive = true;

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glClearStencil(~0);
		glClear(GL_STENCIL_BUFFER_BIT);

		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, (GLuint)~0);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);

		foreach (const Sector &s, getSectors()) {
			glBegin(GL_POLYGON);
			int num = s._vertices.size();
			for (int k = 0; k < num; k++) {
				glVertex3fv(s._vertices[k].getData());
			}
			glEnd();
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glStencilFunc(GL_EQUAL, 1, (GLuint)~0);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


		glEnable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);

		glColor3ubv(color.getData());
		glPushMatrix();
		glShadowProjection(pos, getSectors()[0]._vertices[0], getSectors()[0]._normal, false);
	}

	void disable() {
		glPopMatrix();
		glEnable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 1.0f);
		glDisable(GL_POLYGON_OFFSET_FILL);

		glDisable(GL_STENCIL_TEST);
		glDepthMask(GL_TRUE);

		_renderer->_shadowActive = false;
	}

	OpenGLRenderer *_renderer;
};

class GLTexture : public Texture {
public:
	GLTexture(const Graphics::PixelBuffer &buf, int width, int height)
		: Texture(buf.getFormat(), width, height) {
		GLuint format = 0;
		GLuint internalFormat = 0;

		if (buf.getFormat() == Graphics::PixelFormat(3, 8, 8, 8, 0, 16, 8, 0, 0)) {
			format = GL_BGR;
			internalFormat = GL_RGB;
		} else {
			format = GL_RGBA;
			internalFormat = GL_RGBA;
		}

		glGenTextures(1, &_texId);
		glBindTexture(GL_TEXTURE_2D, _texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, buf.getRawBuffer());
	}

	void bind() const {
		glBindTexture(GL_TEXTURE_2D, _texId);
	}

	GLuint _texId;
};

Target *OpenGLRenderer::setupScreen(int screenW, int screenH, bool fullscreen, int bpp) {
	g_system->setupScreen(screenW, screenH, fullscreen, true);

	_useDepthShader = false;

	GLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientSource);

	glEnable(GL_LIGHTING);

	glPolygonOffset(-6.0, -6.0);

	initExtensions();

	GLTarget *t = new GLTarget(screenW, screenH, bpp);
	t->_storedDisplay = new byte[screenW * screenH * 4];

	return t;
}

void OpenGLRenderer::setupCamera(float fov, float nclip, float fclip, float roll) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float right = nclip * tan(fov / 2 * (LOCAL_PI / 180));
	glFrustum(-right, right, -right * 0.75, right * 0.75, nclip, fclip);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(roll, 0, 0, -1);
}

void OpenGLRenderer::positionCamera(const Math::Matrix3x3 &m, const Math::Vector3d &pos, const Math::Vector3d &interest) {
	Math::Vector3d up_vec(m(2, 0), m(2, 1), (2, 2));

	const float mat[] = {
		m(0, 0), m(0, 1), m(0, 2), 0,
		m(1, 0), m(1, 1), m(1, 2), 0,
		m(2, 0), m(2, 1), m(2, 2), 0,
		0,       0,       0,       1
	};

	glLoadMatrixf(mat);

	if (pos.x() == interest.x() && pos.y() == interest.y())
		up_vec = Math::Vector3d(0, 1, 0);

	gluLookAt(pos.x(), pos.y(), pos.z(), interest.x(), interest.y(), interest.z(), up_vec.x(), up_vec.y(), up_vec.z());
}

void OpenGLRenderer::enableLighting() {
	glEnable(GL_LIGHTING);
}

void OpenGLRenderer::disableLighting() {
	glDisable(GL_LIGHTING);
}

// Simple ARB fragment program that writes the value from a texture to the Z-buffer.
static char fragSrc[] =
	"!!ARBfp1.0\n\
	TEMP d;\n\
	TEX d, fragment.texcoord[0], texture[0], 2D;\n\
	MOV result.depth, d.r;\n\
	END\n";

void OpenGLRenderer::initExtensions() {
#if defined (SDL_BACKEND) && defined(GL_ARB_fragment_program)
	union {
		void* obj_ptr;
		void (APIENTRY *func_ptr)();
	} u;
	// We're casting from an object pointer to a function pointer, the
	// sizes need to be the same for this to work.
	assert(sizeof(u.obj_ptr) == sizeof(u.func_ptr));
	u.obj_ptr = SDL_GL_GetProcAddress("glGenProgramsARB");
	glGenProgramsARB = (PFNGLGENPROGRAMSARBPROC)u.func_ptr;
	u.obj_ptr = SDL_GL_GetProcAddress("glBindProgramARB");
	glBindProgramARB = (PFNGLBINDPROGRAMARBPROC)u.func_ptr;
	u.obj_ptr = SDL_GL_GetProcAddress("glProgramStringARB");
	glProgramStringARB = (PFNGLPROGRAMSTRINGARBPROC)u.func_ptr;
	u.obj_ptr = SDL_GL_GetProcAddress("glDeleteProgramsARB");
	glDeleteProgramsARB = (PFNGLDELETEPROGRAMSARBPROC)u.func_ptr;

	const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
	if (strstr(extensions, "ARB_fragment_program")) {
		_useDepthShader = true;
	}

	if (_useDepthShader) {
		glGenProgramsARB(1, &_fragmentProgram);
		glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _fragmentProgram);

		GLint errorPos;
		glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(fragSrc), fragSrc);
		glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &errorPos);
		if (errorPos != -1) {
			warning("Error compiling fragment program:\n%s", glGetString(GL_PROGRAM_ERROR_STRING_ARB));
			_useDepthShader = false;
		}
	}
#endif
}

Bitmap2D *OpenGLRenderer::createBitmap2D(Bitmap2D::Type texType, const Graphics::PixelBuffer &buf, int width, int height) {
	GLBitmap2D *bitmap = new GLBitmap2D(this, texType, buf, width, height);
	return bitmap;
}

Texture *OpenGLRenderer::createTexture(const Graphics::PixelBuffer &buf, int width, int height) {
	GLTexture *t = new GLTexture(buf, width, height);
	return t;
}

Mesh *OpenGLRenderer::createMesh() {
	return new GLMesh();
}

Light *OpenGLRenderer::createLight(Light::Type type) {
	return new GLLight(type);
}

Primitive *OpenGLRenderer::createPrimitive() {
	return new GLPrimitive();
}

ShadowPlane *OpenGLRenderer::createShadowPlane() {
	GLShadowPlane *s = new GLShadowPlane();
	s->_renderer = this;
	return s;
}

Font *OpenGLRenderer::createFont(FontMetric *metric, const Graphics::PixelBuffer &buf, int width, int height) {
	return new GLFont(metric, buf, width, height);
}

Label *OpenGLRenderer::createLabel() {
	return new GLLabel();
}

Sprite *OpenGLRenderer::createSprite(float width, float height) {
	GLSprite *s = new GLSprite(width, height);
	s->_renderer = this;
	return s;
}

void OpenGLRenderer::pushMatrix() {
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
}
void OpenGLRenderer::translate(float x, float y, float z) {
	glTranslatef(x, y, z);
}
void OpenGLRenderer::rotate(float deg, float x, float y, float z) {
	glRotatef(deg, x, y, z);
}
void OpenGLRenderer::scale(float x, float y, float z) {
	glScalef(x, y, z);
}
void OpenGLRenderer::popMatrix() {
	glPopMatrix();
}

Common::String OpenGLRenderer::prettyName() const {
	return Common::String::format("ResidualVM: %s/%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
}

Common::String OpenGLRenderer::getName() const {
	return "OpenGL Renderer";
}

bool OpenGLRenderer::isHardwareAccelerated() const {
	return true;
}

GLenum OpenGLRenderer::drawMode(DrawMode mode) {
	switch(mode) {
		case Points:
			return GL_POINTS;
		case Lines:
			return GL_LINES;
		case LineLoop:
			return GL_LINE_LOOP;
		case Triangles:
			return GL_TRIANGLES;
		case Quads:
			return GL_QUADS;
		case Polygon:
			return GL_POLYGON;
	}

	return GL_TRIANGLES;
}


class OpenGLPlugin : public RendererPluginObject {
public:
	OpenGLRenderer *createInstance() {
		return new OpenGLRenderer();
	}

	const char *getName() const {
		return "OpenGL";
	}
};

}

REGISTER_PLUGIN_STATIC(OpenGL, PLUGIN_TYPE_AGL_RENDERER, AGL::OpenGLPlugin);

#endif
