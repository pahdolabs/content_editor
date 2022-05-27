#include "track_edit_color.h"

#include "../track_editor/track_editor.h"
#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"

#include "../consts.h"

int TrackEditColor::get_key_height() const {
	Ref<Font> font = get_font("font", "Label");
	return font->get_height() * 0.8;
}

Rect2 TrackEditColor::get_key_rect(int p_index, float p_pixels_sec) {
	Ref<Font> font = get_font("font", "Label");
	int fh = font->get_height() * 0.8;
	return Rect2(-fh / 2, 0, fh, get_size().height);
}

bool TrackEditColor::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditColor::draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) {
	Ref<Font> font = get_font("font", "Label");
	int fh = (font->get_height() * 0.8);

	fh /= 3;

	int x_from = p_x + fh / 2 - 1;
	int x_to = p_next_x - fh / 2 + 1;
	x_from = MAX(x_from, p_clip_left);
	x_to = MIN(x_to, p_clip_right);

	int y_from = (get_size().height - fh) / 2;

	if (x_from > p_clip_right || x_to < p_clip_left) {
		return;
	}

	Vector<Color> color_samples;
	color_samples.push_back(get_animation()->track_get_key_value(get_track(), p_index));

	if (get_animation()->track_get_type(get_track()) == Animation::TYPE_VALUE) {
		if (get_animation()->track_get_interpolation_type(get_track()) != Animation::INTERPOLATION_NEAREST &&
			(get_animation()->value_track_get_update_mode(get_track()) == Animation::UPDATE_CONTINUOUS ||
				get_animation()->value_track_get_update_mode(get_track()) == Animation::UPDATE_CAPTURE) &&
			!Math::is_zero_approx(get_animation()->track_get_key_transition(get_track(), p_index))) {
			float start_time = get_animation()->track_get_key_time(get_track(), p_index);
			float end_time = get_animation()->track_get_key_time(get_track(), p_index + 1);

			Color color_next = get_animation()->value_track_interpolate(get_track(), end_time);

			if (!color_samples[0].is_equal_approx(color_next)) {
				color_samples.resize(1 + (x_to - x_from) / 64); // Make a color sample every 64 px.
				for (int i = 1; i < color_samples.size(); i++) {
					float j = i;
					color_samples.write[i] = get_animation()->value_track_interpolate(
						get_track(),
						Math::lerp(start_time, end_time, j / color_samples.size()));
				}
			}
			color_samples.push_back(color_next);
		}
		else {
			color_samples.push_back(color_samples[0]);
		}
	}
	else {
		color_samples.push_back(get_animation()->track_get_key_value(get_track(), p_index + 1));
	}

	for (int i = 0; i < color_samples.size() - 1; i++) {
		Vector<Vector2> points;
		points.push_back(Vector2(Math::lerp(x_from, x_to, float(i) / (color_samples.size() - 1)), y_from));
		points.push_back(Vector2(Math::lerp(x_from, x_to, float(i + 1) / (color_samples.size() - 1)), y_from));
		points.push_back(Vector2(Math::lerp(x_from, x_to, float(i + 1) / (color_samples.size() - 1)), y_from + fh));
		points.push_back(Vector2(Math::lerp(x_from, x_to, float(i) / (color_samples.size() - 1)), y_from + fh));
		
		Vector<Color> colors;
		colors.push_back(color_samples[i]);
		colors.push_back(color_samples[i + 1]);
		colors.push_back(color_samples[i + 1]);
		colors.push_back(color_samples[i]);
		
		draw_primitive(points, colors, Vector<Vector2>());
	}
}

void TrackEditColor::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	Color color = get_animation()->track_get_key_value(get_track(), p_index);

	Ref<Font> font = get_font("font", "Label");
	int fh = font->get_height() * 0.8;

	Rect2 rect(Vector2(p_x - fh / 2, int(get_size().height - fh) / 2), Size2(fh, fh));

	draw_rect_clipped(Rect2(rect.position, rect.size / 2), Color(0.4, 0.4, 0.4));
	draw_rect_clipped(Rect2(rect.position + rect.size / 2, rect.size / 2), Color(0.4, 0.4, 0.4));
	draw_rect_clipped(Rect2(rect.position + Vector2(rect.size.x / 2, 0), rect.size / 2), Color(0.6, 0.6, 0.6));
	draw_rect_clipped(Rect2(rect.position + Vector2(0, rect.size.y / 2), rect.size / 2), Color(0.6, 0.6, 0.6));
	draw_rect_clipped(rect, color);

	if (p_selected) {
		Color accent = Colors::ACCENT_COLOR;
		draw_rect_clipped(rect, accent, false);
	}
}