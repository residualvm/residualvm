/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
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

#include "common/util.h"
#include "common/textconsole.h"

#include "engines/grim/textsplit.h"

namespace Grim {

// FIXME: Replace this with a proper parser (this is just too dodgy :)
int residual_vsscanf(const char *str, int field_count, const char *format, va_list ap) {
	unsigned int f01 = va_arg(ap, long);
	unsigned int f02 = va_arg(ap, long);
	unsigned int f03 = va_arg(ap, long);
	unsigned int f04 = va_arg(ap, long);
	unsigned int f05 = va_arg(ap, long);
	unsigned int f06 = va_arg(ap, long);
	unsigned int f07 = va_arg(ap, long);
	unsigned int f08 = va_arg(ap, long);
	unsigned int f09 = va_arg(ap, long);
	unsigned int f10 = va_arg(ap, long);
	unsigned int f11 = va_arg(ap, long);
	unsigned int f12 = va_arg(ap, long);
	unsigned int f13 = va_arg(ap, long);
	unsigned int f14 = va_arg(ap, long);
	unsigned int f15 = va_arg(ap, long);
	unsigned int f16 = va_arg(ap, long);
	unsigned int f17 = va_arg(ap, long);
	unsigned int f18 = va_arg(ap, long);
	unsigned int f19 = va_arg(ap, long);
	unsigned int f20 = va_arg(ap, long);

	if (field_count > 20)
		error("Too many fields requested of residual_vsscanf (%d)", field_count);

	return sscanf(str, format, f01, f02, f03, f04, f05, f06, f07, f08, f09, f10,
			f11, f12, f13, f14, f15, f16, f17, f18, f19, f20);
}

TextSplitter::TextSplitter(const char *data, int len) {
	char *line;
	int i;

	_stringData = new char[len + 1];
	memcpy(_stringData, data, len);
	_stringData[len] = '\0';
	// Find out how many lines of text there are
	_numLines = _lineIndex = 0;
	line = (char *)_stringData;
	while (line) {
		line = strchr(line, '\n');
		if (line) {
			_numLines++;
			line++;
		}
	}
	// Allocate an array of the lines
	_lines = new char *[_numLines];
	line = (char *)_stringData;
	for (i = 0; i < _numLines;i++) {
		char *lastLine = line;
		line = strchr(lastLine, '\n');
		*line = '\0';
		_lines[i] = lastLine;
		line++;
	}
	_currLine = NULL;
	processLine();
}

TextSplitter::~TextSplitter() {
	delete[] _stringData;
	delete[] _lines;
}

bool TextSplitter::checkString(const char *needle) {
	// checkString also needs to check for extremely optional
	// components like "object_art" which can be missing entirely
	if (!getCurrentLine())
		return false;
	else if (strstr(getCurrentLine(), needle))
		return true;
	else
		return false;
}

void TextSplitter::expectString(const char *expected) {
	if (!_currLine)
		error("Expected `%s', got EOF", expected);
	if (scumm_stricmp(getCurrentLine(), expected) != 0)
		error("Expected `%s', got '%s'", expected, getCurrentLine());
	nextLine();
}

void TextSplitter::scanString(const char *fmt, int field_count, ...) {
	if (!_currLine)
		error("Expected line of format '%s', got EOF", fmt);

	va_list va;

	va_start(va, field_count);

#ifdef WIN32
	if (residual_vsscanf(getCurrentLine(), field_count, fmt, va) < field_count)
#else
	if (vsscanf(getCurrentLine(), fmt, va) < field_count)
#endif
		error("Expected line of format '%s', got '%s'", fmt, getCurrentLine());
	va_end(va);

	nextLine();
}

void TextSplitter::processLine() {
	if (isEof())
		return;

	_currLine = _lines[_lineIndex++];

	// Cut off comments
	char *comment_start = strchr(_currLine, '#');
	if (comment_start)
		*comment_start = '\0';

	// Cut off trailing whitespace (including '\r')
	char *strend = strchr(_currLine, '\0');
	while (strend > _currLine && isspace(strend[-1]))
		strend--;
	*strend = '\0';

	// Skip blank lines
	if (*_currLine == '\0')
		nextLine();

	// Convert to lower case
	if (!isEof())
		for (char *s = _currLine; *s != '\0'; s++)
			*s = tolower(*s);
}

} // end of namespace Grim
