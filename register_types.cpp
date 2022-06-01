#include "register_types.h"

#include "consts.h"
#include "core/engine.h"

#include "icons_cache.h"
#include "core/class_db.h"
#include "track_editor/player_editor_control.h"
#include "track_edit_plugins/track_edit.h"
#include "track_editor/track_editor.h"
#include "track_editor/timeline_edit.h"

void register_content_editor_types() {
	ClassDB::register_class<PlayerEditorControl>();
	ClassDB::register_class<TrackEditor>();
	ClassDB::register_class<TimelineEdit>();
	ClassDB::register_virtual_class<TrackEdit>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("IconsCache", IconsCache::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("Colors", Colors::get_singleton()));
}

void unregister_content_editor_types() {
}
