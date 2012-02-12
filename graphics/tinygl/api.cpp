
#include "graphics/tinygl/zgl.h"

// glVertex

void tglVertex4f(float x, float y, float z, float w) {
	TinyGL::GLParam p[5];

	p[0].op = TinyGL::OP_Vertex;
	p[1].f = x;
	p[2].f = y;
	p[3].f = z;
	p[4].f = w;

	TinyGL::gl_add_op(p);
}

void tglVertex2f(float x, float y)  {
	tglVertex4f(x, y, 0, 1);
}

void tglVertex2fv(const float *v)  {
	tglVertex4f(v[0], v[1], 0, 1);
}

void tglVertex3f(float x, float y, float z)  {
	tglVertex4f(x, y, z, 1);
}

void tglVertex3fv(const float *v)  {
	tglVertex4f(v[0], v[1], v[2], 1);
}

// glNormal

void tglNormal3f(float x, float y, float z) {
	TinyGL::GLParam p[4];

	p[0].op = TinyGL::OP_Normal;
	p[1].f = x;
	p[2].f = y;
	p[3].f = z;

	TinyGL::gl_add_op(p);
}

void tglNormal3fv(const float *v)  {
	tglNormal3f(v[0], v[1], v[2]);
}

// glColor

void tglColor4f(float r, float g, float b, float a) {
	TinyGL::GLParam p[8];

	p[0].op = TinyGL::OP_Color;
	p[1].f = r;
	p[2].f = g;
	p[3].f = b;
	p[4].f = a;
	// direct convertion to integer to go faster if no shading
	p[5].ui = (unsigned int)(r * (ZB_POINT_RED_MAX - ZB_POINT_RED_MIN) + ZB_POINT_RED_MIN);
	p[6].ui = (unsigned int)(g * (ZB_POINT_GREEN_MAX - ZB_POINT_GREEN_MIN) + ZB_POINT_GREEN_MIN);
	p[7].ui = (unsigned int)(b * (ZB_POINT_BLUE_MAX - ZB_POINT_BLUE_MIN) + ZB_POINT_BLUE_MIN);
	gl_add_op(p);
}

void tglColor4fv(const float *v) {
	TinyGL::GLParam p[8];

	p[0].op = TinyGL::OP_Color;
	p[1].f=v[0];
	p[2].f=v[1];
	p[3].f=v[2];
	p[4].f=v[3];
	// direct convertion to integer to go faster if no shading
	p[5].ui = (unsigned int)(v[0] * (ZB_POINT_RED_MAX - ZB_POINT_RED_MIN) + ZB_POINT_RED_MIN);
	p[6].ui = (unsigned int)(v[1] * (ZB_POINT_GREEN_MAX - ZB_POINT_GREEN_MIN) + ZB_POINT_GREEN_MIN);
	p[7].ui = (unsigned int)(v[2] * (ZB_POINT_BLUE_MAX - ZB_POINT_BLUE_MIN) + ZB_POINT_BLUE_MIN);
	TinyGL::gl_add_op(p);
}

void tglColor3f(float x, float y, float z) {
	tglColor4f(x, y, z, 1);
}

void glColor3fv(const float *v)  {
  tglColor4f(v[0], v[1], v[2], 1);
}

void tglColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	tglColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

// TexCoord

void tglTexCoord4f(float s, float t, float r, float q) {
	TinyGL::GLParam p[5];

	p[0].op = TinyGL::OP_TexCoord;
	p[1].f = s;
	p[2].f = t;
	p[3].f = r;
	p[4].f = q;

	TinyGL::gl_add_op(p);
}

void tglTexCoord2f(float s, float t) {
	tglTexCoord4f(s, t, 0, 1);
}

void tglTexCoord2fv(const float *v) {
	tglTexCoord4f(v[0], v[1], 0, 1);
}

void tglEdgeFlag(int flag) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_EdgeFlag;
	p[1].i = flag;

	gl_add_op(p);
}

// misc

void tglShadeModel(int mode) {
	TinyGL::GLParam p[2];

	assert(mode == TGL_FLAT || mode == TGL_SMOOTH);

	p[0].op = TinyGL::OP_ShadeModel;
	p[1].i = mode;

	TinyGL::gl_add_op(p);
}

void tglCullFace(int mode) {
	TinyGL::GLParam p[2];

	assert(mode == TGL_BACK || mode == TGL_FRONT || mode == TGL_FRONT_AND_BACK);

	p[0].op = TinyGL::OP_CullFace;
	p[1].i = mode;

	TinyGL::gl_add_op(p);
}

void tglFrontFace(int mode) {
	TinyGL::GLParam p[2];

	assert(mode == TGL_CCW || mode == TGL_CW);

	mode = (mode != TGL_CCW);

	p[0].op = TinyGL::OP_FrontFace;
	p[1].i = mode;

	TinyGL::gl_add_op(p);
}

void tglPolygonMode(int face, int mode) {
	TinyGL::GLParam p[3];

	assert(face == TGL_BACK || face == TGL_FRONT || face == TGL_FRONT_AND_BACK);
	assert(mode == TGL_POINT || mode == TGL_LINE || mode == TGL_FILL);

	p[0].op = TinyGL::OP_PolygonMode;
	p[1].i = face;
	p[2].i = mode;

	TinyGL::gl_add_op(p);
}


// glEnable, glDisable

void tglEnable(int cap) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_EnableDisable;
	p[1].i = cap;
	p[2].i = 1;

	TinyGL::gl_add_op(p);
}

void tglDisable(int cap) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_EnableDisable;
	p[1].i = cap;
	p[2].i = 0;

	TinyGL::gl_add_op(p);
}

bool tglIsEnabled(int cap) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_IsEnabled;
	p[1].i = cap;
	p[2].i = 0;

	TinyGL::gl_add_op(p);

	return p[2].i;
}

// glBegin, glEnd

void tglBegin(int mode) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_Begin;
	p[1].i = mode;

	TinyGL::gl_add_op(p);
}

void tglEnd() {
	TinyGL::GLParam p[1];

	p[0].op = TinyGL::OP_End;

	TinyGL::gl_add_op(p);
}

// matrix

void tglMatrixMode(int mode) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_MatrixMode;
	p[1].i = mode;

	TinyGL::gl_add_op(p);
}

void tglLoadMatrixf(const float *m) {
	TinyGL::GLParam p[17];
	int i;

	p[0].op = TinyGL::OP_LoadMatrix;
	for (i = 0; i < 16; i++)
		p[i + 1].f = m[i];

	TinyGL::gl_add_op(p);
}

void tglLoadIdentity() {
	TinyGL::GLParam p[1];

	p[0].op = TinyGL::OP_LoadIdentity;

	TinyGL::gl_add_op(p);
}

void tglMultMatrixf(const float *m) {
	TinyGL::GLParam p[17];
	int i;

	p[0].op = TinyGL::OP_MultMatrix;
	for (i = 0; i < 16; i++)
		p[i + 1].f = m[i];

	TinyGL::gl_add_op(p);
}

void tglPushMatrix() {
	TinyGL::GLParam p[1];

	p[0].op = TinyGL::OP_PushMatrix;

	TinyGL::gl_add_op(p);
}

void tglPopMatrix() {
	TinyGL::GLParam p[1];

	p[0].op = TinyGL::OP_PopMatrix;

	TinyGL::gl_add_op(p);
}

void tglRotatef(float angle, float x, float y, float z) {
	TinyGL::GLParam p[5];

	p[0].op = TinyGL::OP_Rotate;
	p[1].f = angle;
	p[2].f = x;
	p[3].f = y;
	p[4].f = z;

	TinyGL::gl_add_op(p);
}

void tglTranslatef(float x, float y, float z) {
	TinyGL::GLParam p[4];

	p[0].op = TinyGL::OP_Translate;
	p[1].f = x;
	p[2].f = y;
	p[3].f = z;

	TinyGL::gl_add_op(p);
}

void tglScalef(float x, float y, float z) {
	TinyGL::GLParam p[4];

	p[0].op = TinyGL::OP_Scale;
	p[1].f = x;
	p[2].f = y;
	p[3].f = z;

	gl_add_op(p);
}

void tglViewport(int x, int y, int width, int height) {
	TinyGL::GLParam p[5];

	p[0].op = TinyGL::OP_Viewport;
	p[1].i = x;
	p[2].i = y;
	p[3].i = width;
	p[4].i = height;

	gl_add_op(p);
}

void tglFrustum(double left, double right, double bottom, double top, double nearv, double farv) {
	TinyGL::GLParam p[7];

	p[0].op = TinyGL::OP_Frustum;
	p[1].f = (float)left;
	p[2].f = (float)right;
	p[3].f = (float)bottom;
	p[4].f = (float)top;
	p[5].f = (float)nearv;
	p[6].f = (float)farv;

	TinyGL::gl_add_op(p);
}

// lightening

void tglMaterialfv(int mode, int type, float *v) {
	TinyGL::GLParam p[7];
	int i, n;

	assert(mode == TGL_FRONT  || mode == TGL_BACK || mode==TGL_FRONT_AND_BACK);

	p[0].op = TinyGL::OP_Material;
	p[1].i = mode;
	p[2].i = type;
	n = 4;
	if (type == TGL_SHININESS)
		n = 1;
	for (i = 0; i < 4; i++)
		p[3 + i].f = v[i];
	for (i = n; i < 4; i++)
		p[3 + i].f = 0;

	TinyGL::gl_add_op(p);
}

void tglMaterialf(int mode, int type, float v) {
	TinyGL::GLParam p[7];
	int i;

	p[0].op = TinyGL::OP_Material;
	p[1].i = mode;
	p[2].i = type;
	p[3].f = v;
	for (i = 0; i < 3; i++)
		p[4 + i].f = 0;

	TinyGL::gl_add_op(p);
}

void tglColorMaterial(int mode, int type) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_ColorMaterial;
	p[1].i = mode;
	p[2].i = type;

	TinyGL::gl_add_op(p);
}

void tglLightfv(int light, int type, float *v) {
	TinyGL::GLParam p[7];
	int i;

	p[0].op = TinyGL::OP_Light;
	p[1].i = light;
	p[2].i = type;
	// TODO: 3 composants
	for (i = 0; i < 4; i++)
		p[3 + i].f = v[i];

	TinyGL::gl_add_op(p);
}


void tglLightf(int light, int type, float v) {
	TinyGL::GLParam p[7];
	int i;

	p[0].op = TinyGL::OP_Light;
	p[1].i = light;
	p[2].i = type;
	p[3].f = v;
	for (i = 0; i < 3; i++)
		p[4 + i].f = 0;

	TinyGL::gl_add_op(p);
}

void tglLightModeli(int pname, int param) {
	TinyGL::GLParam p[6];
	int i;

	p[0].op = TinyGL::OP_LightModel;
	p[1].i = pname;
	p[2].f = (float)param;
	for (i = 0; i < 3; i++)
		p[3 + i].f = 0;

	TinyGL::gl_add_op(p);
}

void tglLightModelfv(int pname, float *param) {
	TinyGL::GLParam p[6];
	int i;

	p[0].op = TinyGL::OP_LightModel;
	p[1].i = pname;
	for (i = 0; i < 4; i++)
		p[2 + i].f = param[i];

	TinyGL::gl_add_op(p);
}

// clear

void tglClear(int mask) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_Clear;
	p[1].i = mask;

	TinyGL::gl_add_op(p);
}

void tglClearColor(float r, float g, float b, float a) {
	TinyGL::GLParam p[5];

	p[0].op = TinyGL::OP_ClearColor;
	p[1].f = r;
	p[2].f = g;
	p[3].f = b;
	p[4].f = a;

	gl_add_op(p);
}

void tglClearDepth(double depth) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_ClearDepth;
	p[1].f = (float)depth;

	TinyGL::gl_add_op(p);
}

// textures

void tglTexImage2D( int target, int level, int components,
                   int width, int height, int border,
                   int format, int type, void *pixels) {
	TinyGL::GLParam p[10];

	p[0].op = TinyGL::OP_TexImage2D;
	p[1].i = target;
	p[2].i = level;
	p[3].i = components;
	p[4].i = width;
	p[5].i = height;
	p[6].i = border;
	p[7].i = format;
	p[8].i = type;
	p[9].p = pixels;

	TinyGL::gl_add_op(p);
}

void tglBindTexture(int target, int texture) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_BindTexture;
	p[1].i = target;
	p[2].i = texture;

	TinyGL::gl_add_op(p);
}

void tglTexEnvi(int target, int pname, int param) {
	TinyGL::GLParam p[8];

	p[0].op = TinyGL::OP_TexEnv;
	p[1].i = target;
	p[2].i = pname;
	p[3].i = param;
	p[4].f = 0;
	p[5].f = 0;
	p[6].f = 0;
	p[7].f = 0;

	gl_add_op(p);
}

void tglTexParameteri(int target, int pname, int param) {
	TinyGL::GLParam p[8];

	p[0].op = TinyGL::OP_TexParameter;
	p[1].i = target;
	p[2].i = pname;
	p[3].i = param;
	p[4].f = 0;
	p[5].f = 0;
	p[6].f = 0;
	p[7].f = 0;

	TinyGL::gl_add_op(p);
}

void tglPixelStorei(int pname, int param) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_PixelStore;
	p[1].i = pname;
	p[2].i = param;

	TinyGL::gl_add_op(p);
}

// selection

void tglInitNames() {
	TinyGL::GLParam p[1];

	p[0].op = TinyGL::OP_InitNames;

	TinyGL::gl_add_op(p);
}

void tglPushName(unsigned int name) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_PushName;
	p[1].i = name;

	TinyGL::gl_add_op(p);
}

void tglPopName() {
	TinyGL::GLParam p[1];

	p[0].op = TinyGL::OP_PopName;

	TinyGL::gl_add_op(p);
}

void tglLoadName(unsigned int name) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_LoadName;
	p[1].i = name;

	gl_add_op(p);
}

void tglPolygonOffset(TGLfloat factor, TGLfloat units) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_PolygonOffset;
	p[1].f = factor;
	p[2].f = units;

	TinyGL::gl_add_op(p);
}

// Special Functions

void tglCallList(unsigned int list) {
	TinyGL::GLParam p[2];

	p[0].op = TinyGL::OP_CallList;
	p[1].i = list;

	TinyGL::gl_add_op(p);
}

void tglFlush() {
	// nothing to do
}

void tglHint(int target, int mode) {
	TinyGL::GLParam p[3];

	p[0].op = TinyGL::OP_Hint;
	p[1].i = target;
	p[2].i = mode;

	TinyGL::gl_add_op(p);
}

// Non standard functions

void tglDebug(int mode) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	c->print_flag = mode;
}

void tglSetShadowMaskBuf(unsigned char *buf) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	c->zb->shadow_mask_buf = buf;
}

void tglSetShadowColor(unsigned char r, unsigned char g, unsigned char b) {
	TinyGL::GLContext *c = TinyGL::gl_get_context();
	c->zb->shadow_color_r = r << 8;
	c->zb->shadow_color_g = g << 8;
	c->zb->shadow_color_b = b << 8;
}

void tglOrtho(float left, float right,float bottom, float top,float near, float far) {
	//See www.opengl.org/sdk/docs/man/xhtml/glOrtho.xml for documentation

	float a = 2.0f / (right - left);
	float b = 2.0f / (top - bottom);
	float c = -2.0f / (far - near);

	float tx = - (right + left) / (right - left);
	float ty = - (top + bottom) / (top - bottom);
	float tz = - (far + near) / (far - near);

	float ortho[16] = {
		a, 0, 0, 0,
		0, b, 0, 0,
		0, 0, c, 0,
		tx, ty, tz, 1
	};

	tglMultMatrixf(ortho);
}
