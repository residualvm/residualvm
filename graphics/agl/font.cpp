
#include "graphics/agl/font.h"
#include "graphics/agl/texture.h"

namespace AGL {

FontMetric::~FontMetric() {

}

int FontMetric::getStringLength(const Common::String &text) const {
	int result = 0;
	for (uint32 i = 0; i < text.size(); ++i) {
		result += getCharWidth(text[i]);
	}
	return result;
}

Font::Font(FontMetric *metric)
	: _metric(metric) {
}

}
