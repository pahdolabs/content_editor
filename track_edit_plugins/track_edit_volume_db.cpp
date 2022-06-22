#include "track_edit_volume_db.h"

#include "../track_editor/track_editor.h"
#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"

#include "../editor_consts.h"
#include "../icons_cache.h"

int TrackEditVolumeDB::get_key_height() const {
	Ref<Texture> volume_texture = _IconsCache::get_singleton()->get_icon("ColorTrackVu");
	return volume_texture->get_height() * 1.2;
}

void TrackEditVolumeDB::draw_bg(int p_clip_left, int p_clip_right) {
	Ref<Texture> volume_texture = _IconsCache::get_singleton()->get_icon("ColorTrackVu");
	int tex_h = volume_texture != nullptr ? volume_texture->get_height() : 0;

	int y_from = (get_size().height - tex_h) / 2;
	int y_size = tex_h;

	Color color(1, 1, 1, 0.3);
	if (volume_texture != nullptr) {
		draw_texture_rect(volume_texture, Rect2(p_clip_left, y_from, p_clip_right - p_clip_left, y_from + y_size), false, color);
	}
}

void TrackEditVolumeDB::draw_fg(int p_clip_left, int p_clip_right) {
	Ref<Texture> volume_texture = _IconsCache::get_singleton()->get_icon("ColorTrackVu");
	int tex_h = volume_texture->get_height();
	int y_from = (get_size().height - tex_h) / 2;
	int db0 = y_from + (24 / 80.0) * tex_h;

	draw_line(Vector2(p_clip_left, db0), Vector2(p_clip_right, db0), Color(1, 1, 1, 0.3));
}

void TrackEditVolumeDB::draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) {
	if (p_x > p_clip_right || p_next_x < p_clip_left) {
		return;
	}

	float db = get_animation()->track_get_key_value(get_track(), p_index);
	float db_n = get_animation()->track_get_key_value(get_track(), p_index + 1);

	db = CLAMP(db, -60, 24);
	db_n = CLAMP(db_n, -60, 24);

	float h = 1.0 - ((db + 60) / 84.0);
	float h_n = 1.0 - ((db_n + 60) / 84.0);

	int from_x = p_x;
	int to_x = p_next_x;

	if (from_x < p_clip_left) {
		h = Math::lerp(h, h_n, float(p_clip_left - from_x) / float(to_x - from_x));
		from_x = p_clip_left;
	}

	if (to_x > p_clip_right) {
		h_n = Math::lerp(h, h_n, float(p_clip_right - from_x) / float(to_x - from_x));
		to_x = p_clip_right;
	}

	Ref<Texture> volume_texture = _IconsCache::get_singleton()->get_icon("ColorTrackVu");
	int tex_h = volume_texture->get_height();

	int y_from = (get_size().height - tex_h) / 2;

	Color color = get_color("font_color", "Label");
	color.a *= 0.7;

	draw_line(Point2(from_x, y_from + h * tex_h), Point2(to_x, y_from + h_n * tex_h), color, 2);
}