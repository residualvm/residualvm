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
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#ifndef LANGFILTER_H
#define LANGFILTER_H

#include "common/archive.h"
#include "common/util.h"

namespace Grim {

class LangFilter : public Common::Archive {
public:
	LangFilter(Common::Archive *arc, Common::Language lang);
	~LangFilter();

	// Common::Archive API implementation
	bool hasFile(const Common::String &name) const;
	int listMembers(Common::ArchiveMemberList &list) const;
	const Common::ArchiveMemberPtr getMember(const Common::String &name) const;
	Common::SeekableReadStream *createReadStreamForMember(const Common::String &name) const;
private:
	Common::Archive *_arc;

	enum kLang {
		kCommon = 0,
		kEnglish,
		kFrench,
		kGerman,
		kItalian,
		kPortuguese,
		kSpanish
	};
	kLang _lang;
	static const char *kLanguages1[7];
	static const char *kLanguages2[7];
};

} // end of namespace Grim

#endif
