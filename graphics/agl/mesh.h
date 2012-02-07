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

#ifndef AGL_MESH_H
#define AGL_MESH_H

#include "common/list.h"

#include "math/vector2d.h"

#include "graphics/color.h"

#include "graphics/agl/manager.h"

namespace Common {
struct Rect;
}

namespace AGL {

class MeshFace;
class Texture;

class Mesh {
public:
	typedef Common::List<MeshFace *> Faces;

	virtual ~Mesh() { }

	virtual MeshFace *createFace() = 0;

	void pushVertex(float x, float y, float z);
	inline void pushVertex(const Math::Vector3d &v) { pushVertex(v.x(), v.y(), v.z()); }

	void pushTexVertex(float u, float v);
	inline void pushTexVertex(const Math::Vector2d &v) { pushTexVertex(v.getX(), v.getY()); }

	void pushNormal(float x, float y, float z);
	inline void pushNormal(const Math::Vector3d &v) { pushNormal(v.x(), v.y(), v.z()); }

	void pushColor(float r, float g, float b, float a);
	inline void pushColor(const Graphics::Color &c) { pushColor(c.getRed(), c.getGreen(), c.getBlue(), c.getAlpha()); }

	virtual void draw(Texture *texture) = 0;

	void setUseAbsoluteTexCoords(bool useAbs);
	inline bool getUseAbsoluteTexCoords() const { return _useAbsTexCoords; }

	virtual bool calculate2DBoundingBox(Common::Rect *rect) const = 0;

	void setDrawMode(DrawMode mode);
	inline DrawMode getDrawMode() const { return _drawMode; }

// 	Faces::iterator facesBegin() const { return _faces.begin(); }
// 	Faces::iterator facesEnd() const { return _faces.end(); }

	inline const float *getVerticesArray() const { return _vertices.begin(); }
	inline const float *getTexCoordsArray() const { return _textures.begin(); }
	inline const float *getNormalsArray() const { return _normals.begin(); }
	inline const float *getColorsArray() const { return _colors.begin(); }

	float *getVerticesArray();
	float *getTexCoordsArray();
	float *getNormalsArray();
	float *getColorsArray();

	void setVerticesArray(float *array, int n);

protected:
	Mesh();

	Faces _faces;

private:
	Common::Array<float> _vertices;
	Common::Array<float> _textures;
	Common::Array<float> _normals;
	Common::Array<float> _colors;

	bool _useAbsTexCoords;
	DrawMode _drawMode;
};

}

#endif
