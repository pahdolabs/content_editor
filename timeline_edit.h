#ifndef TIMELINE_EDIT_H
#define TIMELINE_EDIT_H

#include "editor/editor_data.h"
#include "editor/editor_spin_slider.h"
#include "editor/property_editor.h"
#include "editor/property_selector.h"

#include "scene/gui/control.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/animation.h"

class TrackEdit;
class ViewPanner;

class TimelineEdit : public Range {
	GDCLASS(TimelineEdit, Range);

	Ref<Animation> animation;
	TrackEdit* track_edit = nullptr;
	int name_limit;
	Range* zoom = nullptr;
	Range* h_scroll = nullptr;
	float play_position_pos;

	HBoxContainer* len_hb = nullptr;
	EditorSpinSlider* length = nullptr;
	Button* loop = nullptr;
	TextureRect* time_icon = nullptr;

	MenuButton* add_track = nullptr;
	Control* play_position = nullptr; //separate control used to draw so updates for only position changed are much faster
	HScrollBar* hscroll = nullptr;

	void _zoom_changed(double);
	void _anim_length_changed(double p_new_len);
	void _anim_loop_pressed();

	void _play_position_draw();
	UndoRedo* undo_redo = nullptr;
	Rect2 hsize_rect;

	bool editing = false;
	bool use_fps = false;

	Ref<ViewPanner> panner;
	void _scroll_callback(Vector2 p_scroll_vec, bool p_alt);
	void _pan_callback(Vector2 p_scroll_vec);
	void _zoom_callback(Vector2 p_scroll_vec, Vector2 p_origin, bool p_alt);

	bool dragging_timeline = false;
	bool dragging_hsize = false;
	float dragging_hsize_from;
	float dragging_hsize_at;

	void _gui_input(const Ref<InputEvent>& p_event);
	void _track_added(int p_track);

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	int get_name_limit() const;
	int get_buttons_width() const;

	float get_zoom_scale() const;

	virtual Size2 get_minimum_size() const override;
	void set_animation(const Ref<Animation>& p_animation);
	void set_track_edit(TrackEdit* p_track_edit);
	void set_zoom(Range* p_zoom);
	Range* get_zoom() const { return zoom; }
	void set_undo_redo(UndoRedo* p_undo_redo);

	void set_play_position(float p_pos);
	float get_play_position() const;
	void update_play_position();

	void update_values();

	void set_use_fps(bool p_use_fps);
	bool is_using_fps() const;

	void set_hscroll(HScrollBar* p_hscroll);

	virtual CursorShape get_cursor_shape(const Point2& p_pos) const override;

	TimelineEdit();
};

#endif