
#include "graphics/agl/modelview.h"
#include "graphics/agl/manager.h"
#include "graphics/agl/renderer.h"

namespace AGL {

void ModelView::pushMatrix() {
	AGLMan._renderer->pushMatrix();
}

void ModelView::popMatrix() {
	AGLMan._renderer->popMatrix();
}

void ModelView::translate(float x, float y, float z) {
	AGLMan._renderer->translate(x, y, z);
}

void ModelView::rotate(const Math::Angle &angle, float x, float y, float z) {
	AGLMan._renderer->rotate(angle.getDegrees(), x, y, z);
}

void ModelView::scale(float x, float y, float z) {
	AGLMan._renderer->scale(x, y, z);
}

}