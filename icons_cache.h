#ifndef ICONS_CACHE_H
#define ICONS_CACHE_H

#include "scene/resources/texture.h"

class IconsCache : public Object {
	GDCLASS(IconsCache, Object);

	Map<String, Ref<Texture>> icons;

	static IconsCache *singleton;

protected:
	static void _bind_methods();

	IconsCache();

public:
	static IconsCache *get_singleton();

	Ref<Texture> get_icon(const String &p_icon_name);
	bool has_icon(const String& p_icon_name);
	void add_icon_path(const String &p_icon_path);

	~IconsCache();
};

#endif
