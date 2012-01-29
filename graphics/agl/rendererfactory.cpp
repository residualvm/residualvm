
#include "common/foreach.h"

#include "graphics/agl/rendererfactory.h"

namespace Common {
DECLARE_SINGLETON(AGL::RendererFactory);
}

namespace AGL {

RendererPlugin *RendererFactory::getPlugin(const Common::String &str) const {
	foreach (RendererPlugin *p, RendererFactory::instance().getPlugins()) {
		if (p->getName() == str) {
			return p;
		}
	};

	return NULL;
}

const RendererPlugin::List &RendererFactory::getPlugins() const {
	return (const RendererPlugin::List &)PluginManager::instance().getPlugins(PLUGIN_TYPE_AGL_RENDERER);
}

}
