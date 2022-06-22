#ifndef ICONS_CACHE_H
#define ICONS_CACHE_H

#include "scene/resources/texture.h"

class _IconsCache : public Object {
	GDCLASS(_IconsCache, Object);

	Map<String, Ref<Texture>> icons;

	static _IconsCache *singleton;

protected:
	static void _bind_methods();

	_IconsCache();

public:
	static _IconsCache *get_singleton();

	Ref<Texture> get_icon(const String &p_icon_name);
	bool has_icon(const String& p_icon_name);
	void add_icon_path(const String &p_icon_path);

	~_IconsCache();
};

#endif
