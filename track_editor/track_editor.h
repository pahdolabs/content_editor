#ifndef TRACK_EDITOR_H
#define TRACK_EDITOR_H

#include "modules/gdscript/gdscript.h"
#include "scene/gui/box_container.h"
#include "scene/gui/check_box.h"
#include "scene/gui/dialogs.h"

#include "scene/gui/control.h"
#include "scene/gui/scroll_bar.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/texture_rect.h"
#include "scene/resources/animation.h"

class UndoRedo;
class TreeItem;
class Tree;
class OptionButton;
class ScrollContainer;
class PanelContainer;
class TimelineEdit;
class TrackEdit;
class ViewPanner;
class TrackKeyEdit;
class MultiTrackKeyEdit;
class TrackEditPlugin;
class Spatial;

class TrackEditor : public VBoxContainer {
	GDCLASS(TrackEditor, VBoxContainer);

	Ref<Animation> animation;
	Node* root = nullptr;

	PanelContainer* main_panel = nullptr;
	HScrollBar* hscroll = nullptr;
	ScrollContainer* scroll = nullptr;
	VBoxContainer* track_vbox = nullptr;

	Label* info_message = nullptr;

	TimelineEdit* timeline = nullptr;
	HSlider* zoom = nullptr;
	SpinBox* step = nullptr;
	TextureRect* zoom_icon = nullptr;
	Button* snap = nullptr;
	OptionButton* snap_mode = nullptr;

	Map<Ref<Script>, Ref<Script>> track_edit_groups;

	Vector<TrackEdit*> track_edits;

	Button* imported_anim_warning = nullptr;
	void _show_imported_anim_warning();

	void _snap_mode_changed(int p_mode);

	bool animation_changing_awaiting_update = false;
	void _animation_update();
	int _get_track_selected();
	void _animation_changed();
	void _update_tracks();

	void _name_limit_changed();
	void _timeline_changed(float p_new_pos, bool p_drag, bool p_timeline_only);
	void _track_remove_request(int p_track);
	void _track_grab_focus(int p_track);

	UndoRedo* undo_redo = nullptr;

	void _update_scroll(double);
	void _update_step(double p_new_step);
	void _update_length(double p_new_len);
	void _dropped_track(int p_from_track, int p_to_track);

	void _add_track(int p_type);
	void _new_track_node_selected(NodePath p_path);
	void _new_track_property_selected(String p_name);

	void _update_step_spinbox();

	int adding_track_type;
	NodePath adding_track_path;

	bool keying = false;

	struct InsertData {
		Animation::TrackType type;
		NodePath path;
		int track_idx = 0;
		Variant value;
		String query;
		bool advance = false;
	}; /* insert_data;*/

	Label* insert_confirm_text = nullptr;
	CheckBox* insert_confirm_bezier = nullptr;
	CheckBox* insert_confirm_reset = nullptr;
	ConfirmationDialog* insert_confirm = nullptr;
	bool insert_queue = false;
	List<InsertData> insert_data;

	void _query_insert(const InsertData& p_id);
	Ref<Animation> _create_and_get_reset_animation();
	void _confirm_insert_list();
	struct TrackIndices {
		int normal;
		int reset;

		TrackIndices(const Animation* p_anim = nullptr, const Animation* p_reset_anim = nullptr) {
			normal = p_anim ? p_anim->get_track_count() : 0;
			reset = p_reset_anim ? p_reset_anim->get_track_count() : 0;
		}
	};
	TrackIndices _confirm_insert(InsertData p_id, TrackIndices p_next_tracks, bool p_create_reset, Ref<Animation> p_reset_anim, bool p_create_beziers);
	void _insert_track(bool p_create_reset, bool p_create_beziers);

	void _root_removed(Node* p_root);

	PropertyInfo _find_hint_for_track(int p_idx, NodePath& r_base_path, Variant* r_current_val = nullptr);

	Ref<ViewPanner> panner;
	void _scroll_callback(Vector2 p_scroll_vec, bool p_alt);
	void _pan_callback(Vector2 p_scroll_vec);
	void _zoom_callback(Vector2 p_scroll_vec, Vector2 p_origin, bool p_alt);

	void _timeline_value_changed(double);

	float insert_key_from_track_call_ofs;
	int insert_key_from_track_call_track;
	void _insert_key_from_track(float p_ofs, int p_track);
	void _add_method_key(const String& p_method);

	void _clear_selection(bool p_update = false);
	void _clear_selection_for_anim(const Ref<Animation>& p_anim);
	void _select_at_anim(const Ref<Animation>& p_anim, int p_track, float p_pos);

	//selection

	struct SelectedKey {
		int track = 0;
		int key = 0;
		bool operator<(const SelectedKey& p_key) const { return track == p_key.track ? key < p_key.key : track < p_key.track; };
	};

	struct KeyInfo {
		float pos = 0;
	};

	Map<SelectedKey, KeyInfo> selection;

	void _key_selected(int p_key, bool p_single, int p_track);
	void _key_deselected(int p_key, int p_track);

	bool moving_selection = false;
	float moving_selection_offset;
	void _move_selection_begin();
	void _move_selection(float p_offset);
	void _move_selection_commit();
	void _move_selection_cancel();

	TrackKeyEdit* key_edit = nullptr;
	MultiTrackKeyEdit* multi_key_edit = nullptr;
	void _update_key_edit();

	void _clear_key_edit();

	Control* box_selection = nullptr;
	void _box_selection_draw();
	bool box_selecting = false;
	Vector2 box_selecting_from;
	Rect2 box_select_rect;
	void _scroll_input(const Ref<InputEvent>& p_event);

	Vector<Ref<TrackEditPlugin>> track_edit_plugins;

	////////////// edit menu stuff

	ConfirmationDialog* optimize_dialog = nullptr;
	SpinBox* optimize_linear_error = nullptr;
	SpinBox* optimize_angular_error = nullptr;
	SpinBox* optimize_max_angle = nullptr;

	ConfirmationDialog* cleanup_dialog = nullptr;
	CheckBox* cleanup_keys = nullptr;
	CheckBox* cleanup_tracks = nullptr;
	CheckBox* cleanup_all = nullptr;

	ConfirmationDialog* scale_dialog = nullptr;
	SpinBox* scale = nullptr;

	void _select_all_tracks_for_copy();

	void _edit_menu_about_to_popup();
	void _edit_menu_pressed(int p_option);
	int last_menu_track_opt;

	void _cleanup_animation(Ref<Animation> p_animation);

	void _anim_duplicate_keys(bool transpose);

	void _view_group_toggle();
	Button* view_group = nullptr;
	Button* selected_filter = nullptr;

	void _selection_changed();

	ConfirmationDialog* track_copy_dialog = nullptr;
	Tree* track_copy_select = nullptr;

	struct TrackClipboard {
		NodePath full_path;
		NodePath base_path;
		Animation::TrackType track_type = Animation::TrackType::TYPE_ANIMATION;
		Animation::InterpolationType interp_type = Animation::InterpolationType::INTERPOLATION_CUBIC;
		Animation::UpdateMode update_mode = Animation::UpdateMode::UPDATE_CAPTURE;
		bool loop_wrap = false;
		bool enabled = false;

		struct Key {
			float time = 0;
			float transition = 0;
			Variant value;
		};
		Vector<Key> keys;
	};

	Vector<TrackClipboard> track_clipboard;

	void _insert_animation_key(NodePath p_path, const Variant& p_value);

	void _pick_track_filter_text_changed(const String& p_newtext);
	void _pick_track_select_recursive(TreeItem* p_item, const String& p_filter, Vector<Node*>& p_select_candidates);
	void _pick_track_filter_input(const Ref<InputEvent>& p_ie);

	void _icons_cache_changed();

	void add_track_edit(TrackEdit *p_track_edit, int p_track);

	const StringName _does_track_belong_to_header = "does_track_belong_to_header";

protected:
	static void _bind_methods();
	void _notification(int p_what);

public:
	enum {
		EDIT_COPY_TRACKS,
		EDIT_COPY_TRACKS_CONFIRM,
		EDIT_PASTE_TRACKS,
		EDIT_SCALE_SELECTION,
		EDIT_SCALE_FROM_CURSOR,
		EDIT_SCALE_CONFIRM,
		EDIT_DUPLICATE_SELECTION,
		EDIT_DUPLICATE_TRANSPOSED,
		EDIT_ADD_RESET_KEY,
		EDIT_DELETE_SELECTION,
		EDIT_GOTO_NEXT_STEP,
		EDIT_GOTO_PREV_STEP,
		EDIT_APPLY_RESET,
		EDIT_OPTIMIZE_ANIMATION,
		EDIT_OPTIMIZE_ANIMATION_CONFIRM,
		EDIT_CLEAN_UP_ANIMATION,
		EDIT_CLEAN_UP_ANIMATION_CONFIRM
	};

	void add_track_edit_plugin(const Ref<TrackEditPlugin>& p_plugin);
	void remove_track_edit_plugin(const Ref<TrackEditPlugin>& p_plugin);

	void set_track_edit_type(const Ref<Script> p_header_class, const Ref<Script> p_track_edit_class);

	void set_animation(const Ref<Animation>& p_anim);
	Ref<Animation> get_current_animation() const;
	void set_root(Node* p_root);
	Node* get_root() const;
	void update_keying();
	bool has_keying() const;

	Dictionary get_state() const;
	void set_state(const Dictionary& p_state);

	void cleanup();

	void set_anim_pos(float p_pos);
	void insert_node_value_key(Node* p_node, const String& p_property, const Variant& p_value, bool p_only_if_exists = false);
	void insert_transform_key(Spatial* p_node, const String& p_sub, const Animation::TrackType p_type, const Variant p_value);
	bool has_track(Spatial* p_node, const String& p_sub, const Animation::TrackType p_type);
	void make_insert_queue();
	void commit_insert_queue();

	void show_select_node_warning(bool p_show);

	bool is_key_selected(int p_track, int p_key) const;
	bool is_selection_active() const;
	bool is_moving_selection() const;
	bool is_snap_enabled() const;
	float get_moving_selection_offset() const;
	float snap_time(float p_value, bool p_relative = false);
	bool is_grouping_tracks();

	/** If `p_from_mouse_event` is `true`, handle Shift key presses for precise snapping. */
	void goto_prev_step(bool p_from_mouse_event);

	/** If `p_from_mouse_event` is `true`, handle Shift key presses for precise snapping. */
	void goto_next_step(bool p_from_mouse_event);
	
	TrackEditor();
	~TrackEditor();
};

#endif
