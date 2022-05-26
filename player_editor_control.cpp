// Ported from Godot 4, code largely by Juan Linietsky and Godot contributors

#include "player_editor_control.h"

#include <scene/gui/separator.h>

#include "icons_cache.h"
#include "track_editor.h"
#include "core/os/input.h"
#include "core/os/keyboard.h"
#include "scene/main/viewport.h"
#include "scene/resources/animation.h"
#include "scene/scene_string_names.h"
#include "servers/visual_server.h"

///////////////////////////////////

void PlayerEditorControl::_node_removed(Node *p_node) {
	if (player && player == p_node) {
		player = nullptr;

		set_process(false);

		track_editor->set_animation(Ref<Animation>());
		track_editor->set_root(nullptr);
		track_editor->show_select_node_warning(true);
		_update_player();
	}
}

void PlayerEditorControl::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PROCESS: {
			if (!player) {
				return;
			}

			updating = true;

			if (player->is_playing()) {
				{
					String animname = player->get_assigned_animation();

					if (player->has_animation(animname)) {
						Ref<Animation> anim = player->get_animation(animname);
						if (!anim.is_null()) {
							frame->set_max((double)anim->get_length());
						}
					}
				}
				frame->set_value(player->get_current_animation_position());
				track_editor->set_anim_pos(player->get_current_animation_position());

			} else if (!player->is_valid()) {
				// Reset timeline when the player has been stopped externally
				frame->set_value(0);
			} else if (last_active) {
				// Need the last frame after it stopped.
				frame->set_value(player->get_current_animation_position());
			}

			last_active = player->is_playing();
			updating = false;
		} break;

		case NOTIFICATION_ENTER_TREE: {
			get_tree()->connect("node_removed", this, "_node_removed");

			//add_style_override("panel", EditorNode::get_singleton()->get_gui_base()->get_stylebox("panel", "Panel"));
		} break;
			
		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			IconsCache* icons = IconsCache::get_singleton();

			play->set_icon(icons->get_icon("PlayStart"));
			play_from->set_icon(icons->get_icon("Play"));
			play_bw->set_icon(icons->get_icon("PlayStartBackwards"));
			play_bw_from->set_icon(icons->get_icon("PlayBackwards"));
			
			stop->set_icon(icons->get_icon("Stop"));

			_update_animation_list_icons();
		} break;
	}
}

void PlayerEditorControl::_play_pressed() {
	String current = _get_current();

	if (!current.empty()) {
		if (current == player->get_assigned_animation()) {
			player->stop(); //so it won't blend with itself
		}
		player->play(current);
	}

	//unstop
	stop->set_pressed(false);
}

void PlayerEditorControl::_play_from_pressed() {
	String current = _get_current();

	if (!current.empty()) {
		float time = player->get_current_animation_position();

		if (current == player->get_assigned_animation() && player->is_playing()) {
			player->stop(); //so it won't blend with itself
		}

		player->play(current);
		player->seek(time);
	}

	//unstop
	stop->set_pressed(false);
}

String PlayerEditorControl::_get_current() const {
	String current;
	if (animation->get_selected() >= 0 && animation->get_selected() < animation->get_item_count()) {
		current = animation->get_item_text(animation->get_selected());
	}
	return current;
}
void PlayerEditorControl::_play_bw_pressed() {
	String current = _get_current();
	if (!current.empty()) {
		if (current == player->get_assigned_animation()) {
			player->stop(); //so it won't blend with itself
		}
		player->play(current, -1, -1, true);
	}

	//unstop
	stop->set_pressed(false);
}

void PlayerEditorControl::_play_bw_from_pressed() {
	String current = _get_current();

	if (!current.empty()) {
		float time = player->get_current_animation_position();
		if (current == player->get_assigned_animation()) {
			player->stop(); //so it won't blend with itself
		}

		player->play(current, -1, -1, true);
		player->seek(time);
	}

	//unstop
	stop->set_pressed(false);
}

void PlayerEditorControl::_stop_pressed() {
	if (!player) {
		return;
	}

	player->stop(false);
	play->set_pressed(false);
	stop->set_pressed(true);
}

void PlayerEditorControl::_animation_selected(int p_which) {
	if (updating) {
		return;
	}
	// when selecting an animation, the idea is that the only interesting behavior
	// ui-wise is that it should play/blend the next one if currently playing
	String current = _get_current();

	if (!current.empty()) {
		player->set_assigned_animation(current);

		Ref<Animation> anim = player->get_animation(current);
		{
			track_editor->set_animation(anim);
			Node *root = player->get_node(player->get_root());
			if (root) {
				track_editor->set_root(root);
			}
		}
		frame->set_max((double)anim->get_length());

	}
	else {
		track_editor->set_animation(Ref<Animation>());
		track_editor->set_root(nullptr);
	}

	PlayerEditorControl::get_singleton()->get_track_editor()->update_keying();
	_animation_key_editor_seek(timeline_position, false);
}

void PlayerEditorControl::_select_anim_by_name(const String &p_anim) {
	int idx = -1;
	for (int i = 0; i < animation->get_item_count(); i++) {
		if (animation->get_item_text(i) == p_anim) {
			idx = i;
			break;
		}
	}

	ERR_FAIL_COND(idx == -1);

	animation->select(idx);

	_animation_selected(idx);
}

double PlayerEditorControl::_get_editor_step() const {
	// Returns the effective snapping value depending on snapping modifiers, or 0 if snapping is disabled.
	if (track_editor->is_snap_enabled()) {
		const String current = player->get_assigned_animation();
		const Ref<Animation> anim = player->get_animation(current);
		ERR_FAIL_COND_V(!anim.is_valid(), 0.0);

		// Use more precise snapping when holding Shift
		return Input::get_singleton()->is_key_pressed(KEY_SHIFT) ? anim->get_step() * 0.25 : anim->get_step();
	}

	return 0.0;
}

void PlayerEditorControl::ensure_visibility() {
	_animation_edit();
}

Dictionary PlayerEditorControl::get_state() const {
	Dictionary d;

	d["visible"] = is_visible_in_tree();
	if (is_visible_in_tree() && player) {
		d["player"] = get_tree()->get_current_scene()->get_path_to(player);
		d["animation"] = player->get_assigned_animation();
		d["track_editor_state"] = track_editor->get_state();
	}

	return d;
}

void PlayerEditorControl::set_state(const Dictionary &p_state) {
	if (!p_state.has("visible") || !p_state["visible"]) {
		return;
	}

	if (p_state.has("player")) {
		Node *n = get_tree()->get_current_scene()->get_node(p_state["player"]);
		if (Object::cast_to<AnimationPlayer>(n)) {
			player = Object::cast_to<AnimationPlayer>(n);
			_update_player();
			set_process(true);
			ensure_visibility();

			if (p_state.has("animation")) {
				String anim = p_state["animation"];
				if (!anim.empty() && player->has_animation(anim)) {
					_select_anim_by_name(anim);
					_animation_edit();
				}
			}
		}
	}

	if (p_state.has("track_editor_state")) {
		track_editor->set_state(p_state["track_editor_state"]);
	}
}

void PlayerEditorControl::_animation_resource_edit() {
	String current = _get_current();
	if (current != String()) {
		Ref<Animation> anim = player->get_animation(current);
		//EditorNode::get_singleton()->edit_resource(anim);
	}
}

void PlayerEditorControl::_animation_edit() {
	String current = _get_current();
	if (current != String()) {
		Ref<Animation> anim = player->get_animation(current);
		track_editor->set_animation(anim);

		Node *root = player->get_node(player->get_root());
		if (root) {
			track_editor->set_root(root);
		}
	} else {
		track_editor->set_animation(Ref<Animation>());
		track_editor->set_root(nullptr);
	}
}

void PlayerEditorControl::_update_animation() {
	// the purpose of _update_animation is to reflect the current state
	// of the animation player in the current editor..

	updating = true;

	if (player->is_playing()) {
		play->set_pressed(true);
		stop->set_pressed(false);

	} else {
		play->set_pressed(false);
		stop->set_pressed(true);
	}

	String current = player->get_assigned_animation();

	for (int i = 0; i < animation->get_item_count(); i++) {
		if (animation->get_item_text(i) == current) {
			animation->select(i);
			break;
		}
	}

	updating = false;
}

void PlayerEditorControl::_update_player() {
	updating = true;

	animation->clear();

	if (!player) {
		PlayerEditorControl::get_singleton()->get_track_editor()->update_keying();
		return;
	}
	
	int active_idx = -1;
	bool no_anims_found = true;

	List<StringName> animlist;
	player->get_animation_list(&animlist);

	for (List<StringName>::Element* E = animlist.front(); E; E = E->next()) {
		String path = "";
		path += E->get();
		animation->add_item(path);
		if (player->get_assigned_animation() == path) {
			active_idx = animation->get_item_count() - 1;
		}
		no_anims_found = false;
	}

	stop->set_disabled(no_anims_found);
	play->set_disabled(no_anims_found);
	play_bw->set_disabled(no_anims_found);
	play_bw_from->set_disabled(no_anims_found);
	play_from->set_disabled(no_anims_found);
	frame->set_editable(!no_anims_found);
	animation->set_disabled(no_anims_found);

	_update_animation_list_icons();

	updating = false;
	if (active_idx != -1) {
		animation->select(active_idx);
		_animation_selected(active_idx);
	} else if (animation->get_item_count()) {
		int item = animation->get_item_id(0);
		animation->select(item);
		_animation_selected(item);
	} else {
		_animation_selected(0);
	}

	if (!no_anims_found) {
		String current = animation->get_item_text(animation->get_selected());
		Ref<Animation> anim = player->get_animation(current);
		track_editor->set_animation(anim);
		Node *root = player->get_node(player->get_root());
		if (root) {
			track_editor->set_root(root);
		}
	}

	_update_animation();
}

void PlayerEditorControl::_update_animation_list_icons() {
	for (int i = 0; i < animation->get_item_count(); i++) {
		String name = animation->get_item_text(i);
		if (animation->is_item_disabled(i)) {
			continue;
		}

		Ref<Texture> icon;

		animation->set_item_icon(i, icon);
	}
}

void PlayerEditorControl::edit(Object *p_player) {
	player = cast_to<AnimationPlayer>(p_player);

	if (player) {
		_update_player();
		
		track_editor->show_select_node_warning(false);
	} else {
		track_editor->show_select_node_warning(true);
	}
}

void PlayerEditorControl::_seek_value_changed(float p_value, bool p_set, bool p_timeline_only) {
	if (updating || !player || player->is_playing()) {
		return;
	};

	updating = true;
	String current = player->get_assigned_animation();
	if (current.empty() || !player->has_animation(current)) {
		updating = false;
		current = "";
		return;
	};

	Ref<Animation> anim;
	anim = player->get_animation(current);

	float pos = CLAMP((double)anim->get_length() * (p_value / frame->get_max()), 0, (double)anim->get_length());
	if (track_editor->is_snap_enabled()) {
		pos = Math::stepify(pos, _get_editor_step());
	}

	if (!p_timeline_only) {
		if (player->is_valid() && !p_set) {
			float cpos = player->get_current_animation_position();

			player->seek_delta(pos, pos - cpos);
		} else {
			player->stop(true);
			player->seek(pos, true);
		}
	}

	track_editor->set_anim_pos(pos);
};

void PlayerEditorControl::_animation_player_changed(Object *p_pl) {
	if (player == p_pl && is_visible_in_tree()) {
		_update_player();
	}
}

void PlayerEditorControl::_list_changed() {
	if (is_visible_in_tree()) {
		_update_player();
	}
}

void PlayerEditorControl::_animation_key_editor_anim_len_changed(float p_len) {
	frame->set_max(p_len);
}

void PlayerEditorControl::_animation_key_editor_seek(float p_pos, bool p_drag, bool p_timeline_only) {
	timeline_position = p_pos;

	if (!is_visible_in_tree()) {
		return;
	}

	if (!player) {
		return;
	}

	if (player->is_playing()) {
		return;
	}

	if (!player->has_animation(player->get_assigned_animation())) {
		return;
	}

	updating = true;
	frame->set_value(Math::stepify(p_pos, _get_editor_step()));
	updating = false;
	_seek_value_changed(p_pos, !p_drag, p_timeline_only);
}

void PlayerEditorControl::_unhandled_key_input(const Ref<InputEvent> &p_ev) {
	ERR_FAIL_COND(p_ev.is_null());

	Ref<InputEventKey> k = p_ev;
	if (is_visible_in_tree() && k.is_valid() && k->is_pressed() && !k->is_echo() && !k->get_alt() && !k->get_control() && !k->get_metakey()) {
		switch (k->get_scancode()) {
			case KEY_A: {
				if (!k->get_shift()) {
					_play_bw_from_pressed();
				} else {
					_play_bw_pressed();
				}
				accept_event();
			} break;
			case KEY_S: {
				_stop_pressed();
				accept_event();
			} break;
			case KEY_D: {
				if (!k->get_shift()) {
					_play_from_pressed();
				} else {
					_play_pressed();
				}
				accept_event();
			} break;
			default:
				break;
		}
	}
}

void PlayerEditorControl::_bind_methods() {
	ClassDB::bind_method(D_METHOD("edit", "animation_player"), &PlayerEditorControl::edit);
	ClassDB::bind_method(D_METHOD("get_undo_redo"), &PlayerEditorControl::get_undo_redo);
	ClassDB::bind_method(D_METHOD("ensure_visibility"), &PlayerEditorControl::ensure_visibility);
	ClassDB::bind_method(D_METHOD("get_track_editor"), &PlayerEditorControl::get_track_editor);
	ClassDB::bind_method(D_METHOD("_node_removed"), &PlayerEditorControl::_node_removed);
	ClassDB::bind_method(D_METHOD("_animation_edit"), &PlayerEditorControl::_animation_edit);
	ClassDB::bind_method(D_METHOD("_animation_resource_edit"), &PlayerEditorControl::_animation_resource_edit);
	ClassDB::bind_method(D_METHOD("_animation_player_changed"), &PlayerEditorControl::_animation_player_changed);
	ClassDB::bind_method(D_METHOD("_list_changed"), &PlayerEditorControl::_list_changed);
	ClassDB::bind_method(D_METHOD("_play_pressed"), &PlayerEditorControl::_play_pressed);
	ClassDB::bind_method(D_METHOD("_play_from_pressed"), &PlayerEditorControl::_play_from_pressed);
	ClassDB::bind_method(D_METHOD("_play_bw_pressed"), &PlayerEditorControl::_play_bw_pressed);
	ClassDB::bind_method(D_METHOD("_play_bw_from_pressed"), &PlayerEditorControl::_play_bw_from_pressed);
	ClassDB::bind_method(D_METHOD("_stop_pressed"), &PlayerEditorControl::_stop_pressed);
	ClassDB::bind_method(D_METHOD("_animation_selected"), &PlayerEditorControl::_animation_selected);
	ClassDB::bind_method(D_METHOD("_seek_value_changed"), &PlayerEditorControl::_seek_value_changed);
	ClassDB::bind_method(D_METHOD("_animation_key_editor_seek"), &PlayerEditorControl::_animation_key_editor_seek);
	ClassDB::bind_method(D_METHOD("_animation_key_editor_anim_len_changed"), &PlayerEditorControl::_animation_key_editor_anim_len_changed);
	ClassDB::bind_method(D_METHOD("_icons_cache_changed"), &PlayerEditorControl::_icons_cache_changed);
}

PlayerEditorControl *PlayerEditorControl::singleton = nullptr;

AnimationPlayer *PlayerEditorControl::get_player() const {
	return player;
}

void PlayerEditorControl::_icons_cache_changed() {
	_notification(NOTIFICATION_THEME_CHANGED);
}

PlayerEditorControl::PlayerEditorControl() {
	singleton = this;

	updating = false;

	undo_redo = memnew(UndoRedo);

	set_focus_mode(FOCUS_ALL);

	player = nullptr;

	HBoxContainer *hb = memnew(HBoxContainer);
	add_child(hb);

	play_bw_from = memnew(Button);
	play_bw_from->set_flat(true);
	play_bw_from->set_tooltip(TTR("Play selected animation backwards from current pos. (A)"));
	hb->add_child(play_bw_from);

	play_bw = memnew(Button);
	play_bw->set_flat(true);
	play_bw->set_tooltip(TTR("Play selected animation backwards from end. (Shift+A)"));
	hb->add_child(play_bw);

	stop = memnew(Button);
	stop->set_flat(true);
	stop->set_toggle_mode(true);
	hb->add_child(stop);
	stop->set_tooltip(TTR("Stop animation playback. (S)"));

	play = memnew(Button);
	play->set_flat(true);
	play->set_tooltip(TTR("Play selected animation from start. (Shift+D)"));
	hb->add_child(play);

	play_from = memnew(Button);
	play_from->set_flat(true);
	play_from->set_tooltip(TTR("Play selected animation from current pos. (D)"));
	hb->add_child(play_from);

	frame = memnew(SpinBox);
	hb->add_child(frame);
	frame->set_custom_minimum_size(Size2(60, 0));
	frame->set_stretch_ratio(2);
	frame->set_step(0.0001);
	frame->set_tooltip(TTR("Animation position (in seconds)."));

	hb->add_child(memnew(VSeparator));
	
	animation = memnew(OptionButton);
	hb->add_child(animation);
	animation->set_h_size_flags(SIZE_EXPAND_FILL);
	animation->set_tooltip(TTR("Display list of animations in player."));
	animation->set_clip_text(true);

	hb->add_child(memnew(VSeparator));

	track_editor = memnew(TrackEditor);
	
	play->connect("pressed", this, "_play_pressed");
	play_from->connect("pressed", this, "_play_from_pressed");
	play_bw->connect("pressed", this, "_play_bw_pressed");
	play_bw_from->connect("pressed", this, "_play_bw_from_pressed");
	stop->connect("pressed", this, "_stop_pressed");

	animation->connect("item_selected", this, "_animation_selected");

	frame->connect("value_changed", this, "_seek_value_changed", make_binds(true, false));

	last_active = false;
	timeline_position = 0;

	set_process_unhandled_key_input(true);

	add_child(track_editor);
	track_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	track_editor->connect("timeline_changed", this, "_animation_key_editor_seek");
	track_editor->connect("animation_len_changed", this, "_animation_key_editor_anim_len_changed");

	_update_player();

	IconsCache::get_singleton()->connect("icons_changed", this, "_icons_cache_changed");
}

PlayerEditorControl::~PlayerEditorControl() {
	if(undo_redo) {
		memfree(undo_redo);
		undo_redo = nullptr;
	}
}
