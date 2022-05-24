#include "register_types.h"
#include "core/class_db.h"
#include "player_editor_control.h"

void register_content_editor_types() {
	ClassDB::register_class<PlayerEditorControl>();
}

void unregister_content_editor_types() {
}
