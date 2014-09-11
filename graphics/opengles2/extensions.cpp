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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "common/algorithm.h"
#include "common/list.h"
#include "common/str.h"
#include "common/tokenizer.h"

#include "graphics/opengles2/system_headers.h"

#ifdef USE_OPENGL

namespace Graphics {

static Common::List<Common::String> g_extensions;

void initExtensions() {
	const char *exts = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
	Common::StringTokenizer tokenizer(exts, " ");
	while (!tokenizer.empty()) {
		g_extensions.push_back(tokenizer.nextToken());
	}
}

bool isExtensionSupported(const char *wanted) {
	return g_extensions.end() != find(g_extensions.begin(), g_extensions.end(), wanted);
}

}

#endif
