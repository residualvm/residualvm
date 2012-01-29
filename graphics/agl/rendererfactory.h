
#ifndef AGL_RENDERERFACTORY_H
#define AGL_RENDERERFACTORY_H

#include "base/plugins.h"

#include "graphics/agl/renderer.h"

namespace AGL {

typedef PluginSubclass<RendererPluginObject> RendererPlugin;

class RendererFactory : public Common::Singleton<RendererFactory> {
private:
	friend class Common::Singleton<SingletonBaseType>;

public:
	RendererPlugin *getPlugin(const Common::String &name) const;
	const RendererPlugin::List &getPlugins() const;
};

}

#endif
