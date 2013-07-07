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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/file.h"
#include "common/array.h"

#include "math/matrix3.h"
#include "math/matrix4.h"
#include "math/vector2d.h"
#include "math/vector3d.h"
#include "math/vector4d.h"

#include "graphics/opengles2/system_headers.h"

namespace Graphics {

namespace BuiltinShaders {
	extern const char *boxVertex, *boxFragment;
	extern const char *compatVertex, *compatFragment;
	extern const char *controlVertex, *controlFragment;
}

struct VertexAttrib {
	VertexAttrib(uint32 idx, const char *name) : _enabled(false), _idx(idx), _name(name), _vbo(0), _size(0), _type(GL_FLOAT), _normalized(false), _stride(0), _offset(NULL) {}
	bool _enabled;
	uint32 _idx;
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
	typedef Common::HashMap<Common::String, GLint> UniformsMap;

public:
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

	// Different name to avoid overload ambiguity
	void setUniform1f(const char *uniform, float f) {
		GLint pos = getUniformLocation(uniform);
		if (pos != -1)
			glUniform1f(pos, f);
	}


	GLint getUniformLocation(const char *uniform) const {
		UniformsMap::iterator kv = _uniforms->find(uniform);
		if (kv == _uniforms->end()) {
			GLint ret = glGetUniformLocation(_shaderNo, uniform);
			_uniforms->setVal(uniform, ret);
			return ret;
		} else {
			return kv->_value;
		}
	}

	void enableVertexAttribute(const char *attrib, GLuint vbo, GLint size, GLenum type, GLboolean normalized, GLsizei stride, uint32 offset);
	void disableVertexAttribute(const char *attrib, int size, const float *data);
	template <int r>
	void disableVertexAttribute(const char *attrib, const Math::Matrix<r,1> &m) {
		disableVertexAttribute(attrib, r, m.getData());
	}
	VertexAttrib & getAttributeAt(uint32 idx);
	VertexAttrib & getAttribute(const char *attrib);

	static GLuint createBuffer(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage = GL_STATIC_DRAW);

	static Shader* fromFiles(const char *vertex, const char *fragment, const char **attributes);
	static Shader* fromFiles(const char *shared, const char **attributes) {
		return fromFiles(shared, shared, attributes);
	}

	static Shader* fromStrings(const Common::String &name, const char *vertex, const char *fragment, const char **attributes);

private:
	Shader(const Common::String &name, GLuint vertexShader, GLuint fragmentShader, const char **attributes);
	GLuint _shaderNo;
	Common::String _name;

	Common::Array<VertexAttrib> _attributes;
	UniformsMap *_uniforms;
};

}
