#include "track_editor_plugins.h"

#include "track_editor.h"
#include "timeline_edit.h"
#include <core/undo_redo.h>
#include <core/os/input_event.h>
#include <servers/audio/audio_stream.h>
#include <scene/2d/sprite.h>
#include <scene/2d/animated_sprite.h>
#include <scene/3d/sprite_3d.h>
#include <scene/animation/animation_player.h>

#include "consts.h"
#include "icons_cache.h"

/// BOOL ///
int TrackEditBool::get_key_height() const {
	Ref<Texture> checked = IconsCache::get_singleton()->get_icon("checked");
	return checked->get_height();
}

Rect2 TrackEditBool::get_key_rect(int p_index, float p_pixels_sec) {
	Ref<Texture> checked = IconsCache::get_singleton()->get_icon("checked");
	return Rect2(-checked->get_width() / 2, 0, checked->get_width(), get_size().height);
}

bool TrackEditBool::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditBool::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	bool checked = get_animation()->track_get_key_value(get_track(), p_index);
	Ref<Texture> icon = IconsCache::get_singleton()->get_icon(checked ? "checked" : "unchecked");

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
		Color color = Colors::accent_color;
		draw_rect_clipped(Rect2(ofs, icon->get_size()), color, false);
	}
}

/// COLOR ///

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
		Color accent = Colors::accent_color;
		draw_rect_clipped(rect, accent, false);
	}
}

/// AUDIO ///

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
			Color accent = Colors::accent_color;
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
			Color accent = Colors::accent_color;
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

/// SPRITE FRAME / FRAME_COORDS ///

int TrackEditSpriteFrame::get_key_height() const {
	if (!ObjectDB::get_instance(id)) {
		return TrackEdit::get_key_height();
	}

	Ref<Font> font = get_font("font", "Label");
		return int(font->get_height() * 2);
}

Rect2 TrackEditSpriteFrame::get_key_rect(int p_index, float p_pixels_sec) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	Size2 size;

	if (Object::cast_to<Sprite>(object) || Object::cast_to<Sprite3D>(object)) {
		Ref<Texture> texture = object->call("get_texture");
		if (!texture.is_valid()) {
			return TrackEdit::get_key_rect(p_index, p_pixels_sec);
		}

		size = texture->get_size();

		if (bool(object->call("is_region"))) {
			size = Rect2(object->call("get_region_rect")).size;
		}

		int hframes = object->call("get_hframes");
		int vframes = object->call("get_vframes");

		if (hframes > 1) {
			size.x /= hframes;
		}
		if (vframes > 1) {
			size.y /= vframes;
		}
	}
	else if (Object::cast_to<AnimatedSprite>(object) || Object::cast_to<AnimatedSprite3D>(object)) {
		Ref<SpriteFrames> sf = object->call("get_sprite_frames");
		if (sf.is_null()) {
			return TrackEdit::get_key_rect(p_index, p_pixels_sec);
		}

		List<StringName> animations;
		sf->get_animation_list(&animations);

		int frame = get_animation()->track_get_key_value(get_track(), p_index);
		String animation;
		if (animations.size() == 1) {
			animation = animations.front()->get();
		}
		else {
			// Go through other track to find if animation is set
			String animation_path = get_animation()->track_get_path(get_track());
			animation_path = animation_path.replace(":frame", ":animation");
			int animation_track = get_animation()->find_track(animation_path);
			float track_time = get_animation()->track_get_key_time(get_track(), p_index);
			int animaiton_index = get_animation()->track_find_key(animation_track, track_time);
			animation = get_animation()->track_get_key_value(animation_track, animaiton_index);
		}

		Ref<Texture> texture = sf->get_frame(animation, frame);
		if (!texture.is_valid()) {
			return TrackEdit::get_key_rect(p_index, p_pixels_sec);
		}

		size = texture->get_size();
	}

	size = size.floor();

	Ref<Font> font = get_font("font", "Label");
		int height = int(font->get_height() * 2);
	int width = height * size.width / size.height;

	return Rect2(0, 0, width, get_size().height);
}

bool TrackEditSpriteFrame::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditSpriteFrame::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
		return;
	}

	Ref<Texture> texture;
	Rect2 region;

	if (Object::cast_to<Sprite>(object) || Object::cast_to<Sprite3D>(object)) {
		texture = object->call("get_texture");
		if (!texture.is_valid()) {
			TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
			return;
		}

		int hframes = object->call("get_hframes");
		int vframes = object->call("get_vframes");

		Vector2 coords;
		if (is_coords) {
			coords = get_animation()->track_get_key_value(get_track(), p_index);
		}
		else {
			int frame = get_animation()->track_get_key_value(get_track(), p_index);
			coords.x = frame % hframes;
			coords.y = frame / hframes;
		}

		region.size = texture->get_size();

		if (bool(object->call("is_region"))) {
			region = Rect2(object->call("get_region_rect"));
		}

		if (hframes > 1) {
			region.size.x /= hframes;
		}
		if (vframes > 1) {
			region.size.y /= vframes;
		}

		region.position.x += region.size.x * coords.x;
		region.position.y += region.size.y * coords.y;

	}
	else if (Object::cast_to<AnimatedSprite>(object) || Object::cast_to<AnimatedSprite3D>(object)) {
		Ref<SpriteFrames> sf = object->call("get_sprite_frames");
		if (sf.is_null()) {
			TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
			return;
		}

		List<StringName> animations;
		sf->get_animation_list(&animations);

		int frame = get_animation()->track_get_key_value(get_track(), p_index);
		String animation;
		if (animations.size() == 1) {
			animation = animations.front()->get();
		}
		else {
			// Go through other track to find if animation is set
			String animation_path = get_animation()->track_get_path(get_track());
			animation_path = animation_path.replace(":frame", ":animation");
			int animation_track = get_animation()->find_track(animation_path);
			float track_time = get_animation()->track_get_key_time(get_track(), p_index);
			int animaiton_index = get_animation()->track_find_key(animation_track, track_time);
			animation = get_animation()->track_get_key_value(animation_track, animaiton_index);
		}

		texture = sf->get_frame(animation, frame);
		if (!texture.is_valid()) {
			TrackEdit::draw_key(p_index, p_pixels_sec, p_x, p_selected, p_clip_left, p_clip_right);
			return;
		}

		region.size = texture->get_size();
	}

	Ref<Font> font = get_font("font", "Label");
		int height = int(font->get_height() * 2);

	int width = height * region.size.width / region.size.height;

	Rect2 rect(p_x, int(get_size().height - height) / 2, width, height);

	if (rect.position.x + rect.size.x < p_clip_left) {
		return;
	}

	if (rect.position.x > p_clip_right) {
		return;
	}

	Color accent = Colors::accent_color;
	Color bg = accent;
	bg.a = 0.15;

	draw_rect_clipped(rect, bg);

	if (texture != nullptr) {
		draw_texture_region_clipped(texture, rect, region);
	}

	if (p_selected) {
		draw_rect_clipped(rect, accent, false);
	}
}

void TrackEditSpriteFrame::set_node(Object* p_object) {
	id = p_object->get_instance_id();
}

void TrackEditSpriteFrame::set_as_coords() {
	is_coords = true;
}

/// SUB ANIMATION ///

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
			Color accent = Colors::accent_color;
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
			Color accent = Colors::accent_color;
			draw_rect_clipped(rect, accent, false);
		}
	}
}

void TrackEditSubAnim::set_node(Object* p_object) {
	id = p_object->get_instance_id();
}

//// VOLUME DB ////

int TrackEditVolumeDB::get_key_height() const {
	Ref<Texture> volume_texture = IconsCache::get_singleton()->get_icon("ColorTrackVu");
	return volume_texture->get_height() * 1.2;
}

void TrackEditVolumeDB::draw_bg(int p_clip_left, int p_clip_right) {
	Ref<Texture> volume_texture = IconsCache::get_singleton()->get_icon("ColorTrackVu");
	int tex_h = volume_texture != nullptr ? volume_texture->get_height() : 0;

	int y_from = (get_size().height - tex_h) / 2;
	int y_size = tex_h;

	Color color(1, 1, 1, 0.3);
	if (volume_texture != nullptr) {
		draw_texture_rect(volume_texture, Rect2(p_clip_left, y_from, p_clip_right - p_clip_left, y_from + y_size), false, color);
	}
}

void TrackEditVolumeDB::draw_fg(int p_clip_left, int p_clip_right) {
	Ref<Texture> volume_texture = IconsCache::get_singleton()->get_icon("ColorTrackVu");
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

	Ref<Texture> volume_texture = IconsCache::get_singleton()->get_icon("ColorTrackVu");
	int tex_h = volume_texture->get_height();

	int y_from = (get_size().height - tex_h) / 2;

	Color color = get_color("font_color", "Label");
	color.a *= 0.7;

	draw_line(Point2(from_x, y_from + h * tex_h), Point2(to_x, y_from + h_n * tex_h), color, 2);
}

////////////////////////

/// AUDIO ///

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

	Color cut_color = Colors::accent_color;
	cut_color.a = 0.7;
	if (start_ofs > 0 && pixel_begin > p_clip_left) {
		draw_rect(Rect2(pixel_begin, rect.position.y, 1, rect.size.y), cut_color);
	}
	if (end_ofs > 0 && pixel_end < p_clip_right) {
		draw_rect(Rect2(pixel_end, rect.position.y, 1, rect.size.y), cut_color);
	}

	if (p_selected) {
		Color accent = Colors::accent_color;
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

////////////////////
/// SUB ANIMATION ///

int TrackEditTypeAnimation::get_key_height() const {
	if (!ObjectDB::get_instance(id)) {
		return TrackEdit::get_key_height();
	}

	Ref<Font> font = get_font("font", "Label");
		return int(font->get_height() * 1.5);
}

Rect2 TrackEditTypeAnimation::get_key_rect(int p_index, float p_pixels_sec) {
	Object* object = ObjectDB::get_instance(id);

	if (!object) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	AnimationPlayer* ap = Object::cast_to<AnimationPlayer>(object);

	if (!ap) {
		return TrackEdit::get_key_rect(p_index, p_pixels_sec);
	}

	String anim = get_animation()->animation_track_get_key_animation(get_track(), p_index);

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

bool TrackEditTypeAnimation::is_key_selectable_by_distance() const {
	return false;
}

void TrackEditTypeAnimation::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
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

	String anim = get_animation()->animation_track_get_key_animation(get_track(), p_index);

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
			Color accent = Colors::accent_color;
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
			Color accent = Colors::accent_color;
			draw_rect_clipped(rect, accent, false);
		}
	}
}

void TrackEditTypeAnimation::set_node(Object* p_object) {
	id = p_object->get_instance_id();
}

TrackEditTypeAnimation::TrackEditTypeAnimation() {
}

/////////
TrackEdit* TrackEditDefaultPlugin::create_value_track_edit(Object* p_object, Variant::Type p_type, const String& p_property, PropertyHint p_hint, const String& p_hint_string, int p_usage) {
	if (p_property == "playing" && (p_object->is_class("AudioStreamPlayer") || p_object->is_class("AudioStreamPlayer2D") || p_object->is_class("AudioStreamPlayer3D"))) {
		TrackEditAudio* audio = memnew(TrackEditAudio);
		audio->set_node(p_object);
		return audio;
	}

	if (p_property == "frame" && (p_object->is_class("Sprite2D") || p_object->is_class("Sprite3D") || p_object->is_class("AnimatedSprite2D") || p_object->is_class("AnimatedSprite3D"))) {
		TrackEditSpriteFrame* sprite = memnew(TrackEditSpriteFrame);
		sprite->set_node(p_object);
		return sprite;
	}

	if (p_property == "frame_coords" && (p_object->is_class("Sprite2D") || p_object->is_class("Sprite3D"))) {
		TrackEditSpriteFrame* sprite = memnew(TrackEditSpriteFrame);
		sprite->set_as_coords();
		sprite->set_node(p_object);
		return sprite;
	}

	if (p_property == "current_animation" && (p_object->is_class("AnimationPlayer"))) {
		TrackEditSubAnim* player = memnew(TrackEditSubAnim);
		player->set_node(p_object);
		return player;
	}

	if (p_property == "volume_db") {
		TrackEditVolumeDB* vu = memnew(TrackEditVolumeDB);
		return vu;
	}

	if (p_type == Variant::BOOL) {
		return memnew(TrackEditBool);
	}
	if (p_type == Variant::COLOR) {
		return memnew(TrackEditColor);
	}

	return nullptr;
}

TrackEdit* TrackEditDefaultPlugin::create_audio_track_edit() {
	return memnew(TrackEditTypeAudio);
}

TrackEdit* TrackEditDefaultPlugin::create_animation_track_edit(Object* p_object) {
	TrackEditTypeAnimation* an = memnew(TrackEditTypeAnimation);
	an->set_node(p_object);
	return an;
}
