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

#include "graphics/opengles2/compat_shaders.h"
#include "graphics/opengles2/shader.h"

namespace Graphics {

static const GLchar *readFile(const Common::String &filename) {
	Common::File file;
	file.open(Common::String("shaders/") + filename);
	if (!file.isOpen())
		error("Could not open shader %s!", filename.c_str());

	const int32 size = file.size();
	GLchar *shaderSource = new GLchar[size + 1];

	file.read(shaderSource, size);
	file.close();
	shaderSource[size] = '\0';
	return shaderSource;
}

static GLuint createDirectShader(const char *shaderSource, GLenum shaderType, const Common::String &name) {
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &shaderSource, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(shader, 512, NULL, buffer);
		error("Could not compile shader %s: %s", name.c_str(), buffer);
	}

	return shader;
}

static GLuint createCompatShader(const char *shaderSource, GLenum shaderType, const Common::String &name) {
	const GLchar *compatSource =
			shaderType == GL_VERTEX_SHADER ? Graphics::BuiltinShaders::compatVertex : Graphics::BuiltinShaders::compatFragment;
	const GLchar *shaderSources[] = {
		"#version 100\n",
		compatSource,
		shaderSource
	};

	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 3, shaderSources, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		char buffer[512];
		glGetShaderInfoLog(shader, 512, NULL, buffer);
		error("Could not compile shader %s: %s", name.c_str(), buffer);
	}

	return shader;
}

static GLuint loadShaderFromFile(const char *base, const char *extension, GLenum shaderType) {
	const Common::String filename = Common::String(base) + "." + extension;
	const GLchar *shaderSource = readFile(filename);

	GLuint shader = createCompatShader(shaderSource, shaderType, filename);

	delete[] shaderSource;

	return shader;
}

Shader::Shader(const Common::String &name, GLuint vertexShader, GLuint fragmentShader, const char **attributes)
	: _name(name) {
	assert(attributes);
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	for (int idx = 0; attributes[idx]; ++idx) {
		glBindAttribLocation(shaderProgram, idx, attributes[idx]);
		_attributes.push_back(VertexAttrib(idx, attributes[idx]));
	}
	glLinkProgram(shaderProgram);

	_shaderNo = shaderProgram;
	_uniforms = new Common::HashMap<Common::String, GLint>();
}

Shader *Shader::fromStrings(const Common::String &name, const char *vertex, const char *fragment, const char **attributes) {
	GLuint vertexShader = createDirectShader(vertex, GL_VERTEX_SHADER, name + ".vertex");
	GLuint fragmentShader = createDirectShader(fragment, GL_FRAGMENT_SHADER, name + ".fragment");
	return new Shader(name, vertexShader, fragmentShader, attributes);
}


Shader *Shader::fromFiles(const char *vertex, const char *fragment, const char **attributes) {
	GLuint vertexShader = loadShaderFromFile(vertex, "vertex", GL_VERTEX_SHADER);
	GLuint fragmentShader = loadShaderFromFile(fragment, "fragment", GL_FRAGMENT_SHADER);

	Common::String name = Common::String::format("%s/%s", vertex, fragment);
	return new Shader(name, vertexShader, fragmentShader, attributes);
}

void Shader::use() {
	static Shader *previousShader = NULL;
	if (this == previousShader)
		return;
	previousShader = this;

	glUseProgram(_shaderNo);
	for (uint32 i = 0; i < _attributes.size(); ++i) {
		Graphics::VertexAttrib &attrib = _attributes[i];
		if (attrib._enabled) {
			glEnableVertexAttribArray(i);
			glBindBuffer(GL_ARRAY_BUFFER, attrib._vbo);
			glVertexAttribPointer(i, attrib._size, attrib._type, attrib._normalized, attrib._stride, attrib._offset);
		} else {
			glDisableVertexAttribArray(i);
			switch (attrib._size) {
			case 2:
				glVertexAttrib2fv(i, attrib._const);
				break;
			case 3:
				glVertexAttrib3fv(i, attrib._const);
				break;
			case 4:
				glVertexAttrib4fv(i, attrib._const);
				break;
			}
		}
	}
}

GLuint Shader::createBuffer(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(target, vbo);
	glBufferData(target, size, data, usage);
	return vbo;
}

VertexAttrib &Shader::getAttributeAt(uint32 idx) {
	assert(idx < _attributes.size());
	return _attributes[idx];
}

VertexAttrib &Shader::getAttribute(const char *attrib) {
	for (uint32 i = 0; i < _attributes.size(); ++i)
		if (_attributes[i]._name.equals(attrib))
			return _attributes[i];
	error("Could not find attribute %s in shader %s", attrib, _name.c_str());
	return _attributes[0];
}

void Shader::enableVertexAttribute(const char *attrib, GLuint vbo, GLint size, GLenum type, GLboolean normalized, GLsizei stride, uint32 offset) {
	VertexAttrib &va = getAttribute(attrib);
	va._enabled = true;
	va._vbo = vbo;
	va._size = size;
	va._type = type;
	va._normalized = normalized;
	va._stride = stride;
	va._offset = (const GLvoid *) offset;
}

void Shader::disableVertexAttribute(const char *attrib, int size, const float *data) {
	VertexAttrib &va = getAttribute(attrib);
	va._enabled = false;
	va._size = size;
	for (int i = 0; i < size; ++i)
		va._const[i] = data[i];
}

}
