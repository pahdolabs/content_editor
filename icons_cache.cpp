#include "icons_cache.h"

#include <core/project_settings.h>
#include <core/os/dir_access.h>
#include <modules/svg/image_loader_svg.h>

IconsCache *IconsCache::singleton;

IconsCache *IconsCache::get_singleton() {
	if(singleton == nullptr) {
		singleton = memnew(IconsCache);
	}
	return singleton;
}

Ref<Texture> IconsCache::get_icon(const String &p_icon_name) {
	if(icons.has(p_icon_name)) {
		return icons[p_icon_name];
	}
	return nullptr;
}

bool IconsCache::has_icon(const String& p_icon_name) {
	return icons.has(p_icon_name);
}

void IconsCache::add_icon_path(const String &p_icon_path) {
	DirAccessRef da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	Error err;
	da->open(p_icon_path, &err);
	if(err != OK) {
		return;
	}

	da->list_dir_begin();
	String next = da->get_next();

	ImageLoaderSVG loader;
	while(!next.empty()) {
		if (!da->current_is_dir()) {
			FileAccessRef fa = FileAccess::create_for_path(p_icon_path + "/" + next);
			Ref<Image> icon_data;

			icon_data.instance();
			if(loader.load_image(icon_data, fa, false, 1.0) == OK) {
				Ref<ImageTexture> texture;
				texture.instance();
				texture->create_from_image(icon_data);
				icons[next.get_file().replace("." + next.get_extension(), "")] = texture;
			}
		}
		next = da->get_next();
	}
	emit_signal("icons_changed");
}

void IconsCache::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_icon_path", "icon_path"), &IconsCache::add_icon_path);
	ClassDB::bind_method(D_METHOD("get_icon"), &IconsCache::get_icon);
	ADD_SIGNAL(MethodInfo("icons_changed"));
}

IconsCache::IconsCache() {
}

IconsCache::~IconsCache() {
	icons.clear();
}
