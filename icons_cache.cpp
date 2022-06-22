#include "icons_cache.h"

#include "core/project_settings.h"
#include "core/os/dir_access.h"
#include "modules/svg/image_loader_svg.h"

_IconsCache *_IconsCache::singleton;

_IconsCache *_IconsCache::get_singleton() {
	if(singleton == nullptr) {
		singleton = memnew(_IconsCache);
	}
	return singleton;
}

Ref<Texture> _IconsCache::get_icon(const String &p_icon_name) {
	if(icons.has(p_icon_name)) {
		return icons[p_icon_name];
	}
	return nullptr;
}

bool _IconsCache::has_icon(const String& p_icon_name) {
	return icons.has(p_icon_name);
}

void _IconsCache::add_icon_path(const String &p_icon_path) {
	Error err;
	DirAccess *da = DirAccess::open(p_icon_path, &err);
	if(err != OK) {
		return;
	}

	da->list_dir_begin();
	String next = da->get_next();

	ImageLoaderSVG loader;
	while(!next.empty()) {
		if (!da->current_is_dir()) {
			if (next.get_extension() == "svg") {
				FileAccess *fa = FileAccess::open(p_icon_path + "/" + next, FileAccess::READ, &err);
				if (err == OK) {
					Ref<Image> icon_data;

					icon_data.instance();
					if (loader.load_image(icon_data, fa, false, 1.0) == OK) {
						Ref<ImageTexture> texture;
						texture.instance();
						texture->create_from_image(icon_data);
						icons[next.get_file().replace("." + next.get_extension(), "")] = texture;
					}
					fa->close();
				}
			}
		}
		next = da->get_next();
	}
	emit_signal("icons_changed");
}

void _IconsCache::_bind_methods() {
	ClassDB::bind_method(D_METHOD("add_icon_path", "icon_path"), &_IconsCache::add_icon_path);
	ClassDB::bind_method(D_METHOD("get_icon"), &_IconsCache::get_icon);
	ADD_SIGNAL(MethodInfo("icons_changed"));
}

_IconsCache::_IconsCache() {
}

_IconsCache::~_IconsCache() {
	icons.clear();
}
