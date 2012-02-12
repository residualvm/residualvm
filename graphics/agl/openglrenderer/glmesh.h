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

#ifndef AGL_GLMESH_H
#define AGL_GLMESH_H

#include "common/array.h"

#include "math/vector3d.h"

#include "graphics/agl/mesh.h"
#include "graphics/agl/meshface.h"

namespace AGL {

class MeshFace;
class Texture;

class GLMesh : public Mesh {
public:
	MeshFace *createFace();

	void draw(Texture *texture);

	bool calculate2DBoundingBox(Common::Rect *rect) const;
};

class GLMeshFace : public MeshFace {
public:
	GLMeshFace(GLMesh *parent);

	void setNormal(float x, float y, float z);

	void vertex(int index);
	void texture(int index);
	void normal(int index);
	void color(int index);

	void draw(Texture *texture);

	GLMesh *_parent;
	Math::Vector3d _normal;
	Common::Array<int> _vertices;
	Common::Array<int> _textures;
	Common::Array<int> _normals;
	Common::Array<int> _colors;

	bool _useColors;
	bool _useTexture;
};

}

#endif
