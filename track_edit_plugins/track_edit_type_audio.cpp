#include "track_edit_type_audio.h"

#include "../track_editor/track_editor.h"
#include "../track_editor/timeline_edit.h"
#include "core/undo_redo.h"
#include "core/os/input_event.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"

#include "../editor_consts.h"

void TrackEditTypeAudio::_preview_changed(ObjectID p_which) {
	for (int i = 0; i < get_animation()->track_get_key_count(get_track()); i++) {
		Ref<AudioStream> stream = get_animation()->audio_track_get_key_stream(get_track(), i);
		if (stream.is_valid() && stream->get_instance_id() == p_which) {
			update();
			return;
		}
	}
}

int TrackEditTypeAudio::get_key_height() const {
	Ref<Font> font = get_font("font", "Label");
		return int(font->get_height() * 1.5);
}

Rect2 TrackEditTypeAudio::get_key_rect(int p_index, float p_pixels_sec) {
	Ref<AudioStream> stream = get_animation()->audio_track_get_key_stream(get_track(), p_index);

	if (!stream.is_valid()) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	float start_ofs = get_animation()->audio_track_get_key_start_offset(get_track(), p_index);
	float end_ofs = get_animation()->audio_track_get_key_end_offset(get_track(), p_index);

	float len = stream->get_length();

	if (len == 0) {
		//Ref<AudioStreamPreview> preview = AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);
		//len = preview->get_length();
	}

	len -= end_ofs;
	len -= start_ofs;
	if (len <= 0.001) {
		len = 0.001;
	}

	if (get_animation()->track_get_key_count(get_track()) > p_index + 1) {
		len = MIN(len, get_animation()->track_get_key_time(get_track(), p_index + 1) - get_animation()->track_get_key_time(get_track(), p_index));
	}

	return Rect2(0, 0, len * p_pixels_sec, get_size().height);
}

bool TrackEditTypeAudio::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditTypeAudio::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	Ref<AudioStream> stream = get_animation()->audio_track_get_key_stream(get_track(), p_index);

	if (!stream.is_valid()) {
		TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
		return;
	}

	float start_ofs = get_animation()->audio_track_get_key_start_offset(get_track(), p_index);
	float end_ofs = get_animation()->audio_track_get_key_end_offset(get_track(), p_index);

	if (len_resizing && p_index == len_resizing_index) {
		float ofs_local = -len_resizing_rel / get_timeline()->get_zoom_scale();
		if (len_resizing_start) {
			start_ofs += ofs_local;
			if (start_ofs < 0) {
				start_ofs = 0;
			}
		}
		else {
			end_ofs += ofs_local;
			if (end_ofs < 0) {
				end_ofs = 0;
			}
		}
	}

	Ref<Font> font = get_font("font", "Label");
		float fh = int(font->get_height() * 1.5);

	float len = stream->get_length();

	//Ref<AudioStreamPreview> preview = AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);

	float preview_len = 0.0; // preview->get_length();

	if (len == 0) {
		len = preview_len;
	}

	int pixel_total_len = len * p_pixels_sec;

	len -= end_ofs;
	len -= start_ofs;

	if (len <= 0.001) {
		len = 0.001;
	}

	int pixel_len = len * p_pixels_sec;

	int pixel_begin = p_x;
	int pixel_end = p_x + pixel_len;

	if (pixel_end < p_clip_left) {
		return;
	}

	if (pixel_begin > p_clip_right) {
		return;
	}

	int from_x = MAX(pixel_begin, p_clip_left);
	int to_x = MIN(pixel_end, p_clip_right);

	if (get_animation()->track_get_key_count(get_track()) > p_index + 1) {
		float limit = MIN(len, get_animation()->track_get_key_time(get_track(), p_index + 1) - get_animation()->track_get_key_time(get_track(), p_index));
		int limit_x = pixel_begin + limit * p_pixels_sec;
		to_x = MIN(limit_x, to_x);
	}

	if (to_x <= from_x) {
		to_x = from_x + 1;
	}

	int h = get_size().height;
	Rect2 rect = Rect2(from_x, (h - fh) / 2, to_x - from_x, fh);
	draw_rect(rect, Color(0.25, 0.25, 0.25));

	Vector<Vector2> lines;
	lines.resize((to_x - from_x + 1) * 2);
	//preview_len = preview->get_length();

	for (int i = from_x; i < to_x; i++) {
		float ofs = (i - pixel_begin) * preview_len / pixel_total_len;
		float ofs_n = ((i + 1) - pixel_begin) * preview_len / pixel_total_len;
		ofs += start_ofs;
		ofs_n += start_ofs;

		float max = 0.0; // preview->get_max(ofs, ofs_n) * 0.5 + 0.5;
		float min = 0.0; // preview->get_min(ofs, ofs_n) * 0.5 + 0.5;

		int idx = i - from_x;
		lines.write[idx * 2 + 0] = Vector2(i, rect.position.y + min * rect.size.y);
		lines.write[idx * 2 + 1] = Vector2(i, rect.position.y + max * rect.size.y);
	}

	Vector<Color> color;
	color.push_back(Color(0.75, 0.75, 0.75));

	VisualServer::get_singleton()->canvas_item_add_multiline(get_canvas_item(), lines, color);

	Color cut_color = _EditorConsts::ACCENT_COLOR;
	cut_color.a = 0.7;
	if (start_ofs > 0 && pixel_begin > p_clip_left) {
		draw_rect(Rect2(pixel_begin, rect.position.y, 1, rect.size.y), cut_color);
	}
	if (end_ofs > 0 && pixel_end < p_clip_right) {
		draw_rect(Rect2(pixel_end, rect.position.y, 1, rect.size.y), cut_color);
	}

	if (p_selected) {
		Color accent = _EditorConsts::ACCENT_COLOR;
		draw_rect(rect, accent, false);
	}
}

void TrackEditTypeAudio::_bind_methods() {
}

TrackEditTypeAudio::TrackEditTypeAudio() {
	//AudioStreamPreviewGenerator::get_singleton()->connect("preview_updated", this, "_preview_changed");
	len_resizing = false;
}

bool TrackEditTypeAudio::can_drop_data(const Point2& p_point, const Variant& p_data) const {
	if (p_point.x > get_timeline()->get_name_limit() && p_point.x < get_size().width - get_timeline()->get_buttons_width()) {
		Dictionary drag_data = p_data;
		if (drag_data.has("type") && String(drag_data["type"]) == "resource") {
			Ref<AudioStream> res = drag_data["resource"];
			if (res.is_valid()) {
				return true;
			}
		}

		if (drag_data.has("type") && String(drag_data["type"]) == "files") {
			Vector<String> files = drag_data["files"];

			if (files.size() == 1) {
				String file = files[0];
				Ref<AudioStream> res = ResourceLoader::load(file);
				if (res.is_valid()) {
					return true;
				}
			}
		}
	}

	return TrackEdit::can_drop_data(p_point, p_data);
}

void TrackEditTypeAudio::drop_data(const Point2& p_point, const Variant& p_data) {
	if (p_point.x > get_timeline()->get_name_limit() && p_point.x < get_size().width - get_timeline()->get_buttons_width()) {
		Ref<AudioStream> stream;
		Dictionary drag_data = p_data;
		if (drag_data.has("type") && String(drag_data["type"]) == "resource") {
			stream = drag_data["resource"];
		}
		else if (drag_data.has("type") && String(drag_data["type"]) == "files") {
			Vector<String> files = drag_data["files"];

			if (files.size() == 1) {
				String file = files[0];
				stream = ResourceLoader::load(file);
			}
		}

		if (stream.is_valid()) {
			int x = p_point.x - get_timeline()->get_name_limit();
			float ofs = x / get_timeline()->get_zoom_scale();
			ofs += get_timeline()->get_value();

			ofs = get_editor()->snap_time(ofs);

			while (get_animation()->track_find_key(get_track(), ofs, true) != -1) { //make sure insertion point is valid
				ofs += 0.001;
			}

			get_undo_redo()->create_action(TTR("Add Audio Track Clip"));
			get_undo_redo()->add_do_method(get_animation().ptr(), "audio_track_insert_key", get_track(), ofs, stream);
			get_undo_redo()->add_undo_method(get_animation().ptr(), "track_remove_key_at_time", get_track(), ofs);
			get_undo_redo()->commit_action();

			update();
			return;
		}
	}

	TrackEdit::drop_data(p_point, p_data);
}

void TrackEditTypeAudio::_gui_input(const Ref<InputEvent>& p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseMotion> mm = p_event;
	if (!len_resizing && mm.is_valid()) {
		bool use_hsize_cursor = false;
		for (int i = 0; i < get_animation()->track_get_key_count(get_track()); i++) {
			Ref<AudioStream> stream = get_animation()->audio_track_get_key_stream(get_track(), i);

			if (!stream.is_valid()) {
				continue;
			}

			float start_ofs = get_animation()->audio_track_get_key_start_offset(get_track(), i);
			float end_ofs = get_animation()->audio_track_get_key_end_offset(get_track(), i);
			float len = stream->get_length();

			if (len == 0) {
				//Ref<AudioStreamPreview> preview = AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);
				//float preview_len = preview->get_length();
				len = 0.0; //preview_len;
			}

			len -= end_ofs;
			len -= start_ofs;
			if (len <= 0.001) {
				len = 0.001;
			}

			if (get_animation()->track_get_key_count(get_track()) > i + 1) {
				len = MIN(len, get_animation()->track_get_key_time(get_track(), i + 1) - get_animation()->track_get_key_time(get_track(), i));
			}

			float ofs = get_animation()->track_get_key_time(get_track(), i);

			ofs -= get_timeline()->get_value();
			ofs *= get_timeline()->get_zoom_scale();
			ofs += get_timeline()->get_name_limit();

			int end = ofs + len * get_timeline()->get_zoom_scale();

			if (end >= get_timeline()->get_name_limit() && end <= get_size().width - get_timeline()->get_buttons_width() && ABS(mm->get_position().x - end) < 5 * 1) {
				use_hsize_cursor = true;
				len_resizing_index = i;
			}
		}
		over_drag_position = use_hsize_cursor;
	}

	if (len_resizing && mm.is_valid()) {
		len_resizing_rel += mm->get_relative().x;
		len_resizing_start = mm->get_shift();
		update();
		accept_event();
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() ==  BUTTON_LEFT && over_drag_position) {
		len_resizing = true;
		len_resizing_start = mb->get_shift();
		len_resizing_from_px = mb->get_position().x;
		len_resizing_rel = 0;
		update();
		accept_event();
		return;
	}

	if (len_resizing && mb.is_valid() && !mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT) {
		float ofs_local = -len_resizing_rel / get_timeline()->get_zoom_scale();
		if (len_resizing_start) {
			float prev_ofs = get_animation()->audio_track_get_key_start_offset(get_track(), len_resizing_index);
			get_undo_redo()->create_action(TTR("Change Audio Track Clip Start Offset"));
			get_undo_redo()->add_do_method(get_animation().ptr(), "audio_track_set_key_start_offset", get_track(), len_resizing_index, prev_ofs + ofs_local);
			get_undo_redo()->add_undo_method(get_animation().ptr(), "audio_track_set_key_start_offset", get_track(), len_resizing_index, prev_ofs);
			get_undo_redo()->commit_action();

		}
		else {
			float prev_ofs = get_animation()->audio_track_get_key_end_offset(get_track(), len_resizing_index);
			get_undo_redo()->create_action(TTR("Change Audio Track Clip End Offset"));
			get_undo_redo()->add_do_method(get_animation().ptr(), "audio_track_set_key_end_offset", get_track(), len_resizing_index, prev_ofs + ofs_local);
			get_undo_redo()->add_undo_method(get_animation().ptr(), "audio_track_set_key_end_offset", get_track(), len_resizing_index, prev_ofs);
			get_undo_redo()->commit_action();
		}

		len_resizing_index = -1;
		update();
		accept_event();
		return;
	}

	TrackEdit::_gui_input(p_event);
}

Control::CursorShape TrackEditTypeAudio::get_cursor_shape(const Point2& p_pos) const {
	if (over_drag_position || len_resizing) {
		return Control::CURSOR_HSIZE;
	}
	else {
		return get_default_cursor_shape();
	}
}