#include "track_edit_audio.h"

#include "../track_editor/track_editor.h"
#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"

#include "../consts.h"

void TrackEditAudio::_preview_changed(ObjectID p_which) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		return;
	}

	Ref<AudioStream> stream = object->call("get_stream");

	if (stream.is_valid() && stream->get_instance_id() == p_which) {
		update();
	}
}

int TrackEditAudio::get_key_height() const {
	if (!ObjectDB::get_instance(id)) {
		return TrackEdit::get_key_height();
	}

	Ref<Font> font = get_font("font", "Label");
	return int(font->get_height() * 1.5);
}

Rect2 TrackEditAudio::get_key_rect(int p_index, float p_pixels_sec) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	Ref<AudioStream> stream = object->call("get_stream");

	if (!stream.is_valid()) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	bool play = get_animation()->track_get_key_value(get_track(), p_index);
	if (play) {
		float len = stream->get_length();

		if (len == 0) {
			//Ref<AudioStreamPreview> preview = AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);
			//len = preview->get_length();
		}

		if (get_animation()->track_get_key_count(get_track()) > p_index + 1) {
			len = MIN(len, get_animation()->track_get_key_time(get_track(), p_index + 1) - get_animation()->track_get_key_time(get_track(), p_index));
		}

		return Rect2(0, 0, len * p_pixels_sec, get_size().height);
	}
	else {
		Ref<Font> font = get_font("font", "Label");
		int fh = font->get_height() * 0.8;
		return Rect2(0, 0, fh, get_size().height);
	}
}

bool TrackEditAudio::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditAudio::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
		return;
	}

	Ref<AudioStream> stream = object->call("get_stream");

	if (!stream.is_valid()) {
		TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
		return;
	}

	bool play = get_animation()->track_get_key_value(get_track(), p_index);
	if (play) {
		float len = stream->get_length();

		//Ref<AudioStreamPreview> preview = AudioStreamPreviewGenerator::get_singleton()->generate_preview(stream);

		//float preview_len = preview->get_length();
		float preview_len = 0.0;

		if (len == 0) {
			len = preview_len;
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
			return;
		}

		Ref<Font> font = get_font("font", "Label");
		float fh = int(font->get_height() * 1.5);
		Rect2 rect = Rect2(from_x, (get_size().height - fh) / 2, to_x - from_x, fh);
		draw_rect(rect, Color(0.25, 0.25, 0.25));

		Vector<Vector2> lines;
		lines.resize((to_x - from_x + 1) * 2);
		//preview_len = preview->get_length();

		for (int i = from_x; i < to_x; i++) {
			float ofs = (i - pixel_begin) * preview_len / pixel_len;
			float ofs_n = ((i + 1) - pixel_begin) * preview_len / pixel_len;
			float max = 0.0;// preview->get_max(ofs, ofs_n) * 0.5 + 0.5;
			float min = 0.0; // preview->get_min(ofs, ofs_n) * 0.5 + 0.5;

			int idx = i - from_x;
			lines.write[idx * 2 + 0] = Vector2(i, rect.position.y + min * rect.size.y);
			lines.write[idx * 2 + 1] = Vector2(i, rect.position.y + max * rect.size.y);
		}

		Vector<Color> color;
		color.push_back(Color(0.75, 0.75, 0.75));

		VisualServer::get_singleton()->canvas_item_add_multiline(get_canvas_item(), lines, color);

		if (p_selected) {
			Color accent = Colors::ACCENT_COLOR;
			draw_rect(rect, accent, false);
		}
	}
	else {
		Ref<Font> font = get_font("font", "Label");
		int fh = font->get_height() * 0.8;
		Rect2 rect(Vector2(p_x, int(get_size().height - fh) / 2), Size2(fh, fh));

		Color color = get_color("font_color", "Label");
		draw_rect_clipped(rect, color);

		if (p_selected) {
			Color accent = Colors::ACCENT_COLOR;
			draw_rect_clipped(rect, accent, false);
		}
	}
}

void TrackEditAudio::set_node(Object* p_object) {
	id = p_object->get_instance_id();
}

void TrackEditAudio::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_gui_input", "event"), &TrackEdit::_gui_input);
}

TrackEditAudio::TrackEditAudio() {
	//AudioStreamPreviewGenerator::get_singleton()->connect("preview_updated", this, "_preview_changed");
}