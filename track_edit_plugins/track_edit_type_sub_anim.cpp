#include "track_edit_type_sub_anim.h"

#include "../track_editor/track_editor.h"
#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"
#include "scene/animation/animation_player.h"

#include "../editor_consts.h"

int TrackEditSubAnim::get_key_height() const {
	if (!ObjectDB::get_instance(id)) {
		return TrackEdit::get_key_height();
	}

	Ref<Font> font = get_font("font", "Label");
		return int(font->get_height() * 1.5);
}

Rect2 TrackEditSubAnim::get_key_rect(int p_index, float p_pixels_sec) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	AnimationPlayer* ap = Object::cast_to<AnimationPlayer>(object);

	if (!ap) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	String anim = get_animation()->track_get_key_value(get_track(), p_index);

	if (anim != "[stop]" && ap->has_animation(anim)) {
		float len = ap->get_animation(anim)->get_length();

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

bool TrackEditSubAnim::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditSubAnim::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
		return;
	}

	AnimationPlayer* ap = Object::cast_to<AnimationPlayer>(object);

	if (!ap) {
		TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
		return;
	}

	String anim = get_animation()->track_get_key_value(get_track(), p_index);

	if (anim != "[stop]" && ap->has_animation(anim)) {
		float len = ap->get_animation(anim)->get_length();

		if (get_animation()->track_get_key_count(get_track()) > p_index + 1) {
			len = MIN(len, get_animation()->track_get_key_time(get_track(), p_index + 1) - get_animation()->track_get_key_time(get_track(), p_index));
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

		if (to_x <= from_x) {
			return;
		}

		Ref<Font> font = get_font("font", "Label");
				int fh = font->get_height() * 1.5;

		Rect2 rect(from_x, int(get_size().height - fh) / 2, to_x - from_x, fh);

		Color color = get_color("font_color", "Label");
		Color bg = color;
		bg.r = 1 - color.r;
		bg.g = 1 - color.g;
		bg.b = 1 - color.b;
		draw_rect(rect, bg);

		Vector<Vector2> lines;
		Vector<Color> colorv;
		{
			Ref<Animation> animation = ap->get_animation(anim);

			for (int i = 0; i < animation->get_track_count(); i++) {
				float h = (rect.size.height - 2) / animation->get_track_count();

				int y = 2 + h * i + h / 2;

				for (int j = 0; j < animation->track_get_key_count(i); j++) {
					float ofs = animation->track_get_key_time(i, j);
					int x = p_x + ofs * p_pixels_sec + 2;

					if (x < from_x || x >= (to_x - 4)) {
						continue;
					}

					lines.push_back(Point2(x, y));
					lines.push_back(Point2(x + 1, y));
				}
			}

			colorv.push_back(color);
		}

		if (lines.size() > 2) {
			VisualServer::get_singleton()->canvas_item_add_multiline(get_canvas_item(), lines, colorv);
		}

		int limit = to_x - from_x - 4;
		if (limit > 0) {
			draw_string(font, Point2(from_x + 2, int(get_size().height - font->get_height()) / 2 + font->get_ascent()), anim, color);
		}

		if (p_selected) {
			Color accent = _EditorConsts::ACCENT_COLOR;
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
			Color accent = _EditorConsts::ACCENT_COLOR;
			draw_rect_clipped(rect, accent, false);
		}
	}
}

void TrackEditSubAnim::set_node(Object* p_object) {
	id = p_object->get_instance_id();
}