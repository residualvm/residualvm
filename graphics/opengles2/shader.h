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

#include "common/file.h"
#include "common/array.h"

#include "math/matrix3.h"
#include "math/matrix4.h"
#include "math/vector2d.h"
#include "math/vector3d.h"
#include "math/vector4d.h"

#if defined(USE_GLES2)
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#undef GL_GLEXT_PROTOTYPES

#define glMapBuffer glMapBufferOES
#define glUnmapBuffer glUnmapBufferOES
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES

#define GL_BGRA GL_BGRA_EXT
#else
#include <GL/glew.h>
#endif

namespace Graphics {

struct VertexAttrib {
	VertexAttrib(uint32_t idx, const char *name) : _enabled(false), _idx(idx), _name(name), _vbo(0), _size(0), _type(GL_FLOAT), _normalized(false), _stride(0), _offset(NULL) {}
	bool _enabled;
	uint32_t _idx;
	Common::String _name;
	GLuint _vbo;
	GLint _size;
	GLenum _type;
	bool _normalized;
	GLsizei _stride;
	const GLvoid *_offset;
	float _const[4];
};

class Shader {
public:
	Shader(const char* vertex, const char* fragment, const char** attributes);

	Shader* clone() {
		return new Shader(*this);
	}

	void use();

	void setUniform(const char *uniform, const Math::Matrix4 &m) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniformMatrix4fv(pos, 1, GL_FALSE, m.getData());
	}

	void setUniform(const char* uniform, const Math::Matrix3 &m) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniformMatrix3fv(pos, 1, GL_FALSE, m.getData());
	}

	void setUniform(const char *uniform, const Math::Vector4d &v) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniform4fv(pos, 1, v.getData());
	}

	void setUniform(const char *uniform, const Math::Vector3d &v) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniform3fv(pos, 1, v.getData());
	}

	void setUniform(const char *uniform, const Math::Vector2d &v) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniform2fv(pos, 1, v.getData());
	}

	void setUniform(const char *uniform, unsigned int x) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniform1i(pos, x);
	}

	GLint getUniformLocation(const char *uniform) {
		return glGetUniformLocation(_shaderNo, uniform);
	}

	void enableVertexAttribute(const char *attrib, GLuint vbo, GLint size, GLenum type, GLboolean normalized, GLsizei stride, uint32_t offset);
	void disableVertexAttribute(const char *attrib, int size, const float *data);
	template <int r>
	void disableVertexAttribute(const char *attrib, const Math::Matrix<r,1> &m) {
		disableVertexAttribute(attrib, r, m.getData());
	}
	VertexAttrib & getAttributeAt(uint32_t idx);
	VertexAttrib & getAttribute(const char *attrib);

	static GLuint createBuffer(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage = GL_STATIC_DRAW);

	static Shader* createShader(const char* shared, const char** attributes) {
		return new Shader(shared, shared, attributes);
	}


private:
	GLuint _shaderNo;
	Common::String _name;


	Common::Array<VertexAttrib> _attributes;
};

}
