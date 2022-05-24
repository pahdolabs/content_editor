#include "register_types.h"
#include "core/class_db.h"
#include "animation_player_editor.h"

void register_content_editor_types() {
	ClassDB::register_class<PlayerEditor>();
}

void unregister_content_editor_types() {
}
