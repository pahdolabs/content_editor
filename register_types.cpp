#include "register_types.h"

#include "EditorConsts.h"
#include "core/engine.h"

#include "icons_cache.h"
#include "core/class_db.h"
#include "gizmos/pahdo_spatial_gizmo.h"
#include "gizmos/pahdo_spatial_gizmo_plugin.h"
#include "gizmos/viewport_gizmo_controller.h"
#include "track_editor/player_editor_control.h"
#include "track_edit_plugins/track_edit.h"
#include "track_editor/track_editor.h"
#include "track_editor/timeline_edit.h"

void register_content_editor_types() {
	ClassDB::register_class<PlayerEditorControl>();
	ClassDB::register_class<TrackEditor>();
	ClassDB::register_class<TimelineEdit>();
	ClassDB::register_class<TrackEdit>();
	ClassDB::register_class<PahdoSpatialGizmo>();
	ClassDB::register_class<PahdoSpatialGizmoPlugin>();
	ClassDB::register_class<ViewportGizmoController>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("IconsCache", IconsCache::get_singleton()));
	Engine::get_singleton()->add_singleton(Engine::Singleton("EditorConsts", EditorConsts::get_singleton()));
}

void unregister_content_editor_types() {
}
