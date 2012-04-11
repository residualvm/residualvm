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

#include "engines/advancedDetector.h"
#include "engines/obsolete.h"

#include "engines/grim/grim.h"
#include "engines/grim/savegame.h"

#include "common/system.h"
#include "common/savefile.h"

namespace Grim {

struct GrimGameDescription {
	ADGameDescription desc;
	GrimGameType gameType;
};

static const PlainGameDescriptor grimGames[] = {
	{"grim", "Grim Fandango"},
	{"monkey4", "Escape From Monkey Island"},
	{0, 0}
};

#define GAMEOPTION_LOAD_DATAUSR GUIO_GAMEOPTIONS1

static const ADExtraGuiOptionsMap gameGuiOptions[] = {
	{
		GAMEOPTION_LOAD_DATAUSR,
		{
			"Load user patch (unsupported)",
			"Load an user patch. Please note that the ResidualVM-team doesn't provide support for using such patches.",
			"datausr_load",
			false
		}
	},

	AD_EXTRA_GUI_OPTIONS_TERMINATOR
};

static const GrimGameDescription gameDescriptions[] = {
	{
		// Grim Fandango English version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("VOX0001.LAB", "444f05f2af689c1bffd179b8b6a632bd", 57993159),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango English version (unpatched)
		{
			"grim",
			"",
			AD_ENTRY1s("VOX0001.LAB", "8b12ed530195c6c577436df27df62ecb", 58011176),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango French version (un/patched ???)
		{
			"grim",
			"",
			AD_ENTRY1s("VOX0001.LAB", "19bc0dc9554257b1f021463de54f359f", 56268691),
			Common::FR_FRA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango Italian version
		{
			"grim",
			"",
			AD_ENTRY1s("VOX0001.LAB", "9e7075f3fb0427ae8136b290538d07dd", 62185775),
			Common::IT_ITA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango Spanish version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("VOX0001.LAB", "85d3e9504c481c5ccf2119ea6e0f4e2f", 53831340),
			Common::ES_ESP,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango German version
		{
			"grim",
			"",
			AD_ENTRY1s("VOX0001.LAB", "d304aa402098de5966816c0a11e45816", 66829347),
			Common::DE_DEU,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
/*	{
		// Grim Fandango German version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("grim.tab", "464138caf47e580cbb237dee10674b16", 398671),
			Common::DE_DEU,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango Spanish version
		{
			"grim",
			"",
			AD_ENTRY1s("grim.tab", "b1460cd029f13718f7f62c2403e047ec", 372709),
			Common::ES_ESP,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango Spanish version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("grim.tab", "b1460cd029f13718f7f62c2403e047ec", 372020),
			Common::ES_ESP,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango Italian version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("grim.tab", "2d99c796b7a4e5c421cae49dc29dab6c", 369242),
			Common::IT_ITA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango French version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("grim.tab", "3bd00ca87214862c012ac99e1758dd83", 386292),
			Common::FR_FRA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
	{
		// Grim Fandango Portuguese version (patched)
		{
			"grim",
			"",
			AD_ENTRY1s("grim.tab", "4dc16be476bb6036b423bc331ca8281a", 362994),
			Common::PT_BRA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
*/	{
		// Grim Fandango English demo version
		{
			"grim",
			"Demo",
			AD_ENTRY1s("gfdemo01.lab", "755cdac083f7f751bec7506402278f1a", 29489930),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_DEMO,
			GUIO1(GAMEOPTION_LOAD_DATAUSR)
		},
		GType_GRIM
	},
#ifdef ENABLE_MONKEY4
	{
		// Escape from Monkey Island English
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "61959da91d864bf5f4588daa4a5a3019", 18515664),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island German
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "007a33881478be6b6e0228d8888536ae", 18512568),
			Common::DE_DEU,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island Italian
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "d2f010c1cd1fd002eea403282a6b9a1e", 18513451),
			Common::IT_ITA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island Spanish
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "0d459954031c086a0448d2eb3fa068a1", 18514404),
			Common::ES_ESP,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island French
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "151af0a694382af873f325fcea293bb1", 18514420),
			Common::FR_FRA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island Portuguese
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "030e7637aee7886a3caad60cf102f797", 18515747),
			Common::PT_BRA,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island Russian
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "779561a70a11dd5686974f122fc1516c", 18500052),
			Common::RU_RUS,
			Common::kPlatformWindows,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island English PS2
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "0dc9a4df0d8553f277d8dc8e23b6249d", 34593974),
			Common::EN_ANY,
			Common::kPlatformPS2,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island German PS2
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "5b5c7a3964c168eab44b82981db357d8", 34642186),
			Common::DE_DEU,
			Common::kPlatformPS2,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island Italian PS2
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "2de68c8fd955c1a3c50202b072bde0cb", 34642651),
			Common::IT_ITA,
			Common::kPlatformPS2,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island Spanish PS2
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "ff6689dcca36c249ec834a3019aeb397", 34642656),
			Common::ES_ESP,
			Common::kPlatformPS2,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island French PS2
		{
			"monkey4",
			"",
			AD_ENTRY1s("artAll.m4b", "5ce964a19a8672944b9b62170e45ce28", 34593681),
			Common::FR_FRA,
			Common::kPlatformPS2,
			ADGF_NO_FLAGS,
			GUIO_NONE
		},
		GType_MONKEY4
	},
	{
		// Escape from Monkey Island demo
		{
			"monkey4",
			"Demo",
			AD_ENTRY1s("magdemo.lab", "9e7eaa1b9317ff47d5deeda0b2c42ce3", 19826116),
			Common::EN_ANY,
			Common::kPlatformWindows,
			ADGF_DEMO,
			GUIO_NONE
		},
		GType_MONKEY4
	},
#endif // ENABLE_MONKEY4

	{ AD_TABLE_END_MARKER, GType_GRIM }
};

static const Engines::ObsoleteGameID obsoleteGameIDsTable[] = {
	{"grimdemo", "grim", Common::kPlatformWindows},
	{0, 0, Common::kPlatformUnknown}
};

class GrimMetaEngine : public AdvancedMetaEngine {
public:
	GrimMetaEngine() : AdvancedMetaEngine(Grim::gameDescriptions, sizeof(Grim::GrimGameDescription), grimGames, gameGuiOptions) {
		_singleid = "grim";
		_guioptions = GUIO_NOMIDI;
	}

	virtual GameDescriptor findGame(const char *gameid) const {
		return Engines::findGameID(gameid, _gameids, obsoleteGameIDsTable);
	}

	virtual const char *getName() const {
		return "Grim Engine";
	}

	virtual const char *getOriginalCopyright() const {
		return "LucasArts GrimE Games (C) LucasArts";
	}

	virtual Common::Error createInstance(OSystem *syst, Engine **engine) const {
		Engines::upgradeTargetIfNecessary(obsoleteGameIDsTable);
		return AdvancedMetaEngine::createInstance(syst, engine);
	}

	virtual bool createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const;

	virtual bool hasFeature(MetaEngineFeature f) const;

	virtual SaveStateList listSaves(const char *target) const;
};

bool GrimMetaEngine::createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const {
	const GrimGameDescription *gd = (const GrimGameDescription *)desc;

	if (gd)
		*engine = new GrimEngine(syst, gd->desc.flags, gd->gameType, gd->desc.platform, gd->desc.language);

	return gd != 0;
}

bool GrimMetaEngine::hasFeature(MetaEngineFeature f) const {
	return
		(f == kSupportsListSaves) ||
		(f == kSupportsLoadingDuringStartup);
}

static bool cmpSave(const SaveStateDescriptor &x, const SaveStateDescriptor &y) {
	return x.getSaveSlot() < y.getSaveSlot();
}

SaveStateList GrimMetaEngine::listSaves(const char *target) const {
	Common::SaveFileManager *saveFileMan = g_system->getSavefileManager();
	Common::StringArray filenames;
	Common::String pattern = "grim*.gsv";

	filenames = saveFileMan->listSavefiles(pattern);

	SaveStateList saveList;
	char str[256];
	int32 strSize;
	for (Common::StringArray::const_iterator file = filenames.begin(); file != filenames.end(); ++file) {
		// Obtain the last digits of the filename, since they correspond to the save slot
		int slotNum = atoi(file->c_str() + 4);

		if (slotNum >= 0) {
			SaveGame *savedState = SaveGame::openForLoading(*file);
			if (savedState && savedState->isCompatible()) {
				savedState->beginSection('SUBS');
				strSize = savedState->readLESint32();
				savedState->read(str, strSize);
				savedState->endSection();
				saveList.push_back(SaveStateDescriptor(slotNum, str));
			}
			delete savedState;
		}
	}

	Common::sort(saveList.begin(), saveList.end(), cmpSave);
	return saveList;
}

} // End of namespace Grim

#if PLUGIN_ENABLED_DYNAMIC(GRIM)
	REGISTER_PLUGIN_DYNAMIC(GRIM, PLUGIN_TYPE_ENGINE, Grim::GrimMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(GRIM, PLUGIN_TYPE_ENGINE, Grim::GrimMetaEngine);
#endif
