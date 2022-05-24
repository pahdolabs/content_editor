#include "track_edit.h"

#include <core/undo_redo.h>
#include <scene/animation/animation_player.h>
#include <scene/main/viewport.h>

#include "player_editor_control.h"
#include "timeline_edit.h"
#include "track_editor.h"

void TrackEdit::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		if (animation.is_null()) {
			return;
		}
		ERR_FAIL_INDEX(track, animation->get_track_count());

		type_icon = _get_key_type_icon();
		selected_icon = get_icon("KeySelected", "EditorIcons");
	} break;

	case NOTIFICATION_DRAW: {
		if (animation.is_null()) {
			return;
		}
		ERR_FAIL_INDEX(track, animation->get_track_count());

		int limit = timeline->get_name_limit();

		if (track % 2 == 1) {
			// Draw a background over odd lines to make long lists of tracks easier to read.
			draw_rect(Rect2(Point2(1 * 1.0, 0), get_size() - Size2(1 * 1.0, 0)), Color(0.5, 0.5, 0.5, 0.05));
		}

		if (hovered) {
			// Draw hover feedback.
			draw_rect(Rect2(Point2(1 * 1.0, 0), get_size() - Size2(1 * 1.0, 0)), Color(0.5, 0.5, 0.5, 0.1));
		}

		if (has_focus()) {
			Color accent = get_color("accent_color", "Editor");
			accent.a *= 0.7;
			// Offside so the horizontal sides aren't cutoff.
			draw_style_box(get_stylebox("Focus", "EditorStyles"), Rect2(Point2(1 * 1.0, 0), get_size() - Size2(1 * 1.0, 0)));
		}

		Ref<Font> font = get_font("font", "Label");
		Color color = get_color("font_color", "Label");
		int hsep = get_constant("hseparation", "ItemList");
		Color linecolor = color;
		linecolor.a = 0.2;

		// NAMES AND ICONS //

		{
			Ref<Texture> check = animation->track_is_enabled(track) ? get_icon("checked", "CheckBox") : get_icon("unchecked", "CheckBox");

			int ofs = in_group ? check->get_width() : 0; // Not the best reference for margin but..

			check_rect = Rect2(Point2(ofs, int(get_size().height - check->get_height()) / 2), check->get_size());
			draw_texture(check, check_rect.position);
			ofs += check->get_width() + hsep;

			Ref<Texture> type_icon = _get_key_type_icon();
			draw_texture(type_icon, Point2(ofs, int(get_size().height - type_icon->get_height()) / 2));
			ofs += type_icon->get_width() + hsep;

			NodePath path = animation->track_get_path(track);
			Node* node = nullptr;
			if (root && root->has_node(path)) {
				node = root->get_node(path);
			}

			String text;
			Color text_color = color;
			if (node) {
				text_color = get_color("accent_color", "Editor");
			}

			if (in_group) {
				if (animation->track_get_type(track) == Animation::TYPE_METHOD) {
					text = TTR("Functions:");
				}
				else if (animation->track_get_type(track) == Animation::TYPE_AUDIO) {
					text = TTR("Audio Clips:");
				}
				else if (animation->track_get_type(track) == Animation::TYPE_ANIMATION) {
					text = TTR("Anim Clips:");
				}
				else {
					text += path.get_concatenated_subnames();
				}
				text_color.a *= 0.7;
			}
			else if (node) {
				Ref<Texture> icon;// = EditorNode::get_singleton()->get_object_icon(node, "Node");

				//draw_texture(icon, Point2(ofs, int(get_size().height - icon->get_height()) / 2));
				//icon_cache = icon;

				text = String() + node->get_name() + ":" + path.get_concatenated_subnames();
				ofs += hsep;
				ofs += icon->get_width();

			}
			else {
				icon_cache = type_icon;

				text = path;
			}

			path_cache = text;

			path_rect = Rect2(ofs, 0, limit - ofs - hsep, get_size().height);

			Vector2 string_pos = Point2(ofs, (get_size().height - font->get_height()) / 2 + font->get_ascent());
			string_pos = string_pos.floor();
			draw_string(font, string_pos, text, text_color);

			draw_line(Point2(limit, 0), Point2(limit, get_size().height), linecolor, Math::round(1.0));
		}

		// KEYFRAMES //

		draw_bg(limit, get_size().width - timeline->get_buttons_width());

		{
			float scale = timeline->get_zoom_scale();
			int limit_end = get_size().width - timeline->get_buttons_width();

			for (int i = 0; i < animation->track_get_key_count(track); i++) {
				float offset = animation->track_get_key_time(track, i) - timeline->get_value();
				if (editor->is_key_selected(track, i) && editor->is_moving_selection()) {
					offset = editor->snap_time(offset + editor->get_moving_selection_offset(), true);
				}
				offset = offset * scale + limit;
				if (i < animation->track_get_key_count(track) - 1) {
					float offset_n = animation->track_get_key_time(track, i + 1) - timeline->get_value();
					if (editor->is_key_selected(track, i + 1) && editor->is_moving_selection()) {
						offset_n = editor->snap_time(offset_n + editor->get_moving_selection_offset());
					}
					offset_n = offset_n * scale + limit;

					draw_key_link(i, scale, int(offset), int(offset_n), limit, limit_end);
				}

				draw_key(i, scale, int(offset), editor->is_key_selected(track, i), limit, limit_end);
			}
		}

		draw_fg(limit, get_size().width - timeline->get_buttons_width());

		// BUTTONS //

		{
			Ref<Texture> wrap_icon[2] = {
				get_icon("InterpWrapClamp", "EditorIcons"),
				get_icon("InterpWrapLoop", "EditorIcons"),
			};

			Ref<Texture> interp_icon[3] = {
				get_icon("InterpRaw", "EditorIcons"),
				get_icon("InterpLinear", "EditorIcons"),
				get_icon("InterpCubic", "EditorIcons")
			};
			Ref<Texture> cont_icon[4] = {
				get_icon("TrackContinuous", "EditorIcons"),
				get_icon("TrackDiscrete", "EditorIcons"),
				get_icon("TrackTrigger", "EditorIcons"),
				get_icon("TrackCapture", "EditorIcons")
			};

			int ofs = get_size().width - timeline->get_buttons_width();

			Ref<Texture> down_icon = get_icon("select_arrow", "Tree");

			draw_line(Point2(ofs, 0), Point2(ofs, get_size().height), linecolor, Math::round(1.0));

			ofs += hsep;
			{
				// Callmode.

				Animation::UpdateMode update_mode;

				if (animation->track_get_type(track) == Animation::TYPE_VALUE) {
					update_mode = animation->value_track_get_update_mode(track);
				}
				else {
					update_mode = Animation::UPDATE_CONTINUOUS;
				}

				Ref<Texture> update_icon = cont_icon[update_mode];

				update_mode_rect.position.x = ofs;
				update_mode_rect.position.y = int(get_size().height - update_icon->get_height()) / 2;
				update_mode_rect.size = update_icon->get_size();

				if (animation->track_get_type(track) == Animation::TYPE_VALUE) {
					draw_texture(update_icon, update_mode_rect.position);
				}
				// Make it easier to click.
				update_mode_rect.position.y = 0;
				update_mode_rect.size.y = get_size().height;

				ofs += update_icon->get_width() + hsep / 2;
				update_mode_rect.size.x += hsep / 2;

				if (animation->track_get_type(track) == Animation::TYPE_VALUE) {
					draw_texture(down_icon, Vector2(ofs, int(get_size().height - down_icon->get_height()) / 2));
					update_mode_rect.size.x += down_icon->get_width();
				}
				else if (animation->track_get_type(track) == Animation::TYPE_BEZIER) {
					Ref<Texture> bezier_icon = get_icon("EditBezier", "EditorIcons");
					update_mode_rect.size.x += down_icon->get_width();

					update_mode_rect = Rect2();
				}
				else {
					update_mode_rect = Rect2();
				}

				ofs += down_icon->get_width();
				draw_line(Point2(ofs + hsep * 0.5, 0), Point2(ofs + hsep * 0.5, get_size().height), linecolor, Math::round(1.0));
				ofs += hsep;
			}

			{
				// Interp.

				Animation::InterpolationType interp_mode = animation->track_get_interpolation_type(track);

				Ref<Texture> icon = interp_icon[interp_mode];

				interp_mode_rect.position.x = ofs;
				interp_mode_rect.position.y = int(get_size().height - icon->get_height()) / 2;
				interp_mode_rect.size = icon->get_size();

				if ((animation->track_get_type(track) == Animation::TYPE_VALUE)) {
					draw_texture(icon, interp_mode_rect.position);
				}
				// Make it easier to click.
				interp_mode_rect.position.y = 0;
				interp_mode_rect.size.y = get_size().height;

				ofs += icon->get_width() + hsep / 2;
				interp_mode_rect.size.x += hsep / 2;

				if ((animation->track_get_type(track) == Animation::TYPE_VALUE)) {
					draw_texture(down_icon, Vector2(ofs, int(get_size().height - down_icon->get_height()) / 2));
					interp_mode_rect.size.x += down_icon->get_width();
				}
				else {
					interp_mode_rect = Rect2();
				}

				ofs += down_icon->get_width();
				draw_line(Point2(ofs + hsep * 0.5, 0), Point2(ofs + hsep * 0.5, get_size().height), linecolor, Math::round(1.0));
				ofs += hsep;
			}

			{
				// Loop.

				bool loop_wrap = animation->track_get_interpolation_loop_wrap(track);

				Ref<Texture> icon = wrap_icon[loop_wrap ? 1 : 0];

				loop_wrap_rect.position.x = ofs;
				loop_wrap_rect.position.y = int(get_size().height - icon->get_height()) / 2;
				loop_wrap_rect.size = icon->get_size();

				if ((animation->track_get_type(track) == Animation::TYPE_VALUE)) {
					draw_texture(icon, loop_wrap_rect.position);
				}

				loop_wrap_rect.position.y = 0;
				loop_wrap_rect.size.y = get_size().height;

				ofs += icon->get_width() + hsep / 2;
				loop_wrap_rect.size.x += hsep / 2;

				if ((animation->track_get_type(track) == Animation::TYPE_VALUE)) {
					draw_texture(down_icon, Vector2(ofs, int(get_size().height - down_icon->get_height()) / 2));
					loop_wrap_rect.size.x += down_icon->get_width();
				}
				else {
					loop_wrap_rect = Rect2();
				}

				ofs += down_icon->get_width();
				draw_line(Point2(ofs + hsep * 0.5, 0), Point2(ofs + hsep * 0.5, get_size().height), linecolor, Math::round(1.0));
				ofs += hsep;
			}

			{
				// Erase.

				Ref<Texture> icon = get_icon("Remove", "EditorIcons");

				remove_rect.position.x = ofs + ((get_size().width - ofs) - icon->get_width());
				remove_rect.position.y = int(get_size().height - icon->get_height()) / 2;
				remove_rect.size = icon->get_size();

				draw_texture(icon, remove_rect.position);
			}
		}

		if (in_group) {
			draw_line(Vector2(timeline->get_name_limit(), get_size().height), get_size(), linecolor, Math::round(1.0));
		}
		else {
			draw_line(Vector2(0, get_size().height), get_size(), linecolor, Math::round(1.0));
		}

		if (dropping_at != 0) {
			Color drop_color = get_color("accent_color", "Editor");
			if (dropping_at < 0) {
				draw_line(Vector2(0, 0), Vector2(get_size().width, 0), drop_color, Math::round(1.0));
			}
			else {
				draw_line(Vector2(0, get_size().height), get_size(), drop_color, Math::round(1.0));
			}
		}
	} break;

	case NOTIFICATION_MOUSE_ENTER:
		hovered = true;
		update();
		break;
	case NOTIFICATION_MOUSE_EXIT:
		hovered = false;
		// When the mouse cursor exits the track, we're no longer hovering any keyframe.
		hovering_key_idx = -1;
		update();
	case NOTIFICATION_DRAG_END: {
		cancel_drop();
	} break;
	}
}

int TrackEdit::get_key_height() const {
	if (!animation.is_valid()) {
		return 0;
	}

	return type_icon->get_height();
}

Rect2 TrackEdit::get_key_rect(int p_index, float p_pixels_sec) {
	if (!animation.is_valid()) {
		return Rect2();
	}
	Rect2 rect = Rect2(-type_icon->get_width() / 2, 0, type_icon->get_width(), get_size().height);

	// Make it a big easier to click.
	rect.position.x -= rect.size.x * 0.5;
	rect.size.x *= 2;
	return rect;
}

bool TrackEdit::is_key_selectable_by_distance() const {
	return true;
}

void TrackEdit::draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) {
	if (p_next_x < p_clip_left) {
		return;
	}
	if (p_x > p_clip_right) {
		return;
	}

	Variant current = animation->track_get_key_value(get_track(), p_index);
	Variant next = animation->track_get_key_value(get_track(), p_index + 1);
	if (current != next) {
		return;
	}

	Color color = get_color("font_color", "Label");
	color.a = 0.5;

	int from_x = MAX(p_x, p_clip_left);
	int to_x = MIN(p_next_x, p_clip_right);

	draw_line(Point2(from_x + 1, get_size().height / 2), Point2(to_x, get_size().height / 2), color, Math::round(2 * 1.0));
}

void TrackEdit::draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) {
	if (!animation.is_valid()) {
		return;
	}

	if (p_x < p_clip_left || p_x > p_clip_right) {
		return;
	}

	Ref<Texture> icon_to_draw = p_selected ? selected_icon : type_icon;

	if (animation->track_get_type(track) == Animation::TYPE_VALUE && !Math::is_equal_approx(animation->track_get_key_transition(track, p_index), real_t(1.0))) {
		// Use a different icon for keys with non-linear easing.
		icon_to_draw = get_icon(p_selected ? "KeyEasedSelected" : "KeyValueEased", "EditorIcons");
	}

	// Override type icon for invalid value keys, unless selected.
	if (!p_selected && animation->track_get_type(track) == Animation::TYPE_VALUE) {
		const Variant& v = animation->track_get_key_value(track, p_index);
		Variant::Type valid_type = Variant::NIL;
		if (!_is_value_key_valid(v, valid_type)) {
			icon_to_draw = get_icon("KeyInvalid", "EditorIcons");
		}
	}

	Vector2 ofs(p_x - icon_to_draw->get_width() / 2, int(get_size().height - icon_to_draw->get_height()) / 2);

	if (animation->track_get_type(track) == Animation::TYPE_METHOD) {
		Ref<Font> font = get_font("font", "Label");
		Color color = get_color("font_color", "Label");
		color.a = 0.5;

		Dictionary d = animation->track_get_key_value(track, p_index);
		String text;

		if (d.has("method")) {
			text += String(d["method"]);
		}
		text += "(";
		Vector<Variant> args;
		if (d.has("args")) {
			args = d["args"];
		}
		for (int i = 0; i < args.size(); i++) {
			if (i > 0) {
				text += ", ";
			}
			text += String(args[i]);
		}
		text += ")";

		int limit = MAX(0, p_clip_right - p_x - icon_to_draw->get_width());
		if (limit > 0) {
			draw_string(font, Vector2(p_x + icon_to_draw->get_width(), int(get_size().height - font->get_height()) / 2 + font->get_ascent()), text, color);
		}
	}

	// Use a different color for the currently hovered key.
	// The color multiplier is chosen to work with both dark and light editor themes,
	// and on both unselected and selected key icons.
	draw_texture(
		icon_to_draw,
		ofs,
		p_index == hovering_key_idx ? get_color("folder_icon_modulate", "FileDialog") : Color(1, 1, 1));
}

// Helper.
void TrackEdit::draw_rect_clipped(const Rect2& p_rect, const Color& p_color, bool p_filled) {
	int clip_left = timeline->get_name_limit();
	int clip_right = get_size().width - timeline->get_buttons_width();

	if (p_rect.position.x > clip_right) {
		return;
	}
	if (p_rect.position.x + p_rect.size.x < clip_left) {
		return;
	}
	Rect2 clip = Rect2(clip_left, 0, clip_right - clip_left, get_size().height);
	draw_rect(clip.clip(p_rect), p_color, p_filled);
}

void TrackEdit::draw_bg(int p_clip_left, int p_clip_right) {
}

void TrackEdit::draw_fg(int p_clip_left, int p_clip_right) {
}

void TrackEdit::draw_texture_region_clipped(const Ref<Texture>& p_texture, const Rect2& p_rect, const Rect2& p_region) {
	int clip_left = timeline->get_name_limit();
	int clip_right = get_size().width - timeline->get_buttons_width();

	// Clip left and right.
	if (clip_left > p_rect.position.x + p_rect.size.x) {
		return;
	}
	if (clip_right < p_rect.position.x) {
		return;
	}

	Rect2 rect = p_rect;
	Rect2 region = p_region;

	if (clip_left > rect.position.x) {
		int rect_pixels = (clip_left - rect.position.x);
		int region_pixels = rect_pixels * region.size.x / rect.size.x;

		rect.position.x += rect_pixels;
		rect.size.x -= rect_pixels;

		region.position.x += region_pixels;
		region.size.x -= region_pixels;
	}

	if (clip_right < rect.position.x + rect.size.x) {
		int rect_pixels = rect.position.x + rect.size.x - clip_right;
		int region_pixels = rect_pixels * region.size.x / rect.size.x;

		rect.size.x -= rect_pixels;
		region.size.x -= region_pixels;
	}

	draw_texture_rect_region(p_texture, rect, region);
}

int TrackEdit::get_track() const {
	return track;
}

Ref<Animation> TrackEdit::get_animation() const {
	return animation;
}

void TrackEdit::set_animation_and_track(const Ref<Animation>& p_animation, int p_track) {
	animation = p_animation;
	track = p_track;
	update();

	ERR_FAIL_INDEX(track, animation->get_track_count());

	node_path = animation->track_get_path(p_track);
	type_icon = _get_key_type_icon();
	selected_icon = get_icon("KeySelected", "EditorIcons");
}

NodePath TrackEdit::get_path() const {
	return node_path;
}

Size2 TrackEdit::get_minimum_size() const {
	Ref<Texture> texture = get_icon("Object", "EditorIcons");
	Ref<Font> font = get_font("font", "Label");
	int separation = get_constant("vseparation", "ItemList");

	int max_h = MAX(texture->get_height(), font->get_height());
	max_h = MAX(max_h, get_key_height());

	return Vector2(1, max_h + separation);
}

void TrackEdit::set_undo_redo(UndoRedo* p_undo_redo) {
	undo_redo = p_undo_redo;
}

void TrackEdit::set_timeline(TimelineEdit* p_timeline) {
	timeline = p_timeline;
	timeline->set_track_edit(this);
	timeline->connect("zoom_changed", this, "_zoom_changed");
	timeline->connect("name_limit_changed", this, "_zoom_changed");
}

void TrackEdit::set_editor(TrackEditor* p_editor) {
	editor = p_editor;
}

void TrackEdit::_play_position_draw() {
	if (!animation.is_valid() || play_position_pos < 0) {
		return;
	}

	float scale = timeline->get_zoom_scale();
	int h = get_size().height;

	int px = (-timeline->get_value() + play_position_pos) * scale + timeline->get_name_limit();

	if (px >= timeline->get_name_limit() && px < (get_size().width - timeline->get_buttons_width())) {
		Color color = get_color("accent_color", "Editor");
		play_position->draw_line(Point2(px, 0), Point2(px, h), color, Math::round(2 * 1.0));
	}
}

void TrackEdit::set_play_position(float p_pos) {
	play_position_pos = p_pos;
	play_position->update();
}

void TrackEdit::update_play_position() {
	play_position->update();
}

void TrackEdit::set_root(Node* p_root) {
	root = p_root;
}

void TrackEdit::_zoom_changed() {
	update();
	play_position->update();
}

void TrackEdit::_path_submitted(const String& p_text) {
	undo_redo->create_action(TTR("Change Track Path"));
	undo_redo->add_do_method(animation.ptr(), "track_set_path", track, p_text);
	undo_redo->add_undo_method(animation.ptr(), "track_set_path", track, animation->track_get_path(track));
	undo_redo->commit_action();
	path_popup->hide();
}

bool TrackEdit::_is_value_key_valid(const Variant& p_key_value, Variant::Type& r_valid_type) const {
	if (root == nullptr) {
		return false;
	}

	RES res;
	Vector<StringName> leftover_path;
	Node* node = root->get_node_and_resource(animation->track_get_path(track), res, leftover_path);

	Object* obj = nullptr;
	if (res.is_valid()) {
		obj = res.ptr();
	}
	else if (node) {
		obj = node;
	}

	bool prop_exists = false;
	if (obj) {
		r_valid_type = obj->get_static_property_type_indexed(leftover_path, &prop_exists);
	}

	return (!prop_exists || Variant::can_convert(p_key_value.get_type(), r_valid_type));
}

Ref<Texture> TrackEdit::_get_key_type_icon() const {
	Ref<Texture> type_icons[9] = {
		get_icon("KeyValue", "EditorIcons"),
		get_icon("KeyTrackPosition", "EditorIcons"),
		get_icon("KeyTrackRotation", "EditorIcons"),
		get_icon("KeyTrackScale", "EditorIcons"),
		get_icon("KeyTrackBlendShape", "EditorIcons"),
		get_icon("KeyCall", "EditorIcons"),
		get_icon("KeyBezier", "EditorIcons"),
		get_icon("KeyAudio", "EditorIcons"),
		get_icon("KeyAnimation", "EditorIcons")
	};
	return type_icons[animation->track_get_type(track)];
}

String TrackEdit::get_tooltip(const Point2& p_pos) const {
	if (check_rect.has_point(p_pos)) {
		return TTR("Toggle this track on/off.");
	}

	// Don't overlap track keys if they start at 0.
	if (path_rect.has_point(p_pos + Size2(type_icon->get_width(), 0))) {
		return animation->track_get_path(track);
	}

	if (update_mode_rect.has_point(p_pos)) {
		return TTR("Update Mode (How this property is set)");
	}

	if (interp_mode_rect.has_point(p_pos)) {
		return TTR("Interpolation Mode");
	}

	if (loop_wrap_rect.has_point(p_pos)) {
		return TTR("Loop Wrap Mode (Interpolate end with beginning on loop)");
	}

	if (remove_rect.has_point(p_pos)) {
		return TTR("Remove this track.");
	}

	int limit = timeline->get_name_limit();
	int limit_end = get_size().width - timeline->get_buttons_width();
	// Left Border including space occupied by keyframes on t=0.
	int limit_start_hitbox = limit - type_icon->get_width();

	if (p_pos.x >= limit_start_hitbox && p_pos.x <= limit_end) {
		int key_idx = -1;
		float key_distance = 1e20;

		// Select should happen in the opposite order of drawing for more accurate overlap select.
		for (int i = animation->track_get_key_count(track) - 1; i >= 0; i--) {
			Rect2 rect = const_cast<TrackEdit*>(this)->get_key_rect(i, timeline->get_zoom_scale());
			float offset = animation->track_get_key_time(track, i) - timeline->get_value();
			offset = offset * timeline->get_zoom_scale() + limit;
			rect.position.x += offset;

			if (rect.has_point(p_pos)) {
				if (const_cast<TrackEdit*>(this)->is_key_selectable_by_distance()) {
					float distance = ABS(offset - p_pos.x);
					if (key_idx == -1 || distance < key_distance) {
						key_idx = i;
						key_distance = distance;
					}
				}
				else {
					// First one does it.
					break;
				}
			}
		}

		if (key_idx != -1) {
			String text = "Time (s): " + rtos(animation->track_get_key_time(track, key_idx)) + "\n";
			switch (animation->track_get_type(track)) {
			case Animation::TYPE_VALUE: {
				const Variant& v = animation->track_get_key_value(track, key_idx);
				text += "Type: " + Variant::get_type_name(v.get_type()) + "\n";
				Variant::Type valid_type = Variant::NIL;
				if (!_is_value_key_valid(v, valid_type)) {
					text += "Value: " + String(v) + "  (Invalid, expected type: " + Variant::get_type_name(valid_type) + ")\n";
				}
				else {
					text += "Value: " + String(v) + "\n";
				}
				text += "Easing: " + rtos(animation->track_get_key_transition(track, key_idx));

			} break;
			case Animation::TYPE_METHOD: {
				Dictionary d = animation->track_get_key_value(track, key_idx);
				if (d.has("method")) {
					text += String(d["method"]);
				}
				text += "(";
				Vector<Variant> args;
				if (d.has("args")) {
					args = d["args"];
				}
				for (int i = 0; i < args.size(); i++) {
					if (i > 0) {
						text += ", ";
					}
					text += String(args[i]);
				}
				text += ")\n";

			} break;
			case Animation::TYPE_AUDIO: {
				String stream_name = "null";
				RES stream = animation->audio_track_get_key_stream(track, key_idx);
				if (stream.is_valid()) {
					if (stream->get_path().is_resource_file()) {
						stream_name = stream->get_path().get_file();
					}
					else if (!stream->get_name().empty()) {
						stream_name = stream->get_name();
					}
					else {
						stream_name = stream->get_class();
					}
				}

				text += "Stream: " + stream_name + "\n";
				float so = animation->audio_track_get_key_start_offset(track, key_idx);
				text += "Start (s): " + rtos(so) + "\n";
				float eo = animation->audio_track_get_key_end_offset(track, key_idx);
				text += "End (s): " + rtos(eo) + "\n";
			} break;
			case Animation::TYPE_ANIMATION: {
				String name = animation->animation_track_get_key_animation(track, key_idx);
				text += "Animation Clip: " + name + "\n";
			} break;
			}
			return text;
		}
	}

	return Control::get_tooltip(p_pos);
}

void TrackEdit::_gui_input(const Ref<InputEvent>& p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (p_event->is_pressed()) {
		/*if (ED_GET_SHORTCUT("animation_editor/duplicate_selection")->matches_event(p_event)) {
			emit_signal("duplicate_request");
			accept_event();
		}

		if (ED_GET_SHORTCUT("animation_editor/duplicate_selection_transposed")->matches_event(p_event)) {
			emit_signal("duplicate_transpose_request");
			accept_event();
		}

		if (ED_GET_SHORTCUT("animation_editor/delete_selection")->matches_event(p_event)) {
			emit_signal("delete_request");
			accept_event();
		}*/
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT) {
		Point2 pos = mb->get_position();

		if (check_rect.has_point(pos)) {
			undo_redo->create_action(TTR("Toggle Track Enabled"));
			undo_redo->add_do_method(animation.ptr(), "track_set_enabled", track, !animation->track_is_enabled(track));
			undo_redo->add_undo_method(animation.ptr(), "track_set_enabled", track, animation->track_is_enabled(track));
			undo_redo->commit_action();
			update();
			accept_event();
		}

		// Don't overlap track keys if they start at 0.
		if (path_rect.has_point(pos + Size2(type_icon->get_width(), 0))) {
			clicking_on_name = true;
			accept_event();
		}

		if (update_mode_rect.has_point(pos)) {
			if (!menu) {
				menu = memnew(PopupMenu);
				add_child(menu);
				menu->connect("id_pressed", this, "_menu_selected");
			}
			menu->clear();
			menu->add_icon_item(get_icon("TrackContinuous", "EditorIcons"), TTR("Continuous"), MENU_CALL_MODE_CONTINUOUS);
			menu->add_icon_item(get_icon("TrackDiscrete", "EditorIcons"), TTR("Discrete"), MENU_CALL_MODE_DISCRETE);
			menu->add_icon_item(get_icon("TrackTrigger", "EditorIcons"), TTR("Trigger"), MENU_CALL_MODE_TRIGGER);
			menu->add_icon_item(get_icon("TrackCapture", "EditorIcons"), TTR("Capture"), MENU_CALL_MODE_CAPTURE);

			Vector2 popup_pos = get_viewport()->get_canvas_transform().xform(get_global_position()) + update_mode_rect.position + Vector2(0, update_mode_rect.size.height);
			menu->set_position(popup_pos);
			menu->popup();
			accept_event();
		}

		if (interp_mode_rect.has_point(pos)) {
			if (!menu) {
				menu = memnew(PopupMenu);
				add_child(menu);
				menu->connect("id_pressed", this, "_menu_selected");
			}
			menu->clear();
			menu->add_icon_item(get_icon("InterpRaw", "EditorIcons"), TTR("Nearest"), MENU_INTERPOLATION_NEAREST);
			menu->add_icon_item(get_icon("InterpLinear", "EditorIcons"), TTR("Linear"), MENU_INTERPOLATION_LINEAR);
			menu->add_icon_item(get_icon("InterpCubic", "EditorIcons"), TTR("Cubic"), MENU_INTERPOLATION_CUBIC);

			Vector2 popup_pos = get_viewport()->get_canvas_transform().xform(get_global_position()) + interp_mode_rect.position + Vector2(0, interp_mode_rect.size.height);
			menu->set_position(popup_pos);
			menu->popup();
			accept_event();
		}

		if (loop_wrap_rect.has_point(pos)) {
			if (!menu) {
				menu = memnew(PopupMenu);
				add_child(menu);
				menu->connect("id_pressed", this, "_menu_selected");
			}
			menu->clear();
			menu->add_icon_item(get_icon("InterpWrapClamp", "EditorIcons"), TTR("Clamp Loop Interp"), MENU_LOOP_CLAMP);
			menu->add_icon_item(get_icon("InterpWrapLoop", "EditorIcons"), TTR("Wrap Loop Interp"), MENU_LOOP_WRAP);

			Vector2 popup_pos = get_viewport()->get_canvas_transform().xform(get_global_position()) + loop_wrap_rect.position + Vector2(0, loop_wrap_rect.size.height);
			menu->set_position(popup_pos);
			menu->popup();
			accept_event();
		}

		if (remove_rect.has_point(pos)) {
			emit_signal("remove_request", track);
			accept_event();
			return;
		}

		// Check keyframes.


		float scale = timeline->get_zoom_scale();
		int limit = timeline->get_name_limit();
		int limit_end = get_size().width - timeline->get_buttons_width();
		// Left Border including space occupied by keyframes on t=0.
		int limit_start_hitbox = limit - type_icon->get_width();

		if (pos.x >= limit_start_hitbox && pos.x <= limit_end) {
			int key_idx = -1;
			float key_distance = 1e20;

			// Select should happen in the opposite order of drawing for more accurate overlap select.
			for (int i = animation->track_get_key_count(track) - 1; i >= 0; i--) {
				Rect2 rect = get_key_rect(i, scale);
				float offset = animation->track_get_key_time(track, i) - timeline->get_value();
				offset = offset * scale + limit;
				rect.position.x += offset;

				if (rect.has_point(pos)) {
					if (is_key_selectable_by_distance()) {
						float distance = ABS(offset - pos.x);
						if (key_idx == -1 || distance < key_distance) {
							key_idx = i;
							key_distance = distance;
						}
					}
					else {
						// First one does it.
						key_idx = i;
						break;
					}
				}
			}

			if (key_idx != -1) {
				if (mb->get_command() || mb->get_shift()) {
					if (editor->is_key_selected(track, key_idx)) {
						emit_signal("deselect_key", key_idx);
					}
					else {
						emit_signal("select_key", key_idx, false);
						moving_selection_attempt = true;
						select_single_attempt = -1;
						moving_selection_from_ofs = (mb->get_position().x - limit) / timeline->get_zoom_scale();
					}
				}
				else {
					if (!editor->is_key_selected(track, key_idx)) {
						emit_signal("select_key", key_idx, true);
						select_single_attempt = -1;
					}
					else {
						select_single_attempt = key_idx;
					}

					moving_selection_attempt = true;
					moving_selection_from_ofs = (mb->get_position().x - limit) / timeline->get_zoom_scale();
				}
				accept_event();
			}
		}

	}

	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == BUTTON_RIGHT) {
		Point2 pos = mb->get_position();
		if (pos.x >= timeline->get_name_limit() && pos.x <= get_size().width - timeline->get_buttons_width()) {
			// Can do something with menu too! show insert key.
			float offset = (pos.x - timeline->get_name_limit()) / timeline->get_zoom_scale();
			if (!menu) {
				menu = memnew(PopupMenu);
				add_child(menu);
				menu->connect("id_pressed", this, "_menu_selected");
			}

			menu->clear();
			menu->add_icon_item(get_icon("Key", "EditorIcons"), TTR("Insert Key"), MENU_KEY_INSERT);
			if (editor->is_selection_active()) {
				menu->add_separator();
				menu->add_icon_item(get_icon("Duplicate", "EditorIcons"), TTR("Duplicate Key(s)"), MENU_KEY_DUPLICATE);

				AnimationPlayer* player = PlayerEditorControl::get_singleton()->get_player();
				if (!player->has_animation("RESET") || animation != player->get_animation("RESET")) {
					menu->add_icon_item(get_icon("Reload", "EditorIcons"), TTR("Add RESET Value(s)"), MENU_KEY_ADD_RESET);
				}

				menu->add_separator();
				menu->add_icon_item(get_icon("Remove", "EditorIcons"), TTR("Delete Key(s)"), MENU_KEY_DELETE);
			}

			menu->set_position(get_viewport()->get_canvas_transform().xform(get_global_position()) + get_local_mouse_position());
			menu->popup();

			insert_at_pos = offset + timeline->get_value();
			accept_event();
		}
	}

	if (mb.is_valid() && !mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT && clicking_on_name) {
		if (!path) {
			path_popup = memnew(Popup);
			add_child(path_popup);
			path = memnew(LineEdit);
			path_popup->add_child(path);
			path->set_anchors_and_margins_preset(PRESET_WIDE);
			path->connect("text_entered", this, "_path_submitted");
		}

		path->set_text(animation->track_get_path(track));
		Vector2 theme_ofs = path->get_stylebox("normal", "LineEdit")->get_offset();
		path_popup->set_position(get_viewport()->get_canvas_transform().xform(get_global_position()) + path_rect.position - theme_ofs);
		path_popup->set_size(path_rect.size);
		path_popup->popup();
		path->grab_focus();
		clicking_on_name = false;
	}

	if (mb.is_valid() && moving_selection_attempt) {
		if (!mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT) {
			moving_selection_attempt = false;
			if (moving_selection) {
				emit_signal("move_selection_commit");
			}
			else if (select_single_attempt != -1) {
				emit_signal("select_key", select_single_attempt, true);
			}
			moving_selection = false;
			select_single_attempt = -1;
		}

		if (moving_selection && mb->is_pressed() && mb->get_button_index() == BUTTON_RIGHT) {
			moving_selection_attempt = false;
			moving_selection = false;
			emit_signal("move_selection_cancel");
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		const int previous_hovering_key_idx = hovering_key_idx;

		// Hovering compressed keyframes for editing is not possible.
		const float scale = timeline->get_zoom_scale();
		const int limit = timeline->get_name_limit();
		const int limit_end = get_size().width - timeline->get_buttons_width();
		// Left Border including space occupied by keyframes on t=0.
		const int limit_start_hitbox = limit - type_icon->get_width();
		const Point2 pos = mm->get_position();

		if (pos.x >= limit_start_hitbox && pos.x <= limit_end) {
			// Use the same logic as key selection to ensure that hovering accurately represents
			// which key will be selected when clicking.
			int key_idx = -1;
			float key_distance = 1e20;

			hovering_key_idx = -1;

			// Hovering should happen in the opposite order of drawing for more accurate overlap hovering.
			for (int i = animation->track_get_key_count(track) - 1; i >= 0; i--) {
				Rect2 rect = get_key_rect(i, scale);
				float offset = animation->track_get_key_time(track, i) - timeline->get_value();
				offset = offset * scale + limit;
				rect.position.x += offset;

				if (rect.has_point(pos)) {
					if (is_key_selectable_by_distance()) {
						const float distance = ABS(offset - pos.x);
						if (key_idx == -1 || distance < key_distance) {
							key_idx = i;
							key_distance = distance;
							hovering_key_idx = i;
						}
					}
					else {
						// First one does it.
						hovering_key_idx = i;
						break;
					}
				}
			}

			print_line(itos(hovering_key_idx));

			if (hovering_key_idx != previous_hovering_key_idx) {
				// Required to draw keyframe hover feedback on the correct keyframe.
				update();
			}

		}
	}

	if (mm.is_valid() && (mm->get_button_mask() & BUTTON_MASK_LEFT) != 0 && moving_selection_attempt) {
		if (!moving_selection) {
			moving_selection = true;
			emit_signal("move_selection_begin");
		}

		float new_ofs = (mm->get_position().x - timeline->get_name_limit()) / timeline->get_zoom_scale();
		emit_signal("move_selection", new_ofs - moving_selection_from_ofs);
	}
}

Variant TrackEdit::get_drag_data(const Point2& p_point) {
	if (!clicking_on_name) {
		return Variant();
	}

	Dictionary drag_data;
	drag_data["type"] = "animation_track";
	String base_path = animation->track_get_path(track);
	base_path = base_path.get_slice(":", 0); // Remove sub-path.
	drag_data["group"] = base_path;
	drag_data["index"] = track;

	Button* tb = memnew(Button);
	tb->set_flat(true);
	tb->set_text(path_cache);
	tb->set_icon(icon_cache);
	set_drag_preview(tb);

	clicking_on_name = false;

	return drag_data;
}

bool TrackEdit::can_drop_data(const Point2& p_point, const Variant& p_data) const {
	Dictionary d = p_data;
	if (!d.has("type")) {
		return false;
	}

	String type = d["type"];
	if (type != "animation_track") {
		return false;
	}

	// Don't allow moving tracks outside their groups.
	if (get_editor()->is_grouping_tracks()) {
		String base_path = animation->track_get_path(track);
		base_path = base_path.get_slice(":", 0); // Remove sub-path.
		if (d["group"] != base_path) {
			return false;
		}
	}

	if (p_point.y < get_size().height / 2) {
		dropping_at = -1;
	}
	else {
		dropping_at = 1;
	}

	const_cast<TrackEdit*>(this)->update();
	const_cast<TrackEdit*>(this)->emit_signal("drop_attempted", track);

	return true;
}

void TrackEdit::drop_data(const Point2& p_point, const Variant& p_data) {
	Dictionary d = p_data;
	if (!d.has("type")) {
		return;
	}

	String type = d["type"];
	if (type != "animation_track") {
		return;
	}

	// Don't allow moving tracks outside their groups.
	if (get_editor()->is_grouping_tracks()) {
		String base_path = animation->track_get_path(track);
		base_path = base_path.get_slice(":", 0); // Remove sub-path.
		if (d["group"] != base_path) {
			return;
		}
	}

	int from_track = d["index"];

	if (dropping_at < 0) {
		emit_signal("dropped", from_track, track);
	}
	else {
		emit_signal("dropped", from_track, track + 1);
	}
}

void TrackEdit::_menu_selected(int p_index) {
	switch (p_index) {
	case MENU_CALL_MODE_CONTINUOUS:
	case MENU_CALL_MODE_DISCRETE:
	case MENU_CALL_MODE_TRIGGER:
	case MENU_CALL_MODE_CAPTURE: {
		Animation::UpdateMode update_mode = Animation::UpdateMode(p_index);
		undo_redo->create_action(TTR("Change Animation Update Mode"));
		undo_redo->add_do_method(animation.ptr(), "value_track_set_update_mode", track, update_mode);
		undo_redo->add_undo_method(animation.ptr(), "value_track_set_update_mode", track, animation->value_track_get_update_mode(track));
		undo_redo->commit_action();
		update();

	} break;
	case MENU_INTERPOLATION_NEAREST:
	case MENU_INTERPOLATION_LINEAR:
	case MENU_INTERPOLATION_CUBIC: {
		Animation::InterpolationType interp_mode = Animation::InterpolationType(p_index - MENU_INTERPOLATION_NEAREST);
		undo_redo->create_action(TTR("Change Animation Interpolation Mode"));
		undo_redo->add_do_method(animation.ptr(), "track_set_interpolation_type", track, interp_mode);
		undo_redo->add_undo_method(animation.ptr(), "track_set_interpolation_type", track, animation->track_get_interpolation_type(track));
		undo_redo->commit_action();
		update();
	} break;
	case MENU_LOOP_WRAP:
	case MENU_LOOP_CLAMP: {
		bool loop_wrap = p_index == MENU_LOOP_WRAP;
		undo_redo->create_action(TTR("Change Animation Loop Mode"));
		undo_redo->add_do_method(animation.ptr(), "track_set_interpolation_loop_wrap", track, loop_wrap);
		undo_redo->add_undo_method(animation.ptr(), "track_set_interpolation_loop_wrap", track, animation->track_get_interpolation_loop_wrap(track));
		undo_redo->commit_action();
		update();

	} break;
	case MENU_KEY_INSERT: {
		emit_signal("insert_key", insert_at_pos);
	} break;
	case MENU_KEY_DUPLICATE: {
		emit_signal("duplicate_request");
	} break;
	case MENU_KEY_ADD_RESET: {
		emit_signal("create_reset_request");

	} break;
	case MENU_KEY_DELETE: {
		emit_signal("delete_request");

	} break;
	}
}

void TrackEdit::cancel_drop() {
	if (dropping_at != 0) {
		dropping_at = 0;
		update();
	}
}

void TrackEdit::set_in_group(bool p_enable) {
	in_group = p_enable;
	update();
}

void TrackEdit::append_to_selection(const Rect2& p_box, bool p_deselection) {
	// Left Border including space occupied by keyframes on t=0.
	int limit_start_hitbox = timeline->get_name_limit() - type_icon->get_width();
	Rect2 select_rect(limit_start_hitbox, 0, get_size().width - timeline->get_name_limit() - timeline->get_buttons_width(), get_size().height);
	select_rect = select_rect.clip(p_box);

	// Select should happen in the opposite order of drawing for more accurate overlap select.
	for (int i = animation->track_get_key_count(track) - 1; i >= 0; i--) {
		Rect2 rect = const_cast<TrackEdit*>(this)->get_key_rect(i, timeline->get_zoom_scale());
		float offset = animation->track_get_key_time(track, i) - timeline->get_value();
		offset = offset * timeline->get_zoom_scale() + timeline->get_name_limit();
		rect.position.x += offset;

		if (select_rect.intersects(rect)) {
			if (p_deselection) {
				emit_signal("deselect_key", i);
			}
			else {
				emit_signal("select_key", i, false);
			}
		}
	}
}

void TrackEdit::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_zoom_changed"), &TrackEdit::_zoom_changed);
	ClassDB::bind_method(D_METHOD("_menu_selected"), &TrackEdit::_menu_selected);
	ClassDB::bind_method(D_METHOD("_path_submitted"), &TrackEdit::_path_submitted);
	ClassDB::bind_method(D_METHOD("_play_position_draw"), &TrackEdit::_play_position_draw);

	ADD_SIGNAL(MethodInfo("timeline_changed", PropertyInfo(Variant::REAL, "position"), PropertyInfo(Variant::BOOL, "drag"), PropertyInfo(Variant::BOOL, "timeline_only")));
	ADD_SIGNAL(MethodInfo("remove_request", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("dropped", PropertyInfo(Variant::INT, "from_track"), PropertyInfo(Variant::INT, "to_track")));
	ADD_SIGNAL(MethodInfo("insert_key", PropertyInfo(Variant::REAL, "ofs")));
	ADD_SIGNAL(MethodInfo("select_key", PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::BOOL, "single")));
	ADD_SIGNAL(MethodInfo("deselect_key", PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("bezier_edit"));

	ADD_SIGNAL(MethodInfo("move_selection_begin"));
	ADD_SIGNAL(MethodInfo("move_selection", PropertyInfo(Variant::REAL, "ofs")));
	ADD_SIGNAL(MethodInfo("move_selection_commit"));
	ADD_SIGNAL(MethodInfo("move_selection_cancel"));

	ADD_SIGNAL(MethodInfo("duplicate_request"));
	ADD_SIGNAL(MethodInfo("create_reset_request"));
	ADD_SIGNAL(MethodInfo("duplicate_transpose_request"));
	ADD_SIGNAL(MethodInfo("delete_request"));
}

TrackEdit::TrackEdit() {
	undo_redo = nullptr;
	timeline = nullptr;
	root = nullptr;
	path = nullptr;
	path_popup = nullptr;
	menu = nullptr;
	dropping_at = 0;

	select_single_attempt = -1;

	play_position_pos = 0;
	play_position = memnew(Control);
	play_position->set_mouse_filter(MOUSE_FILTER_PASS);
	add_child(play_position);
	play_position->set_anchors_and_margins_preset(PRESET_WIDE);
	play_position->connect("draw", this, "_play_position_draw");
	set_focus_mode(FOCUS_CLICK);
	set_mouse_filter(MOUSE_FILTER_PASS); // Scroll has to work too for selection.
}