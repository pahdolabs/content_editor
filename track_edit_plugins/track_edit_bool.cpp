#include "track_edit_bool.h"

#include "../track_editor/track_editor.h"
#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"

#include "../editor_consts.h"
#include "../icons_cache.h"

int TrackEditBool::get_key_height() const {
	Ref<Texture> checked = _IconsCache::get_singleton()->get_icon("checked");
	return checked->get_height();
}

Rect2 TrackEditBool::get_key_rect(int p_index, float p_pixels_sec) {
	Ref<Texture> checked = _IconsCache::get_singleton()->get_icon("checked");
	return Rect2(-checked->get_width() / 2, 0, checked->get_width(), get_size().height);
}

bool TrackEditBool::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditBool::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	bool checked = get_animation()->track_get_key_value(get_track(), p_index);
	Ref<Texture> icon = _IconsCache::get_singleton()->get_icon(checked ? "checked" : "unchecked");

	Vector2 ofs(p_x - icon->get_width() / 2, int(get_size().height - icon->get_height()) / 2);

	if (ofs.x + icon->get_width() / 2 < p_clip_left) {
		return;
	}

	if (ofs.x + icon->get_width() / 2 > p_clip_right) {
		return;
	}

	if (icon != nullptr) {
		draw_texture(icon, ofs);
	}

	if (p_selected) {
		Color color = _EditorConsts::ACCENT_COLOR;
		draw_rect_clipped(Rect2(ofs, icon->get_size()), color, false);
	}
}
