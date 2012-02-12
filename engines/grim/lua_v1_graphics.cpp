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

#define FORBIDDEN_SYMBOL_EXCEPTION_chdir
#define FORBIDDEN_SYMBOL_EXCEPTION_getcwd
#define FORBIDDEN_SYMBOL_EXCEPTION_getwd
#define FORBIDDEN_SYMBOL_EXCEPTION_mkdir
#define FORBIDDEN_SYMBOL_EXCEPTION_unlink

#include "graphics/surface.h"

#include "graphics/agl/manager.h"
#include "graphics/agl/target.h"

#include "engines/grim/grim.h"
#include "engines/grim/lua_v1.h"
#include "engines/grim/resource.h"
#include "engines/grim/colormap.h"
#include "engines/grim/bitmap.h"
#include "engines/grim/primitives.h"
#include "engines/grim/iris.h"

#include "engines/grim/movie/movie.h"

#include "engines/grim/lua/lua.h"

namespace Grim {

#if 0
void killBitmapPrimitives(Bitmap *bitmap) {
	for (GrimEngine::PrimitiveListType::const_iterator i = g_grim->primitivesBegin(); i != g_grim->primitivesEnd(); ++i) {
		PrimitiveObject *p = i->_value;
		if (p->isBitmap() && p->getBitmapHandle() == bitmap) {
			g_grim->killPrimitiveObject(p);
			break;
		}
	}
}
#endif

void Lua_V1::GetImage() {
	lua_Object nameObj = lua_getparam(1);
	if (!lua_isstring(nameObj)) {
		lua_pushnil();
		return;
	}
	const char *bitmapName = lua_getstring(nameObj);
	Bitmap *b = Bitmap::create(bitmapName);
	lua_pushusertag(b->getId(), MKTAG('V','B','U','F'));
}

void Lua_V1::FreeImage() {
	lua_Object param = lua_getparam(1);
	if (!lua_isuserdata(param) || lua_tag(param) != MKTAG('V','B','U','F'))
		return;
	Bitmap *bitmap = getbitmap(param);
	delete bitmap;
}

void Lua_V1::BlastImage() {
	lua_Object param = lua_getparam(1);
	if (!lua_isuserdata(param) || lua_tag(param) != MKTAG('V','B','U','F'))
		return;
	Bitmap *bitmap = getbitmap(param);
	lua_Object xObj = lua_getparam(2);
	lua_Object yObj = lua_getparam(3);
	if (!lua_isnumber(xObj) || !lua_isnumber(yObj))
		return;

	int x = (int)lua_getnumber(xObj);
	int y = (int)lua_getnumber(yObj);
//	bool transparent = getbool(4); // TODO transparent/masked copy into display
	bitmap->draw(x, y);
}

void Lua_V1::CleanBuffer() {
	AGLMan.getTarget()->restoreContent();
}

void Lua_V1::StartFullscreenMovie() {
	lua_Object name = lua_getparam(1);
	if (!lua_isstring(name)) {
		lua_pushnil();
		return;
	}
	Lua_V1::CleanBuffer();

	GrimEngine::EngineMode prevEngineMode = g_grim->getMode();
	g_grim->setMode(GrimEngine::SmushMode);
	bool looping = getbool(2);
	bool result = g_movie->play(lua_getstring(name), looping, 0, 0);
	if (!result)
		g_grim->setMode(prevEngineMode);
	pushbool(result);
}

void Lua_V1::StartMovie() {
	lua_Object name = lua_getparam(1);
	if (!lua_isstring(name)) {
		lua_pushnil();
		return;
	}
	int x = 0, y = 0;
	if (!lua_isnil(lua_getparam(3)))
		x = (int)lua_getnumber(lua_getparam(3));
	if (!lua_isnil(lua_getparam(4)))
		y = (int)lua_getnumber(lua_getparam(4));

	GrimEngine::EngineMode prevEngineMode = g_grim->getMode();
	g_grim->setMode(GrimEngine::NormalMode);

	bool looping = getbool(2);
	bool result = g_movie->play(lua_getstring(name), looping, x, y);
	if (!result)
		g_grim->setMode(prevEngineMode);
	pushbool(result);
}

/* Fullscreen movie playing query and normal movie
 * query should actually detect correctly and not
 * just return true whenever ANY movie is playing
 */
void Lua_V1::IsFullscreenMoviePlaying() {
	pushbool(g_movie->isPlaying());
}

void Lua_V1::IsMoviePlaying() {
	// Previously, if the game was *not* the demo, this checked also if the mode
	// was GrimEngine::NormalMode. This doesn't seem to be what original does, and causes
	// bug #301 because the movie eldepot.snm is played before legslide.snm ends.
	pushbool(g_movie->isPlaying());
}

void Lua_V1::StopMovie() {
	g_movie->stop();
}

void Lua_V1::PauseMovie() {
	g_movie->pause(lua_isnil(lua_getparam(1)) == 0);
}

void Lua_V1::PurgePrimitiveQueue() {
	PrimitiveObject::getPool().deleteObjects();
}

void Lua_V1::DrawPolygon() {
	lua_Object pointObj;
	Common::Point p1, p2, p3, p4;
	Color color;

	lua_Object tableObj1 = lua_getparam(1);
	if (!lua_istable(tableObj1)) {
		lua_pushnil();
		return;
	}

	int layer = 2;
	lua_Object tableObj2 = lua_getparam(2);
	if (lua_istable(tableObj2)) {
		lua_pushobject(tableObj2);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('C','O','L','R')) {
			color = getcolor(colorObj);
		}
		lua_pushobject(tableObj2);
		lua_pushstring("layer");
		lua_Object layerObj = lua_gettable();
		if (lua_isnumber(layerObj))
			layer = (int)lua_getnumber(layerObj);
	}

	// This code support static 4 points polygon as game doesn't use other than that.
	// However original engine can support many points per polygon
	lua_pushobject(tableObj1);
	lua_pushnumber(1);
	pointObj = lua_gettable();
	p1.x = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(2);
	pointObj = lua_gettable();
	p1.y = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(3);
	pointObj = lua_gettable();
	p2.x = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(4);
	pointObj = lua_gettable();
	p2.y = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(5);
	pointObj = lua_gettable();
	p3.x = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(6);
	pointObj = lua_gettable();
	p3.y = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(7);
	pointObj = lua_gettable();
	p4.x = (int)lua_getnumber(pointObj);
	lua_pushobject(tableObj1);
	lua_pushnumber(8);
	pointObj = lua_gettable();
	p4.y = (int)lua_getnumber(pointObj);

	PrimitiveObject *p = new PrimitiveObject();
	p->createPolygon(p1, p2, p3, p4, color);
	lua_pushusertag(p->getId(), MKTAG('P','R','I','M'));
}

void Lua_V1::DrawLine() {
	Common::Point p1, p2;
	Color color;
	lua_Object x1Obj = lua_getparam(1);
	lua_Object y1Obj = lua_getparam(2);
	lua_Object x2Obj = lua_getparam(3);
	lua_Object y2Obj = lua_getparam(4);
	lua_Object tableObj = lua_getparam(5);

	if (!lua_isnumber(x1Obj) || !lua_isnumber(y1Obj) || !lua_isnumber(x2Obj) || !lua_isnumber(y2Obj)) {
		lua_pushnil();
		return;
	}

	p1.x = (int)lua_getnumber(x1Obj);
	p1.y = (int)lua_getnumber(y1Obj);
	p2.x = (int)lua_getnumber(x2Obj);
	p2.y = (int)lua_getnumber(y2Obj);

	int layer = 2;
	if (lua_istable(tableObj)) {
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('C','O','L','R')) {
			color = getcolor(colorObj);
		}
		lua_pushobject(tableObj);
		lua_pushstring("layer");
		lua_Object layerObj = lua_gettable();
		if (lua_isnumber(layerObj))
			layer = (int)lua_getnumber(layerObj);
	}

	PrimitiveObject *p = new PrimitiveObject();
	p->createLine(p1, p2, color); // TODO Add layer support
	lua_pushusertag(p->getId(), MKTAG('P','R','I','M'));
}

void Lua_V1::ChangePrimitive() {
	lua_Object param1 = lua_getparam(1);
	if (!lua_isuserdata(param1) || lua_tag(param1) != MKTAG('P','R','I','M'))
		return;

	lua_Object tableObj = lua_getparam(2);
	if (!lua_istable(tableObj))
		return;

	PrimitiveObject *pmodify = getprimitive(param1);
	assert(pmodify);

	Color color;
	lua_pushobject(tableObj);
	lua_pushstring("color");
	lua_Object colorObj = lua_gettable();
	if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('C','O','L','R')) {
		color = getcolor(colorObj);
		pmodify->setColor(color);
	}

	lua_pushobject(tableObj);
	lua_pushstring("layer");
	lua_Object layer = lua_gettable();
	if (lua_isnumber(layer)) {
		// TODO pmodify->setLayer(lua_getnumber(layer));
		warning("Not implemented: PrimitiveObject::setLayer. Layer: %d", (int)lua_getnumber(layer));
	}

	lua_pushobject(tableObj);
	lua_pushstring("xoffset");
	lua_Object xoffset = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("yoffset");
	lua_Object yoffset = lua_gettable();
	if (lua_isnumber(xoffset) || lua_isnumber(yoffset)) {
		int x = 0;
		int y = 0;
		if (lua_isnumber(xoffset))
			x = (int)lua_getnumber(xoffset);
		if (lua_isnumber(yoffset))
			y = (int)lua_getnumber(yoffset);
		// TODO pmodify->setOffets(x, y);
		assert(0);
	}

	lua_pushobject(tableObj);
	lua_pushstring("x");
	lua_Object xobj = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("y");
	lua_Object yobj = lua_gettable();
	if (lua_isnumber(xobj) || lua_isnumber(yobj)) {
		int x = -1;
		int y = -1;
		if (lua_isnumber(xobj))
			x = (int)lua_getnumber(xobj);
		if (lua_isnumber(yobj))
			y = (int)lua_getnumber(yobj);
		pmodify->setPos(x, y);
	}

	lua_pushobject(tableObj);
	lua_pushstring("x2");
	lua_Object x2 = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("y2");
	lua_Object y2 = lua_gettable();
	if (lua_isnumber(x2) || lua_isnumber(y2)) {
		int x = -1;
		int y = -1;
		if (lua_isnumber(x2))
			x = (int)lua_getnumber(x2);
		if (lua_isnumber(y2))
			y = (int)lua_getnumber(y2);
		// TODO pmodify->setSize(x, y);
		assert(0);
	}

	lua_pushobject(tableObj);
	lua_pushstring("width");
	lua_Object width = lua_gettable();
	lua_pushobject(tableObj);
	lua_pushstring("height");
	lua_Object height = lua_gettable();
	if (lua_isnumber(width) || lua_isnumber(height)) {
		int x = -1;
		int y = -1;
		if (lua_isnumber(width))
			x = (int)lua_getnumber(width);
		if (lua_isnumber(height))
			y = (int)lua_getnumber(height);
		// TODO pmodify->setSize(x, y);
		assert(0);
	}
}

void Lua_V1::DrawRectangle() {
	Common::Point p1, p2;
	Color color;
	lua_Object objX1 = lua_getparam(1);
	lua_Object objY1 = lua_getparam(2);
	lua_Object objX2 = lua_getparam(3);
	lua_Object objY2 = lua_getparam(4);
	lua_Object tableObj = lua_getparam(5);

	if (!lua_isnumber(objX1) || !lua_isnumber(objY1) || !lua_isnumber(objX2) || !lua_isnumber(objY2)) {
		lua_pushnil();
		return;
	}
	p1.x = (int)lua_getnumber(objX1);
	p1.y = (int)lua_getnumber(objY1);
	p2.x = (int)lua_getnumber(objX2);
	p2.y = (int)lua_getnumber(objY2);
	bool filled = false;

	if (lua_istable(tableObj)){
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('C','O','L','R')) {
			color = getcolor(colorObj);
		}

		lua_pushobject(tableObj);
		lua_pushstring("filled");
		lua_Object objFilled = lua_gettable();
		if (!lua_isnil(objFilled))
			filled = true;
	}

	PrimitiveObject *p = new PrimitiveObject();
	p->createRectangle(p1, p2, color, filled);
	lua_pushusertag(p->getId(), MKTAG('P','R','I','M')); // FIXME: we use PRIM usetag here
}

void Lua_V1::BlastRect() {
	Common::Point p1, p2;
	Color color;
	lua_Object objX1 = lua_getparam(1);
	lua_Object objY1 = lua_getparam(2);
	lua_Object objX2 = lua_getparam(3);
	lua_Object objY2 = lua_getparam(4);
	lua_Object tableObj = lua_getparam(5);

	if (!lua_isnumber(objX1) || !lua_isnumber(objY1) || !lua_isnumber(objX2) || !lua_isnumber(objY2)) {
		lua_pushnil();
		return;
	}
	p1.x = (int)lua_getnumber(objX1);
	p1.y = (int)lua_getnumber(objY1);
	p2.x = (int)lua_getnumber(objX2);
	p2.y = (int)lua_getnumber(objY2);
	bool filled = false;

	if (lua_istable(tableObj)){
		lua_pushobject(tableObj);
		lua_pushstring("color");
		lua_Object colorObj = lua_gettable();
		if (lua_isuserdata(colorObj) && lua_tag(colorObj) == MKTAG('C','O','L','R')) {
			color = getcolor(colorObj);
		}

		lua_pushobject(tableObj);
		lua_pushstring("filled");
		lua_Object objFilled = lua_gettable();
		if (!lua_isnil(objFilled))
			filled = true;
	}

	PrimitiveObject *p = new PrimitiveObject();
	p->createRectangle(p1, p2, color, filled);
	p->draw();
	delete p;
}

void Lua_V1::KillPrimitive() {
	lua_Object primObj = lua_getparam(1);

	if (!lua_isuserdata(primObj) || lua_tag(primObj) != MKTAG('P','R','I','M'))
		return;

	PrimitiveObject *prim = getprimitive(primObj);
	delete prim;
}

void Lua_V1::DimScreen() {
	AGLMan.getTarget()->storeContent();
	AGLMan.getTarget()->dim(0.1f);
}

void Lua_V1::DimRegion() {
	int x = (int)lua_getnumber(lua_getparam(1));
	int y = (int)lua_getnumber(lua_getparam(2));
	int w = (int)lua_getnumber(lua_getparam(3));
	int h = (int)lua_getnumber(lua_getparam(4));
	float level = lua_getnumber(lua_getparam(5));
	AGLMan.getTarget()->dimRegion(x, y, w, h, level);
}

void Lua_V1::ScreenShot() {
	int width = (int)lua_getnumber(lua_getparam(1));
	int height = (int)lua_getnumber(lua_getparam(2));
	GrimEngine::EngineMode mode = g_grim->getMode();
	g_grim->setMode(GrimEngine::NormalMode);
	g_grim->updateDisplayScene();

	Graphics::PixelFormat format(2, 5, 6, 5, 0, 11, 5, 0, 0);
	Graphics::Surface *screenshot = AGLMan.getTarget()->getScreenshot(format, width, height);
	g_grim->setMode(mode);
	if (screenshot) {
		Bitmap *bitmap = new Bitmap(screenshot);
		lua_pushusertag(bitmap->getId(), MKTAG('V','B','U','F'));
	} else {
		lua_pushnil();
	}
}

void Lua_V1::SetGamma() {
	lua_Object levelObj = lua_getparam(1);

	if (!lua_isnumber(levelObj))
		return;
	int level = (int)lua_getnumber(levelObj);

	// FIXME: func(level)
	warning("Lua_V1::SetGamma, implement opcode, level: %d", level);
}

void Lua_V1::Display() {
	if (g_grim->getFlipEnable()) {
		AGLMan.flipBuffer();
	}
}

void Lua_V1::EngineDisplay() {
	// it enable/disable updating display
	bool mode = (int)lua_getnumber(lua_getparam(1)) != 0;
	if (mode) {
		g_grim->setFlipEnable(true);
	} else {
		g_grim->setFlipEnable(false);
	}
}

void Lua_V1::ForceRefresh() {
	g_grim->refreshDrawMode();
}

void Lua_V1::RenderModeUser() {
	lua_Object param1 = lua_getparam(1);
	if (!lua_isnil(param1) && g_grim->getMode() != GrimEngine::DrawMode) {
		g_grim->setPreviousMode(g_grim->getMode());
		g_movie->pause(true);
		g_grim->setMode(GrimEngine::DrawMode);
	} else if (lua_isnil(param1) && g_grim->getMode() == GrimEngine::DrawMode) {
		g_movie->pause(false);
		g_grim->refreshDrawMode();
		g_grim->setMode(g_grim->getPreviousMode());
	}
}

void Lua_V1::IrisUp() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object timeObj = lua_getparam(3);

	g_grim->playIrisAnimation(Iris::Open, (int)lua_getnumber(xObj), (int)lua_getnumber(yObj), (int)lua_getnumber(timeObj));
}

void Lua_V1::IrisDown() {
	lua_Object xObj = lua_getparam(1);
	lua_Object yObj = lua_getparam(2);
	lua_Object timeObj = lua_getparam(3);

	g_grim->playIrisAnimation(Iris::Close, (int)lua_getnumber(xObj), (int)lua_getnumber(yObj), (int)lua_getnumber(timeObj));
}

void Lua_V1::PreRender() {
	Bitmap::renderBitmaps(getbool(1));
	Bitmap::renderZBitmaps(getbool(2));
}

} // end of namespace Grim
