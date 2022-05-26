#include "timeline_edit.h"

#include <core/undo_redo.h>

#include "consts.h"
#include "icons_cache.h"
#include "track_edit.h"
#include "track_editor.h"
#include "view_panner.h"

void TimelineEdit::_zoom_changed(double) {
	update();
	play_position->update();
	emit_signal("zoom_changed");
}

float TimelineEdit::get_zoom_scale() const {
	float zv = zoom->get_max() - zoom->get_value();
	if (zv < 1) {
		zv = 1.0 - zv;
		return Math::pow(1.0f + zv, 8.0f) * 100;
	}
	else {
		return 1.0 / Math::pow(zv, 8.0f) * 100;
	}
}

void TimelineEdit::_anim_length_changed(double p_new_len) {
	if (editing) {
		return;
	}

	p_new_len = MAX(0.001, p_new_len);
	if (use_fps && animation->get_step() > 0) {
		p_new_len *= animation->get_step();
	}

	editing = true;
	undo_redo->create_action(TTR("Change Animation Length"));
	undo_redo->add_do_method(animation.ptr(), "set_length", p_new_len);
	undo_redo->add_undo_method(animation.ptr(), "set_length", animation->get_length());
	undo_redo->commit_action();
	editing = false;
	update();

	emit_signal("length_changed", p_new_len);
}

void TimelineEdit::_anim_loop_pressed() {
	undo_redo->create_action(TTR("Change Animation Loop"));
	if (animation->has_loop()) {
		undo_redo->add_do_method(animation.ptr(), "set_loop", true);
	}
	else {
		undo_redo->add_do_method(animation.ptr(), "set_loop", false);

	}
	undo_redo->add_undo_method(animation.ptr(), "set_loop_mode", animation->has_loop());
	undo_redo->commit_action();
}

int TimelineEdit::get_buttons_width() const {
	IconsCache* icons = IconsCache::get_singleton();
	Ref<Texture> interp_mode = icons->get_icon("TrackContinuous");
	Ref<Texture> interp_type = icons->get_icon("InterpRaw");
	Ref<Texture> loop_type = icons->get_icon("InterpWrapClamp");
	Ref<Texture> remove_icon = icons->get_icon("Remove");
	Ref<Texture> down_icon = icons->get_icon("select_arrow");

	int total_w = (interp_mode != nullptr ? interp_mode->get_width() : 0) + (interp_type != nullptr ? interp_type->get_width() : 0) + (loop_type != nullptr ? loop_type->get_width() : 0) + (remove_icon != nullptr ? remove_icon->get_width() : 0);
	total_w += ((down_icon != nullptr ? down_icon->get_width() : 0) + 4 * 1) * 4;

	return total_w;
}

int TimelineEdit::get_name_limit() const {
	Ref<Texture> hsize_icon = IconsCache::get_singleton()->get_icon("Hsize");

	int limit = MAX(name_limit, (hsize_icon != nullptr ? hsize_icon->get_width() : 0));

	limit = MIN(limit, get_size().width - get_buttons_width() - 1);

	return limit;
}

void TimelineEdit::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_RESIZED: {
		len_hb->set_position(Vector2(get_size().width - get_buttons_width(), 0));
		len_hb->set_size(Size2(get_buttons_width(), get_size().height));
	} break;

	case NOTIFICATION_DRAW: {
		int key_range = get_size().width - get_buttons_width() - get_name_limit();

		if (!animation.is_valid()) {
			return;
		}

		Ref<Font> font = get_font("font", "Label");
		Color color = get_color("font_color", "Label");

		int zoomw = key_range;
		float scale = get_zoom_scale();
		int h = get_size().height;

		float l = animation->get_length();
		if (l <= 0) {
			l = 0.001; // Avoid crashor.
		}

		IconsCache* icons = IconsCache::get_singleton();

		Ref<Texture> hsize_icon = icons->get_icon("Hsize");
		int hsize_width = hsize_icon != nullptr ? hsize_icon->get_width() : 0;
		int hsize_height = hsize_icon != nullptr ? hsize_icon->get_height() : 0;
		hsize_rect = Rect2(get_name_limit() - hsize_width - 2 * 1, (get_size().height - hsize_height) / 2, hsize_width, hsize_height);
		if (hsize_icon != nullptr) {
			draw_texture(hsize_icon, hsize_rect.position);
		}

		{
			float time_min = 0;
			float time_max = animation->get_length();
			for (int i = 0; i < animation->get_track_count(); i++) {
				if (animation->track_get_key_count(i) > 0) {
					float beg = animation->track_get_key_time(i, 0);

					if (beg < time_min) {
						time_min = beg;
					}

					float end = animation->track_get_key_time(i, animation->track_get_key_count(i) - 1);

					if (end > time_max) {
						time_max = end;
					}
				}
			}

			float extra = (zoomw / scale) * 0.5;

			time_max += extra;
			set_min(time_min);
			set_max(time_max);

			if (zoomw / scale < (time_max - time_min)) {
				hscroll->show();

			}
			else {
				hscroll->hide();
			}
		}

		set_page(zoomw / scale);

		int end_px = (l - get_value()) * scale;
		int begin_px = -get_value() * scale;
		Color notimecol = Colors::dark_color_2;
		Color timecolor = color;
		timecolor.a = 0.2;
		Color linecolor = color;
		linecolor.a = 0.2;

		{
			draw_rect(Rect2(Point2(get_name_limit(), 0), Point2(zoomw - 1, h)), notimecol);

			if (begin_px < zoomw && end_px > 0) {
				if (begin_px < 0) {
					begin_px = 0;
				}
				if (end_px > zoomw) {
					end_px = zoomw;
				}

				draw_rect(Rect2(Point2(get_name_limit() + begin_px, 0), Point2(end_px - begin_px - 1, h)), timecolor);
			}
		}

		Color color_time_sec = color;
		Color color_time_dec = color;
		color_time_dec.a *= 0.5;
#define SC_ADJ 100
		int min = 30;
		int dec = 1;
		int step = 1;
		int decimals = 2;
		bool step_found = false;

		const float period_width = font->get_char_size('.', 0).width;
		float max_digit_width = font->get_char_size('0', 0).width;
		for (int i = 1; i <= 9; i++) {
			const float digit_width = font->get_char_size('0' + i, 0).width;
			max_digit_width = MAX(digit_width, max_digit_width);
		}
		const int max_sc = int(Math::ceil(zoomw / scale));
		const int max_sc_width = String::num(max_sc).length() * max_digit_width;

		while (!step_found) {
			min = max_sc_width;
			if (decimals > 0) {
				min += period_width + max_digit_width * decimals;
			}

			static const int _multp[3] = { 1, 2, 5 };
			for (int i = 0; i < 3; i++) {
				step = (_multp[i] * dec);
				if (step * scale / SC_ADJ > min) {
					step_found = true;
					break;
				}
			}
			if (step_found) {
				break;
			}
			dec *= 10;
			decimals--;
			if (decimals < 0) {
				decimals = 0;
			}
		}

		if (use_fps) {
			float step_size = animation->get_step();
			if (step_size > 0) {
				int prev_frame_ofs = -10000000;

				for (int i = 0; i < zoomw; i++) {
					float pos = get_value() + double(i) / scale;
					float prev = get_value() + (double(i) - 1.0) / scale;

					int frame = pos / step_size;
					int prev_frame = prev / step_size;

					bool sub = Math::floor(prev) == Math::floor(pos);

					if (frame != prev_frame && i >= prev_frame_ofs) {
						draw_line(Point2(get_name_limit() + i, 0), Point2(get_name_limit() + i, h), linecolor, Math::round(1.0));

						draw_string(font, Point2(get_name_limit() + i + 3 * 1, (h - font->get_height()) / 2 + font->get_ascent()).floor(), itos(frame), sub ? color_time_dec : color_time_sec);
						prev_frame_ofs = i + font->get_string_size(itos(frame)).x + 5 * 1;
					}
				}
			}

		}
		else {
			for (int i = 0; i < zoomw; i++) {
				float pos = get_value() + double(i) / scale;
				float prev = get_value() + (double(i) - 1.0) / scale;

				int sc = int(Math::floor(pos * SC_ADJ));
				int prev_sc = int(Math::floor(prev * SC_ADJ));
				bool sub = (sc % SC_ADJ);

				if ((sc / step) != (prev_sc / step) || (prev_sc < 0 && sc >= 0)) {
					int scd = sc < 0 ? prev_sc : sc;
					draw_line(Point2(get_name_limit() + i, 0), Point2(get_name_limit() + i, h), linecolor, Math::round(1.0));
					draw_string(font, Point2(get_name_limit() + i + 3, (h - font->get_height()) / 2 + font->get_ascent()).floor(), String::num((scd - (scd % step)) / double(SC_ADJ), decimals), sub ? color_time_dec : color_time_sec);
				}
			}
		}

		draw_line(Vector2(0, get_size().height), get_size(), linecolor, Math::round(1.0));
	} break;
	}
}

void TimelineEdit::set_animation(const Ref<Animation>& p_animation) {
	animation = p_animation;
	if (animation.is_valid()) {
		len_hb->show();
		play_position->show();
	}
	else {
		len_hb->hide();
		play_position->hide();
	}
	update();
}

Size2 TimelineEdit::get_minimum_size() const {
	Size2 ms = Size2(0, 0);
	Ref<Font> font = get_font("font", "Label");
	ms.height = MAX(ms.height, font->get_height());
	IconsCache* icons = IconsCache::get_singleton();
	ms.width = get_buttons_width() + (icons->has_icon("Hsize") ? icons->get_icon("Hsize")->get_width() : 0) + 2;
	return ms;
}

void TimelineEdit::set_undo_redo(UndoRedo* p_undo_redo) {
	undo_redo = p_undo_redo;
}

void TimelineEdit::set_zoom(Range* p_zoom) {
	zoom = p_zoom;
	zoom->connect("value_changed", this, "_zoom_changed");
}

void TimelineEdit::set_track_edit(TrackEdit* p_track_edit) {
	track_edit = p_track_edit;
}

void TimelineEdit::set_play_position(float p_pos) {
	play_position_pos = p_pos;
	play_position->update();
}

float TimelineEdit::get_play_position() const {
	return play_position_pos;
}

void TimelineEdit::update_play_position() {
	play_position->update();
}

void TimelineEdit::_play_position_draw() {
	if (!animation.is_valid() || play_position_pos < 0) {
		return;
	}

	float scale = get_zoom_scale();
	int h = play_position->get_size().height;

	int px = (-get_value() + play_position_pos) * scale + get_name_limit();

	if (px >= get_name_limit() && px < (play_position->get_size().width - get_buttons_width())) {
		Color color = Colors::accent_color;
		play_position->draw_line(Point2(px, 0), Point2(px, h), color, Math::round(2 * 1.0));

		IconsCache* icons = IconsCache::get_singleton();
		if (icons->has_icon("TimelineIndicator")) {
			play_position->draw_texture(
				icons->get_icon("TimelineIndicator"),
				Point2(px - icons->get_icon("TimelineIndicator")->get_width() * 0.5, 0),
				color);
		}
	}
}

void TimelineEdit::_gui_input(const Ref<InputEvent>& p_event) {
	ERR_FAIL_COND(p_event.is_null());

	if (panner->gui_input(p_event)) {
		accept_event();
		return;
	}

	const Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid() && mb->is_pressed() && mb->get_alt() && mb->get_button_index() == BUTTON_WHEEL_UP) {
		if (track_edit) {
			track_edit->get_editor()->goto_prev_step(true);
		}
		accept_event();
	}

	if (mb.is_valid() && mb->is_pressed() && mb->get_alt() && mb->get_button_index() == BUTTON_WHEEL_DOWN) {
		if (track_edit) {
			track_edit->get_editor()->goto_next_step(true);
		}
		accept_event();
	}

	if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT && hsize_rect.has_point(mb->get_position())) {
		dragging_hsize = true;
		dragging_hsize_from = mb->get_position().x;
		dragging_hsize_at = name_limit;
	}

	if (mb.is_valid() && !mb->is_pressed() && mb->get_button_index() == BUTTON_LEFT && dragging_hsize) {
		dragging_hsize = false;
	}
	if (mb.is_valid() && mb->get_position().x > get_name_limit() && mb->get_position().x < (get_size().width - get_buttons_width())) {
		if (!panner->is_panning() && mb->get_button_index() == BUTTON_LEFT) {
			int x = mb->get_position().x - get_name_limit();

			float ofs = x / get_zoom_scale() + get_value();
			emit_signal("timeline_changed", ofs, false, mb->get_alt());
			dragging_timeline = true;
		}
	}

	if (dragging_timeline && mb.is_valid() && mb->get_button_index() == BUTTON_LEFT && !mb->is_pressed()) {
		dragging_timeline = false;
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid()) {
		if (dragging_hsize) {
			int ofs = mm->get_position().x - dragging_hsize_from;
			name_limit = dragging_hsize_at + ofs;
			update();
			emit_signal("name_limit_changed");
			play_position->update();
		}
		if (dragging_timeline) {
			int x = mm->get_position().x - get_name_limit();
			float ofs = x / get_zoom_scale() + get_value();
			emit_signal("timeline_changed", ofs, false, mm->get_alt());
		}
	}
}

Control::CursorShape TimelineEdit::get_cursor_shape(const Point2& p_pos) const {
	if (dragging_hsize || hsize_rect.has_point(p_pos)) {
		// Indicate that the track name column's width can be adjusted
		return Control::CURSOR_HSIZE;
	}
	else {
		return get_default_cursor_shape();
	}
}

void TimelineEdit::_scroll_callback(Vector2 p_scroll_vec, bool p_alt) {
	// Timeline has no vertical scroll, so we change it to horizontal.
	p_scroll_vec.x += p_scroll_vec.y;
	_pan_callback(-p_scroll_vec * 32);
}

void TimelineEdit::_pan_callback(Vector2 p_scroll_vec) {
	set_value(get_value() - p_scroll_vec.x / get_zoom_scale());
}

void TimelineEdit::_zoom_callback(Vector2 p_scroll_vec, Vector2 p_origin, bool p_alt) {
	double new_zoom_value;
	double current_zoom_value = get_zoom()->get_value();
	if (current_zoom_value <= 0.1) {
		new_zoom_value = MAX(0.01, current_zoom_value - 0.01 * SGN(p_scroll_vec.y));
	}
	else {
		new_zoom_value = p_scroll_vec.y > 0 ? MAX(0.01, current_zoom_value / 1.05) : current_zoom_value * 1.05;
	}
	get_zoom()->set_value(new_zoom_value);
}

void TimelineEdit::set_use_fps(bool p_use_fps) {
	use_fps = p_use_fps;
	update();
}

bool TimelineEdit::is_using_fps() const {
	return use_fps;
}

void TimelineEdit::set_hscroll(HScrollBar* p_hscroll) {
	hscroll = p_hscroll;
}

void TimelineEdit::_track_added(int p_track) {
	emit_signal("track_added", p_track);
}

void TimelineEdit::_icons_cache_changed() {
	_notification(NOTIFICATION_THEME_CHANGED);
	update();
}


void TimelineEdit::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_zoom_changed"), &TimelineEdit::_zoom_changed);
	ClassDB::bind_method(D_METHOD("_play_position_draw"), &TimelineEdit::_play_position_draw);
	ClassDB::bind_method(D_METHOD("_anim_length_changed"), &TimelineEdit::_anim_length_changed);
	ClassDB::bind_method(D_METHOD("_anim_loop_pressed"), &TimelineEdit::_anim_loop_pressed);
	ClassDB::bind_method(D_METHOD("_track_added"), &TimelineEdit::_track_added);
	ClassDB::bind_method(D_METHOD("_scroll_callback"), &TimelineEdit::_scroll_callback);
	ClassDB::bind_method(D_METHOD("_pan_callback"), &TimelineEdit::_pan_callback);
	ClassDB::bind_method(D_METHOD("_zoom_callback"), &TimelineEdit::_zoom_callback);
	ClassDB::bind_method(D_METHOD("_icons_cache_changed"), &TimelineEdit::_icons_cache_changed);
	ClassDB::bind_method(D_METHOD("_gui_input", "event"), &TimelineEdit::_gui_input);
	ADD_SIGNAL(MethodInfo("zoom_changed"));
	ADD_SIGNAL(MethodInfo("name_limit_changed"));
	ADD_SIGNAL(MethodInfo("timeline_changed", PropertyInfo(Variant::REAL, "position"), PropertyInfo(Variant::BOOL, "drag"), PropertyInfo(Variant::BOOL, "timeline_only")));
	ADD_SIGNAL(MethodInfo("track_added", PropertyInfo(Variant::INT, "track")));
	ADD_SIGNAL(MethodInfo("length_changed", PropertyInfo(Variant::REAL, "size")));
}

TimelineEdit::TimelineEdit() {
	name_limit = 150 * 1.0;
	zoom = nullptr;
	track_edit = nullptr;

	play_position_pos = 0;
	play_position = memnew(Control);
	play_position->set_mouse_filter(MOUSE_FILTER_PASS);
	add_child(play_position);
	play_position->set_anchors_and_margins_preset(PRESET_WIDE);
	play_position->connect("draw", this, "_play_position_draw");

	len_hb = memnew(HBoxContainer);

	Control* expander = memnew(Control);
	expander->set_h_size_flags(SIZE_EXPAND_FILL);
	len_hb->add_child(expander);
	add_child(len_hb);

	len_hb->hide();

	panner.instance();
	panner->set_callbacks(this, "_scroll_callback", this, "_pan_callback", this, "_zoom_callback");

	IconsCache::get_singleton()->connect("icons_changed", this, "_icons_cache_changed");
}