#ifndef ANIMATION_PLAYER_EDITOR_H
#define ANIMATION_PLAYER_EDITOR_H

#include "editor/animation_track_editor.h"
#include "scene/animation/animation_player.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/tree.h"

class PlayerEditor : public VBoxContainer {
	GDCLASS(PlayerEditor, VBoxContainer);
	
	AnimationPlayer *player = nullptr;

	enum {
		TOOL_NEW_ANIM,
		TOOL_ANIM_LIBRARY,
		TOOL_DUPLICATE_ANIM,
		TOOL_RENAME_ANIM,
		TOOL_EDIT_TRANSITIONS,
		TOOL_REMOVE_ANIM,
		TOOL_EDIT_RESOURCE
	};

	enum {
		ANIM_OPEN,
		ANIM_SAVE,
		ANIM_SAVE_AS
	};

	enum {
		RESOURCE_LOAD,
		RESOURCE_SAVE
	};

	OptionButton *animation = nullptr;
	Button *stop = nullptr;
	Button *play = nullptr;
	Button *play_from = nullptr;
	Button *play_bw = nullptr;
	Button *play_bw_from = nullptr;
	Button *autoplay = nullptr;

	MenuButton *tool_anim = nullptr;
	SpinBox *frame = nullptr;
	LineEdit *scale = nullptr;
	LineEdit *name = nullptr;
	OptionButton *library = nullptr;
	Label *name_title = nullptr;
	UndoRedo *undo_redo = nullptr;

	Ref<Texture> autoplay_icon;
	Ref<Texture> reset_icon;
	Ref<ImageTexture> autoplay_reset_icon;
	bool last_active;
	float timeline_position;

	EditorFileDialog *file = nullptr;
	ConfirmationDialog *delete_dialog = nullptr;

	struct BlendEditor {
		AcceptDialog *dialog = nullptr;
		Tree *tree = nullptr;
		OptionButton *next = nullptr;

	} blend_editor;

	ConfirmationDialog *name_dialog = nullptr;
	ConfirmationDialog *error_dialog = nullptr;
	int name_dialog_op = TOOL_NEW_ANIM;

	bool updating;
	bool updating_blends;

	AnimationTrackEditor *track_editor = nullptr;
	static PlayerEditor *singleton;

	// Onion skinning.
	struct {
		// Settings.
		bool enabled = false;
		bool past = false;
		bool future = false;
		int steps = 0;
		bool differences_only = false;
		bool force_white_modulate = false;
		bool include_gizmos = false;

		int get_needed_capture_count() const {
			// 'Differences only' needs a capture of the present.
			return (past && future ? 2 * steps : steps) + (differences_only ? 1 : 0);
		}

		// Rendering.
		int64_t last_frame = 0;
		int can_overlay = 0;
		Size2 capture_size;
		Vector<RID> captures;
		Vector<bool> captures_valid;
		struct {
			RID canvas;
			RID canvas_item;
			Ref<ShaderMaterial> material;
			Ref<Shader> shader;
		} capture;
	} onion;

	void _select_anim_by_name(const String &p_anim);
	double _get_editor_step() const;
	void _play_pressed();
	void _play_from_pressed();
	void _play_bw_pressed();
	void _play_bw_from_pressed();
	void _autoplay_pressed();
	void _stop_pressed();
	void _animation_selected(int p_which);
	void _animation_new();
	void _animation_rename();
	void _animation_name_edited();

	void _animation_remove();
	void _animation_remove_confirmed();
	void _animation_blend();
	void _animation_edit();
	void _animation_duplicate();
	Ref<Animation> _animation_clone(const Ref<Animation> p_anim);
	void _animation_resource_edit();
	void _scale_changed(const String &p_scale);
	void _seek_value_changed(float p_value, bool p_set = false, bool p_timeline_only = false);
	void _blend_editor_next_changed(const int p_idx);

	void _list_changed();
	void _update_animation();
	void _update_player();
	void _update_animation_list_icons();
	void _blend_edited();

	void _animation_player_changed(Object *p_pl);

	void _animation_key_editor_seek(float p_pos, bool p_drag, bool p_timeline_only = false);
	void _animation_key_editor_anim_len_changed(float p_len);

	void _unhandled_key_input(const Ref<InputEvent> &p_ev);
	void _animation_tool_menu(int p_option);
	void _onion_skinning_menu(int p_option);

	void _pin_pressed();
	String _get_current() const;

protected:
	void _notification(int p_what);
	void _node_removed(Node *p_node);
	static void _bind_methods();

public:
	AnimationPlayer *get_player() const;

	static PlayerEditor *get_singleton() { return singleton; }
	AnimationTrackEditor *get_track_editor() { return track_editor; }
	Dictionary get_state() const;
	void set_state(const Dictionary &p_state);

	void ensure_visibility();

	void set_undo_redo(UndoRedo *p_undo_redo) { undo_redo = p_undo_redo; }
	void edit(AnimationPlayer *p_player);
	void forward_force_draw_over_viewport(Control *p_overlay);

	PlayerEditor();
	~PlayerEditor();
};

#endif // ANIMATION_PLAYER_EDITOR_H
