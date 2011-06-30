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

/** @file common/huffman.cpp
 *  Decompressing Huffman codes.
 */


#include "huffman.h"
#include "bitstream.h"
#include "common/textconsole.h"

namespace Common {

Huffman::Symbol::Symbol(uint32 c, uint32 s) : code(c), symbol(s) {
}


Huffman::Huffman(uint8 maxLength, uint32 codeCount, const uint32 *codes, const uint8 *lengths, const uint32 *symbols) {
	assert(maxLength > 0);
	assert(codeCount > 0);

	assert(codes);
	assert(lengths);

	_codes.resize(maxLength);
	_symbols.resize(codeCount);

	for (uint32 i = 0; i < codeCount; i++) {
		// The symbol. If none were specified, just assume it's identical to the code index
		uint32 symbol = symbols ? symbols[i] : i;

		// Put the code and symbol into the correct list
		_codes[lengths[i] - 1].push_back(Symbol(codes[i], symbol));

		// And put the pointer to the symbol/code struct into the symbol list.
		_symbols[i] = &_codes[lengths[i] - 1].back();
	}
}

Huffman::~Huffman() {
}

void Huffman::setSymbols(const uint32 *symbols) {
	for (uint32 i = 0; i < _symbols.size(); i++)
		_symbols[i]->symbol = symbols ? *symbols++ : i;
}

uint32 Huffman::getSymbol(BitStream &bits) {
	uint32 code = 0;

	for (uint32 i = 0; i < _codes.size(); i++) {
		bits.addBit(code, i);

		for (CodeList::const_iterator cCode = _codes[i].begin(); cCode != _codes[i].end(); ++cCode)
			if (code == cCode->code)
				return cCode->symbol;
	}

	error("Unknown Huffman code");
}

} // End of namespace Common
