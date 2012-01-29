
#include "common/system.h"
#include "common/endian.h"
#include "common/streamdebug.h"

#include "math/vector3d.h"

#include "graphics/pixelbuffer.h"

#include "graphics/agl/texture.h"
#include "graphics/agl/light.h"
#include "graphics/agl/primitive.h"

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

class GLPrimitive : public Primitive {
public:
	GLPrimitive()
		: Primitive() {

	}

	void draw(float x, float y) {
		int _screenWidth = 640;
		int _screenHeight = 480;

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, _screenWidth, _screenHeight, 0, 0, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(x, y, 0);

		glDisable(GL_LIGHTING);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		GLenum mode;
		switch(getMode()) {
			case Points:
				mode = GL_POINTS;
				break;
			case Lines:
				mode = GL_LINES;
				break;
			case LineLoop:
				mode = GL_LINE_LOOP;
				break;
			case Quads:
				mode = GL_QUADS;
		}

		glBegin(mode);
		uint num = getNumVertices();
		for (uint i = 0; i < num; ++i) {
			const Graphics::Color &c = getColor(i);
			const Math::Vector2d &v = getVertex(i);
			glColor4ubv(c.getData());
			glVertex2fv(v.getData());
			if (breaksAt(i)) {
				glEnd();
				glBegin(mode);
			}
		}
		glEnd();

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

		assert(_id > -1);

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

class GLTexture : public Texture {
public:
	GLTexture(const Graphics::PixelBuffer &buf, int width, int height)
		: Texture(buf.getFormat(), width, height) {
		GLuint format = 0;
		GLuint internalFormat = 0;
// 		if (material->_colorFormat == BM_RGBA) {
			format = GL_RGBA;
			internalFormat = GL_RGBA;
// 		} else {	// The only other colorFormat we load right now is BGR
// 		format = GL_BGR;
// 		internalFormat = GL_RGB;
// 		}

		glGenTextures(1, &_texId);
		glBindTexture(GL_TEXTURE_2D, _texId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, buf.getRawBuffer());
	}

	void bind() {
		glBindTexture(GL_TEXTURE_2D, _texId);
	}

	GLuint _texId;
};

Target *OpenGLRenderer::setupScreen(int screenW, int screenH, bool fullscreen, int bpp) {
	g_system->setupScreen(screenW, screenH, fullscreen, true);

// 	_screenWidth = screenW;
// 	_screenHeight = screenH;
// 	_isFullscreen = g_system->getFeatureState(OSystem::kFeatureFullscreenMode);
	_useDepthShader = false;
//
	// Load emergency built-in font
	loadEmergFont();
//
// 	_screenSize = _screenWidth * _screenHeight * 4;
// 	_storedDisplay = new byte[_screenSize];
// 	memset(_storedDisplay, 0, _screenSize);
// 	_smushNumTex = 0;
//
// 	_currentShadowArray = NULL;
//
	GLfloat ambientSource[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientSource);

	glEnable(GL_LIGHTING);

// 	glPolygonOffset(-6.0, -6.0);

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

void OpenGLRenderer::positionCamera(const Math::Vector3d &pos, const Math::Vector3d &interest) {
	Math::Vector3d up_vec(0, 0, 1);

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

void OpenGLRenderer::loadEmergFont() {
// 	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//
// 	_emergFont = glGenLists(128);
// 	for (int i = 32; i < 127; i++) {
// 		glNewList(_emergFont + i, GL_COMPILE);
// 		glBitmap(8, 13, 0, 2, 10, 0, Font::emerFont[i - 32]);
// 		glEndList();
// 	}
}

#define BITMAP_TEXTURE_SIZE 256

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
void OpenGLRenderer::popMatrix() {
	glPopMatrix();
}

const char *OpenGLRenderer::prettyString() const {
	char GLDriver[1024];
	sprintf(GLDriver, "ResidualVM: %s/%s", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
	return GLDriver;
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
