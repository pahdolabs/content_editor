#ifndef PLAYER_EDITOR_CONTROL_H
#define PLAYER_EDITOR_CONTROL_H

#include "scene/gui/box_container.h"
#include "core/undo_redo.h"
#include "scene/gui/file_dialog.h"
#include "scene/gui/option_button.h"

#include "scene/animation/animation_player.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/spin_box.h"

class TrackEditor;
class MenuButton;

class PlayerEditorControl : public VBoxContainer {
	GDCLASS(PlayerEditorControl, VBoxContainer);
	
	AnimationPlayer *player = nullptr;
	
	OptionButton *animation = nullptr;
	Label* animation_label = nullptr;
	Button *stop = nullptr;
	Button *play = nullptr;
	Button *play_from = nullptr;
	Button *play_bw = nullptr;
	Button *play_bw_from = nullptr;
	
	SpinBox *frame = nullptr;
	Label *name_title = nullptr;
	UndoRedo *undo_redo = nullptr;
	
	bool last_active;
	bool updating;
	float timeline_position;
	
	TrackEditor *track_editor = nullptr;

	void _select_anim_by_name(const String &p_anim);
	double _get_editor_step() const;
	void _play_pressed();
	void _play_from_pressed();
	void _play_bw_pressed();
	void _play_bw_from_pressed();
	void _stop_pressed();
	void _animation_selected(int p_which);
	
	void _animation_edit();
	void _animation_resource_edit();
	void _seek_value_changed(float p_value, bool p_set = false, bool p_timeline_only = false);

	void _list_changed();
	void _update_animation();
	void _update_player();
	void _update_animation_list_icons();

	void _animation_player_changed(Object *p_pl);

	void _animation_key_editor_seek(float p_pos, bool p_drag, bool p_timeline_only = false);
	void _animation_key_editor_anim_len_changed(float p_len);

	void _unhandled_key_input(const Ref<InputEvent> &p_ev);
	
	void _icons_cache_changed();

protected:
	void _notification(int p_what);
	void _node_removed(Node *p_node);
	static void _bind_methods();

public:
	AnimationPlayer *get_player() const;
	
	TrackEditor *get_track_editor() { return track_editor; }
	Dictionary get_state() const;
	void set_state(const Dictionary &p_state);

	void ensure_visibility();

	void set_use_dropdown_animation_selector(bool p_use_dropdown);

	UndoRedo* get_undo_redo() const {
		return undo_redo;
	}
	void edit(Object *p_player);

	String get_current() const;
	void set_current(const String &p_animation);

	PlayerEditorControl();
	~PlayerEditorControl();
};

#endif // PLAYER_EDITOR_CONTROL_H
