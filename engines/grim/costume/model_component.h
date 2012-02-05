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

#ifndef GRIM_MODEL_COMPONENT_H
#define GRIM_MODEL_COMPONENT_H

#include "engines/grim/costume/component.h"

namespace Grim {

class Model;
class AnimManager;

class ModelComponent : public Component {
public:
	ModelComponent(Component *parent, int parentID, const char *filename, Component *prevComponent, tag32 tag);
	~ModelComponent();

	void init();
	void setKey(int val);
	void animate();
	void reset();
	void resetColormap();
	void setMatrix(const Math::Matrix4 &matrix) { _matrix = matrix; };
	void restoreState(SaveGame *state);
	void translateObject(bool reset);
	static void translateObject(ModelNode *node, bool reset);
	AnimManager *getAnimManager() const;

	ModelNode *getHierarchy() { return _hier; }
	int getNumNodes();
	Model *getModel() { return _obj; }
	void draw();
	void getBoundingBox(int *x1, int *y1, int *x2, int *y2);

protected:
	Common::String _filename;
	Model *_obj;
	ModelNode *_hier;
	Math::Matrix4 _matrix;
	AnimManager *_animation;
	Component *_prevComp;
};

} // end of namespace Grim

#endif
