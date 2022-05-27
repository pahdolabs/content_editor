#ifndef TRACK_EDIT_H
#define TRACK_EDIT_H

#include "scene/gui/control.h"
#include "scene/resources/animation.h"

class TimelineEdit;
class TrackEditor;
class UndoRedo;
class Popup;
class PopupMenu;
class LineEdit;

class TrackEdit : public Control {
	GDCLASS(TrackEdit, Control);

	enum {
		MENU_KEY_INSERT,
		MENU_KEY_DUPLICATE,
		MENU_KEY_ADD_RESET,
		MENU_KEY_DELETE
	};
	TimelineEdit* timeline = nullptr;
	UndoRedo* undo_redo = nullptr;
	Node* root = nullptr;
	Control* play_position = nullptr; //separate control used to draw so updates for only position changed are much faster
	float play_position_pos;
	NodePath node_path;

	Ref<Animation> animation;
	int track;

	Rect2 check_rect;
	Rect2 path_rect;

	Rect2 remove_rect;

	Ref<Texture> type_icon;
	Ref<Texture> selected_icon;

	PopupMenu* menu = nullptr;

	bool hovered = false;
	bool clicking_on_name = false;
	int hovering_key_idx = -1;

	void _zoom_changed();

	Ref<Texture> icon_cache;
	String path_cache;

	void _menu_selected(int p_index);
	
	void _play_position_draw();
	bool _is_value_key_valid(const Variant& p_key_value, Variant::Type& r_valid_type) const;

	Ref<Texture> _get_key_type_icon() const;

	mutable int dropping_at;
	float insert_at_pos;
	bool moving_selection_attempt = false;
	int select_single_attempt;
	bool moving_selection = false;
	float moving_selection_from_ofs;

	bool in_group = false;
	TrackEditor* editor = nullptr;

	void _icons_cache_changed();

protected:
	static void _bind_methods();
	void _notification(int p_what);
	
public:
	void _gui_input(const Ref<InputEvent>& p_event);

	virtual Variant get_drag_data(const Point2& p_point) override;
	virtual bool can_drop_data(const Point2& p_point, const Variant& p_data) const override;
	virtual void drop_data(const Point2& p_point, const Variant& p_data) override;

	virtual String get_tooltip(const Point2& p_pos) const override;

	virtual int get_key_height() const;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec);
	virtual bool is_key_selectable_by_distance() const;
	virtual void draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right);
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right);
	virtual void draw_bg(int p_clip_left, int p_clip_right);
	virtual void draw_fg(int p_clip_left, int p_clip_right);

	//helper
	void draw_texture_region_clipped(const Ref<Texture>& p_texture, const Rect2& p_rect, const Rect2& p_region);
	void draw_rect_clipped(const Rect2& p_rect, const Color& p_color, bool p_filled = true);

	int get_track() const;
	Ref<Animation> get_animation() const;
	TimelineEdit* get_timeline() const { return timeline; }
	TrackEditor* get_editor() const { return editor; }
	UndoRedo* get_undo_redo() const { return undo_redo; }
	void set_animation_and_track(const Ref<Animation>& p_animation, int p_track);
	virtual Size2 get_minimum_size() const override;

	void set_undo_redo(UndoRedo* p_undo_redo);
	void set_timeline(TimelineEdit* p_timeline);
	void set_editor(TrackEditor* p_editor);
	void set_root(Node* p_root);

	void set_play_position(float p_pos);
	void update_play_position();
	void cancel_drop();

	void set_in_group(bool p_enable);
	void append_to_selection(const Rect2& p_box, bool p_deselection);

	TrackEdit();
};

#endif