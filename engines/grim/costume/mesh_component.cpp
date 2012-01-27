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

#include "engines/grim/debug.h"
#include "engines/grim/model.h"
#include "engines/grim/savegame.h"
#include "engines/grim/costume/mesh_component.h"
#include "engines/grim/costume/model_component.h"

namespace Grim {

MeshComponent::MeshComponent(Component *p, int parentID, const char *name, tag32 t) :
		Component(p, parentID, t), _name(name), _node(NULL) {
	if (sscanf(name, "mesh %d", &_num) < 1)
		error("Couldn't parse mesh name %s", name);

}

void MeshComponent::init() {
	ModelComponent *mc = dynamic_cast<ModelComponent *>(_parent);
	if (mc) {
		_node = mc->getHierarchy() + _num;
		_model = mc->getModel();
	} else {
		Debug::warning(Debug::Costumes, "Parent of mesh %d was not a model", _num);
		_node = NULL;
		_model = NULL;
	}
}

CMap *MeshComponent::cmap() {
	ModelComponent *mc = dynamic_cast<ModelComponent *>(_parent);
	if (!mc)
		return NULL;
	return mc->getCMap();
}

void MeshComponent::setKey(int val) {
	_node->_meshVisible = (val != 0);
}

void MeshComponent::reset() {
	// NOTE: Setting the visibility to true here causes a bug with the thunderboy costume:
	// closing the inventory causes the hat to appear, while it shouldn't.
	// This could however introduce regressions somewhere else, so if there is something
	// disappearing or not behaving properly in a costume the cause might be here.
// 	_node->_meshVisible = true;
}

int MeshComponent::update(uint /*time*/) {
	_node->setMatrix(_matrix);
	return 0;
}

void MeshComponent::saveState(SaveGame *state) {
	state->writeBool(_node->_meshVisible);
}

void MeshComponent::restoreState(SaveGame *state) {
	_node->_meshVisible = state->readBool();
}

} // end of namespace Grim
