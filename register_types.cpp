#include "register_types.h"

#include <core/engine.h>

#include "icons_cache.h"
#include "core/class_db.h"
#include "track_editor/player_editor_control.h"

void register_content_editor_types() {
	ClassDB::register_class<PlayerEditorControl>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("IconsCache", IconsCache::get_singleton()));
}

void unregister_content_editor_types() {
}
