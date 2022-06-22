#include "track_edit_type_sprite_frame.h"

#include "../track_editor/track_editor.h"
#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"
#include "scene/2d/sprite.h"
#include "scene/2d/animated_sprite.h"
#include "scene/3d/sprite_3d.h"

#include "../editor_consts.h"

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

	Color accent = _EditorConsts::ACCENT_COLOR;
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