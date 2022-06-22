#include "register_types.h"

#include "editor_consts.h"
#include "core/engine.h"

#include "icons_cache.h"
#include "core/class_db.h"
#include "gizmos/pahdo_spatial_gizmo.h"
#include "gizmos/pahdo_spatial_gizmo_plugin.h"
#include "gizmos/viewport_gizmo_controller.h"
#include "sfx_gen/sfx_preview_generator.h"
#include "sfx_gen/sfx_preview.h"
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
	ClassDB::register_class<SFXPreview>();

	ClassDB::register_class<_SFXPreviewGenerator>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("SFXPreviewGenerator", _SFXPreviewGenerator::get_singleton()));

	ClassDB::register_class<_IconsCache>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("IconsCache", _IconsCache::get_singleton()));

	ClassDB::register_class<_EditorConsts>();
	Engine::get_singleton()->add_singleton(Engine::Singleton("EditorConsts", _EditorConsts::get_singleton()));
}

void unregister_content_editor_types() {
}
