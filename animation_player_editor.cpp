/*************************************************************************/
/*  animation_player_editor_plugin.cpp                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "animation_player_editor.h"

#include "core/os/input.h"
#include "core/os/keyboard.h"
#include "editor/editor_file_dialog.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "editor/plugins/canvas_item_editor_plugin.h" // For onion skinning.
#include "editor/scene_tree_dock.h"
#include "scene/main/viewport.h"
#include "scene/resources/animation.h"
#include "scene/scene_string_names.h"
#include "servers/visual_server.h"

///////////////////////////////////

void PlayerEditor::_node_removed(Node *p_node) {
	if (player && player == p_node) {
		player = nullptr;

		set_process(false);

		track_editor->set_animation(Ref<Animation>());
		track_editor->set_root(nullptr);
		track_editor->show_select_node_warning(true);
		_update_player();
	}
}

void PlayerEditor::_notification(int p_what) {
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
			tool_anim->get_popup()->connect("id_pressed", this, "_animation_tool_menu");

			onion_skinning->get_popup()->connect("id_pressed", this, "_onion_skinning_menu");

			blend_editor.next->connect("item_selected", this, "_blend_editor_next_changed");

			get_tree()->connect("node_removed", this, "_node_removed");

			add_style_override("panel", EditorNode::get_singleton()->get_gui_base()->get_stylebox("panel", "Panel"));
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			add_style_override("panel", EditorNode::get_singleton()->get_gui_base()->get_stylebox("panel", "Panel"));
		} break;

		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			autoplay->set_icon(get_icon("AutoPlay", "EditorIcons"));

			play->set_icon(get_icon("PlayStart", "EditorIcons"));
			play_from->set_icon(get_icon("Play", "EditorIcons"));
			play_bw->set_icon(get_icon("PlayStartBackwards", "EditorIcons"));
			play_bw_from->set_icon(get_icon("PlayBackwards", "EditorIcons"));

			autoplay_icon = get_icon("AutoPlay", "EditorIcons");
			reset_icon = get_icon("Reload", "EditorIcons");
			{
				Ref<Image> autoplay_img = autoplay_icon->get_data();
				Ref<Image> reset_img = reset_icon->get_data();
				Ref<Image> autoplay_reset_img;
				Size2 icon_size = autoplay_img->get_size();
				autoplay_reset_img.instance();
				autoplay_reset_img->create(icon_size.x * 2, icon_size.y, false, autoplay_img->get_format());
				autoplay_reset_img->blit_rect(autoplay_img, Rect2(Point2(), icon_size), Point2());
				autoplay_reset_img->blit_rect(reset_img, Rect2(Point2(), icon_size), Point2(icon_size.x, 0));
				autoplay_reset_icon.instance();
				autoplay_reset_icon->create_from_image(autoplay_reset_img);
			}
			stop->set_icon(get_icon("Stop", "EditorIcons"));

			onion_toggle->set_icon(get_icon("Onion", "EditorIcons"));
			onion_skinning->set_icon(get_icon("GuiTabMenuHl", "EditorIcons"));

			pin->set_icon(get_icon("Pin", "EditorIcons"));

			tool_anim->add_style_override("normal", get_stylebox("normal", "Button"));
			track_editor->get_edit_menu()->add_style_override("normal", get_stylebox("normal", "Button"));

#define ITEM_ICON(m_item, m_icon) tool_anim->get_popup()->set_item_icon(tool_anim->get_popup()->get_item_index(m_item), get_icon(m_icon, "EditorIcons"))

			ITEM_ICON(TOOL_NEW_ANIM, "New");
			ITEM_ICON(TOOL_ANIM_LIBRARY, "AnimationLibrary");
			ITEM_ICON(TOOL_DUPLICATE_ANIM, "Duplicate");
			ITEM_ICON(TOOL_RENAME_ANIM, "Rename");
			ITEM_ICON(TOOL_EDIT_TRANSITIONS, "Blend");
			ITEM_ICON(TOOL_EDIT_RESOURCE, "Edit");
			ITEM_ICON(TOOL_REMOVE_ANIM, "Remove");

			_update_animation_list_icons();
		} break;
	}
}

void PlayerEditor::_autoplay_pressed() {
	if (updating) {
		return;
	}
	if (animation->get_item_count() == 0) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());
	if (player->get_autoplay() == current) {
		//unset
		undo_redo->create_action(TTR("Toggle Autoplay"));
		undo_redo->add_do_method(player, "set_autoplay", "");
		undo_redo->add_undo_method(player, "set_autoplay", player->get_autoplay());
		undo_redo->add_do_method(this, "_animation_player_changed", player);
		undo_redo->add_undo_method(this, "_animation_player_changed", player);
		undo_redo->commit_action();

	} else {
		//set
		undo_redo->create_action(TTR("Toggle Autoplay"));
		undo_redo->add_do_method(player, "set_autoplay", current);
		undo_redo->add_undo_method(player, "set_autoplay", player->get_autoplay());
		undo_redo->add_do_method(this, "_animation_player_changed", player);
		undo_redo->add_undo_method(this, "_animation_player_changed", player);
		undo_redo->commit_action();
	}
}

void PlayerEditor::_play_pressed() {
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

void PlayerEditor::_play_from_pressed() {
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

String PlayerEditor::_get_current() const {
	String current;
	if (animation->get_selected() >= 0 && animation->get_selected() < animation->get_item_count()) {
		current = animation->get_item_text(animation->get_selected());
	}
	return current;
}
void PlayerEditor::_play_bw_pressed() {
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

void PlayerEditor::_play_bw_from_pressed() {
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

void PlayerEditor::_stop_pressed() {
	if (!player) {
		return;
	}

	player->stop(false);
	play->set_pressed(false);
	stop->set_pressed(true);
}

void PlayerEditor::_animation_selected(int p_which) {
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

	} else {
		track_editor->set_animation(Ref<Animation>());
		track_editor->set_root(nullptr);
	}

	autoplay->set_pressed(current == player->get_autoplay());

	PlayerEditor::get_singleton()->get_track_editor()->update_keying();
	_animation_key_editor_seek(timeline_position, false);
}

void PlayerEditor::_animation_new() {
	name_dialog_op = TOOL_NEW_ANIM;
	name_title->set_text(TTR("New Animation Name:"));

	int count = 1;
	String base = TTR("New Anim");
	while (true) {
		String attempt = base;
		if (count > 1) {
			attempt += " (" + itos(count) + ")";
		}
		if (player->has_animation(attempt)) {
			count++;
			continue;
		}
		base = attempt;
		break;
	}
	
	name->set_text(base);
	name_dialog->popup_centered(Size2(300, 90));
	name->select_all();
	name->grab_focus();
}

void PlayerEditor::_animation_rename() {
	if (!animation->get_item_count()) {
		return;
	}
	int selected = animation->get_selected();
	String selected_name = animation->get_item_text(selected);

	name_title->set_text(TTR("Change Animation Name:"));
	name->set_text(selected_name);
	name_dialog_op = TOOL_RENAME_ANIM;
	name_dialog->popup_centered(Size2(300, 90));
	name->select_all();
	name->grab_focus();
	library->hide();
}

void PlayerEditor::_animation_remove() {
	if (!animation->get_item_count()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());

	delete_dialog->set_text(vformat(TTR("Delete Animation '%s'?"), current));
	delete_dialog->popup_centered();
}

void PlayerEditor::_animation_remove_confirmed() {
	String current = animation->get_item_text(animation->get_selected());
	Ref<Animation> anim = player->get_animation(current);
	
	undo_redo->create_action(TTR("Remove Animation"));
	if (player->get_autoplay() == current) {
		undo_redo->add_do_method(player, "set_autoplay", "");
		undo_redo->add_undo_method(player, "set_autoplay", current);
		// Avoid having the autoplay icon linger around if there is only one animation in the player.
		undo_redo->add_do_method(this, "_animation_player_changed", player);
	}
	undo_redo->add_do_method(player, "remove_animation", current);
	undo_redo->add_undo_method(player, "add_animation", current, anim);
	undo_redo->add_do_method(this, "_animation_player_changed", player);
	undo_redo->add_undo_method(this, "_animation_player_changed", player);
	if (animation->get_item_count() && animation->get_item_id(0) == animation->get_item_id(animation->get_item_count())) { // Last item remaining.
		undo_redo->add_do_method(this, "_stop_onion_skinning");
		undo_redo->add_undo_method(this, "_start_onion_skinning");
	}
	undo_redo->commit_action();
}

void PlayerEditor::_select_anim_by_name(const String &p_anim) {
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

double PlayerEditor::_get_editor_step() const {
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

void PlayerEditor::_animation_name_edited() {
	player->stop();

	String new_name = name->get_text();
	if (new_name.empty() || new_name.find(":") > -1 || new_name.find("/") > -1) {
		error_dialog->set_text(TTR("Invalid animation name!"));
		error_dialog->popup_centered();
		return;
	}

	if (name_dialog_op == TOOL_RENAME_ANIM && animation->get_item_count() && animation->get_item_text(animation->get_selected()) == new_name) {
		name_dialog->hide();
		return;
	}

	if (player->has_animation(new_name)) {
		error_dialog->set_text(TTR("Animation name already exists!"));
		error_dialog->popup_centered();
		return;
	}

	switch (name_dialog_op) {
		case TOOL_RENAME_ANIM: {
			String current = animation->get_item_text(animation->get_selected());
			Ref<Animation> anim = player->get_animation(current);

			undo_redo->create_action(TTR("Rename Animation"));
			undo_redo->add_do_method(player, "rename_animation", current, new_name);
			undo_redo->add_do_method(anim.ptr(), "set_name", new_name);
			undo_redo->add_undo_method(player, "rename_animation", new_name, current);
			undo_redo->add_undo_method(anim.ptr(), "set_name", current);
			undo_redo->add_do_method(this, "_animation_player_changed", player);
			undo_redo->add_undo_method(this, "_animation_player_changed", player);
			undo_redo->commit_action();

			_select_anim_by_name(new_name);
		} break;

		case TOOL_NEW_ANIM: {
			Ref<Animation> new_anim = Ref<Animation>(memnew(Animation));
			new_anim->set_name(new_name);
			
			undo_redo->create_action(TTR("Add Animation"));

			bool lib_added = false;
			
			undo_redo->add_do_method(player, "add_animation", new_name, new_anim);
			undo_redo->add_undo_method(player, "remove_animation", new_name);
			undo_redo->add_do_method(this, "_animation_player_changed", player);
			undo_redo->add_undo_method(this, "_animation_player_changed", player);
			if (!animation->get_item_count()) {
				undo_redo->add_do_method(this, "_start_onion_skinning");
				undo_redo->add_undo_method(this, "_stop_onion_skinning");
			}
			if (lib_added) {
				undo_redo->add_undo_method(player, "remove_animation_library", "");
			}
			undo_redo->commit_action();

			_select_anim_by_name(new_name);
		} break;

		case TOOL_DUPLICATE_ANIM: {
			String current = animation->get_item_text(animation->get_selected());
			Ref<Animation> anim = player->get_animation(current);

			Ref<Animation> new_anim = _animation_clone(anim);
			new_anim->set_name(new_name);
			
			undo_redo->create_action(TTR("Duplicate Animation"));
			undo_redo->add_do_method(player, "add_animation", new_name, new_anim);
			undo_redo->add_undo_method(player, "remove_animation", new_name);
			undo_redo->add_do_method(player, "animation_set_next", new_name, player->animation_get_next(current));
			undo_redo->add_do_method(this, "_animation_player_changed", player);
			undo_redo->add_undo_method(this, "_animation_player_changed", player);
			undo_redo->commit_action();

			_select_anim_by_name(new_name);
		} break;
	}

	name_dialog->hide();
}

void PlayerEditor::_blend_editor_next_changed(const int p_idx) {
	if (!animation->get_item_count()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());

	undo_redo->create_action(TTR("Blend Next Changed"));
	undo_redo->add_do_method(player, "animation_set_next", current, blend_editor.next->get_item_text(p_idx));
	undo_redo->add_undo_method(player, "animation_set_next", current, player->animation_get_next(current));
	undo_redo->add_do_method(this, "_animation_player_changed", player);
	undo_redo->add_undo_method(this, "_animation_player_changed", player);
	undo_redo->commit_action();
}

void PlayerEditor::_animation_blend() {
	if (updating_blends) {
		return;
	}

	blend_editor.tree->clear();

	if (!animation->get_item_count()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());

	blend_editor.dialog->popup_centered(Size2(400, 400) * EDSCALE);

	blend_editor.tree->set_hide_root(true);
	blend_editor.tree->set_column_min_width(0, 10);
	blend_editor.tree->set_column_expand(0, true);
	blend_editor.tree->set_column_min_width(1, 3);
	blend_editor.tree->set_column_expand(1, true);

	List<StringName> anims;
	player->get_animation_list(&anims);
	TreeItem *root = blend_editor.tree->create_item();
	updating_blends = true;

	int i = 0;
	bool anim_found = false;
	blend_editor.next->clear();
	blend_editor.next->add_item("", i);
	
	for (List<StringName>::Element* E = anims.front(); E; E = E->next()) {
		TreeItem *blend = blend_editor.tree->create_item(root);
		blend->set_editable(0, false);
		blend->set_editable(1, true);
		blend->set_text(0, E->get());
		blend->set_cell_mode(1, TreeItem::CELL_MODE_RANGE);
		blend->set_range_config(1, 0, 3600, 0.001);
		blend->set_range(1, player->get_blend_time(current, E->get()));

		i++;
		blend_editor.next->add_item(E->get(), i);
		if (E->get() == player->animation_get_next(current)) {
			blend_editor.next->select(i);
			anim_found = true;
		}
	}

	// make sure we reset it else it becomes out of sync and could contain a deleted animation
	if (!anim_found) {
		blend_editor.next->select(0);
		player->animation_set_next(current, blend_editor.next->get_item_text(0));
	}

	updating_blends = false;
}

void PlayerEditor::_blend_edited() {
	if (updating_blends) {
		return;
	}

	if (!animation->get_item_count()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());

	TreeItem *selected = blend_editor.tree->get_edited();
	if (!selected) {
		return;
	}

	updating_blends = true;
	String to = selected->get_text(0);
	float blend_time = selected->get_range(1);
	float prev_blend_time = player->get_blend_time(current, to);

	undo_redo->create_action(TTR("Change Blend Time"));
	undo_redo->add_do_method(player, "set_blend_time", current, to, blend_time);
	undo_redo->add_undo_method(player, "set_blend_time", current, to, prev_blend_time);
	undo_redo->add_do_method(this, "_animation_player_changed", player);
	undo_redo->add_undo_method(this, "_animation_player_changed", player);
	undo_redo->commit_action();
	updating_blends = false;
}

void PlayerEditor::ensure_visibility() {
	if (player && pin->is_pressed()) {
		return; // another player is pinned, don't reset
	}

	_animation_edit();
}

Dictionary PlayerEditor::get_state() const {
	Dictionary d;

	d["visible"] = is_visible_in_tree();
	if (EditorNode::get_singleton()->get_edited_scene() && is_visible_in_tree() && player) {
		d["player"] = EditorNode::get_singleton()->get_edited_scene()->get_path_to(player);
		d["animation"] = player->get_assigned_animation();
		d["track_editor_state"] = track_editor->get_state();
	}

	return d;
}

void PlayerEditor::set_state(const Dictionary &p_state) {
	if (!p_state.has("visible") || !p_state["visible"]) {
		return;
	}
	if (!EditorNode::get_singleton()->get_edited_scene()) {
		return;
	}

	if (p_state.has("player")) {
		Node *n = EditorNode::get_singleton()->get_edited_scene()->get_node(p_state["player"]);
		if (Object::cast_to<AnimationPlayer>(n) && EditorNode::get_singleton()->get_editor_selection()->is_selected(n)) {
			player = Object::cast_to<AnimationPlayer>(n);
			_update_player();
			EditorNode::get_singleton()->make_bottom_panel_item_visible(this);
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

void PlayerEditor::_animation_resource_edit() {
	String current = _get_current();
	if (current != String()) {
		Ref<Animation> anim = player->get_animation(current);
		EditorNode::get_singleton()->edit_resource(anim);
	}
}

void PlayerEditor::_animation_edit() {
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

void PlayerEditor::_scale_changed(const String &p_scale) {
	player->set_speed_scale(p_scale.to_float());
}

void PlayerEditor::_update_animation() {
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

	scale->set_text(String::num(player->get_speed_scale(), 2));
	String current = player->get_assigned_animation();

	for (int i = 0; i < animation->get_item_count(); i++) {
		if (animation->get_item_text(i) == current) {
			animation->select(i);
			break;
		}
	}

	updating = false;
}

void PlayerEditor::_update_player() {
	updating = true;

	animation->clear();

	if (!player) {
		PlayerEditor::get_singleton()->get_track_editor()->update_keying();
		return;
	}
	
	int active_idx = -1;
	bool no_anims_found = true;
	
#define ITEM_CHECK_DISABLED(m_item) tool_anim->get_popup()->set_item_disabled(tool_anim->get_popup()->get_item_index(m_item), no_anims_found)

	ITEM_CHECK_DISABLED(TOOL_DUPLICATE_ANIM);
	ITEM_CHECK_DISABLED(TOOL_RENAME_ANIM);
	ITEM_CHECK_DISABLED(TOOL_EDIT_TRANSITIONS);
	ITEM_CHECK_DISABLED(TOOL_REMOVE_ANIM);
	ITEM_CHECK_DISABLED(TOOL_EDIT_RESOURCE);

#undef ITEM_CHECK_DISABLED

	stop->set_disabled(no_anims_found);
	play->set_disabled(no_anims_found);
	play_bw->set_disabled(no_anims_found);
	play_bw_from->set_disabled(no_anims_found);
	play_from->set_disabled(no_anims_found);
	frame->set_editable(!no_anims_found);
	animation->set_disabled(no_anims_found);
	autoplay->set_disabled(no_anims_found);
	tool_anim->set_disabled(player == nullptr);
	onion_toggle->set_disabled(no_anims_found);
	onion_skinning->set_disabled(no_anims_found);
	pin->set_disabled(player == nullptr);

	_update_animation_list_icons();

	updating = false;
	if (active_idx != -1) {
		animation->select(active_idx);
		autoplay->set_pressed(animation->get_item_text(active_idx) == player->get_autoplay());
		_animation_selected(active_idx);
	} else if (animation->get_item_count()) {
		int item = animation->get_item_id(0);
		animation->select(item);
		autoplay->set_pressed(animation->get_item_text(item) == player->get_autoplay());
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

void PlayerEditor::_update_animation_list_icons() {
	for (int i = 0; i < animation->get_item_count(); i++) {
		String name = animation->get_item_text(i);
		if (animation->is_item_disabled(i)) {
			continue;
		}

		Ref<Texture> icon;
		if (name == player->get_autoplay()) {
			if (name == "RESET") {
				icon = autoplay_reset_icon;
			} else {
				icon = autoplay_icon;
			}
		} else if (name == "RESET") {
			icon = reset_icon;
		}

		animation->set_item_icon(i, icon);
	}
}

void PlayerEditor::edit(AnimationPlayer *p_player) {
	if (player && pin->is_pressed()) {
		return; // Ignore, pinned.
	}
	player = p_player;

	if (player) {
		_update_player();

		if (onion.enabled) {
			if (animation->get_item_count()) {
				_start_onion_skinning();
			} else {
				_stop_onion_skinning();
			}
		}

		track_editor->show_select_node_warning(false);
	} else {
		if (onion.enabled) {
			_stop_onion_skinning();
		}

		track_editor->show_select_node_warning(true);
	}
}

void PlayerEditor::forward_force_draw_over_viewport(Control *p_overlay) {
	if (!onion.can_overlay) {
		return;
	}

	// Can happen on viewport resize, at least.
	if (!_are_onion_layers_valid()) {
		return;
	}

	RID ci = p_overlay->get_canvas_item();
	Rect2 src_rect = p_overlay->get_global_rect();
	// Re-flip since captures are already flipped.
	src_rect.position.y = onion.capture_size.y - (src_rect.position.y + src_rect.size.y);
	src_rect.size.y *= -1;

	Rect2 dst_rect = Rect2(Point2(), p_overlay->get_size());

	float alpha_step = 1.0 / (onion.steps + 1);

	int cidx = 0;
	if (onion.past) {
		float alpha = 0;
		do {
			alpha += alpha_step;

			if (onion.captures_valid[cidx]) {
				VisualServer::get_singleton()->canvas_item_add_texture_rect_region(
						ci, dst_rect, VisualServer::get_singleton()->viewport_get_texture(onion.captures[cidx]), src_rect, Color(1, 1, 1, alpha));
			}

			cidx++;
		} while (cidx < onion.steps);
	}
	if (onion.future) {
		float alpha = 1;
		int base_cidx = cidx;
		do {
			alpha -= alpha_step;

			if (onion.captures_valid[cidx]) {
				VisualServer::get_singleton()->canvas_item_add_texture_rect_region(
						ci, dst_rect, VisualServer::get_singleton()->viewport_get_texture(onion.captures[cidx]), src_rect, Color(1, 1, 1, alpha));
			}

			cidx++;
		} while (cidx < base_cidx + onion.steps); // In case there's the present capture at the end, skip it.
	}
}

void PlayerEditor::_animation_duplicate() {
	if (!animation->get_item_count()) {
		return;
	}

	String current = animation->get_item_text(animation->get_selected());
	Ref<Animation> anim = player->get_animation(current);
	if (!anim.is_valid()) {
		return;
	}

	String new_name = current;
	while (player->has_animation(new_name)) {
		new_name = new_name + " (copy)";
	}

	name_title->set_text(TTR("New Animation Name:"));
	name->set_text(new_name);
	name_dialog_op = TOOL_DUPLICATE_ANIM;
	name_dialog->popup_centered(Size2(300, 90));
	name->select_all();
	name->grab_focus();
}

Ref<Animation> PlayerEditor::_animation_clone(Ref<Animation> p_anim) {
	Ref<Animation> new_anim = memnew(Animation);
	List<PropertyInfo> plist;
	p_anim->get_property_list(&plist);

	for (List<PropertyInfo > ::Element* E = plist.front(); E; E = E->next()) {
		if (E->get().usage & PROPERTY_USAGE_STORAGE) {
			new_anim->set(E->get().name, p_anim->get(E->get().name));
		}
	}
	new_anim->set_path("");

	return new_anim;
}

void PlayerEditor::_seek_value_changed(float p_value, bool p_set, bool p_timeline_only) {
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

void PlayerEditor::_animation_player_changed(Object *p_pl) {
	if (player == p_pl && is_visible_in_tree()) {
		_update_player();
		if (blend_editor.dialog->is_visible()) {
			_animation_blend(); // Update.
		}
	}
}

void PlayerEditor::_list_changed() {
	if (is_visible_in_tree()) {
		_update_player();
	}
}

void PlayerEditor::_animation_key_editor_anim_len_changed(float p_len) {
	frame->set_max(p_len);
}

void PlayerEditor::_animation_key_editor_seek(float p_pos, bool p_drag, bool p_timeline_only) {
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

void PlayerEditor::_animation_tool_menu(int p_option) {
	String current = _get_current();

	Ref<Animation> anim;
	if (!current.empty()) {
		anim = player->get_animation(current);
	}

	switch (p_option) {
		case TOOL_NEW_ANIM: {
			_animation_new();
		} break;
		case TOOL_DUPLICATE_ANIM: {
			_animation_duplicate();
		} break;
		case TOOL_RENAME_ANIM: {
			_animation_rename();
		} break;
		case TOOL_EDIT_TRANSITIONS: {
			_animation_blend();
		} break;
		case TOOL_REMOVE_ANIM: {
			_animation_remove();
		} break;
		case TOOL_EDIT_RESOURCE: {
			if (anim.is_valid()) {
				EditorNode::get_singleton()->edit_resource(anim);
			}
		} break;
	}
}

void PlayerEditor::_onion_skinning_menu(int p_option) {
	PopupMenu *menu = onion_skinning->get_popup();
	int idx = menu->get_item_index(p_option);

	switch (p_option) {
		case ONION_SKINNING_ENABLE: {
			onion.enabled = !onion.enabled;

			if (onion.enabled) {
				_start_onion_skinning();
			} else {
				_stop_onion_skinning();
			}

		} break;
		case ONION_SKINNING_PAST: {
			// Ensure at least one of past/future is checked.
			onion.past = onion.future ? !onion.past : true;
			menu->set_item_checked(idx, onion.past);
		} break;
		case ONION_SKINNING_FUTURE: {
			// Ensure at least one of past/future is checked.
			onion.future = onion.past ? !onion.future : true;
			menu->set_item_checked(idx, onion.future);
		} break;
		case ONION_SKINNING_1_STEP: // Fall-through.
		case ONION_SKINNING_2_STEPS:
		case ONION_SKINNING_3_STEPS: {
			onion.steps = (p_option - ONION_SKINNING_1_STEP) + 1;
			int one_frame_idx = menu->get_item_index(ONION_SKINNING_1_STEP);
			for (int i = 0; i <= ONION_SKINNING_LAST_STEPS_OPTION - ONION_SKINNING_1_STEP; i++) {
				menu->set_item_checked(one_frame_idx + i, onion.steps == i + 1);
			}
		} break;
		case ONION_SKINNING_DIFFERENCES_ONLY: {
			onion.differences_only = !onion.differences_only;
			menu->set_item_checked(idx, onion.differences_only);
		} break;
		case ONION_SKINNING_FORCE_WHITE_MODULATE: {
			onion.force_white_modulate = !onion.force_white_modulate;
			menu->set_item_checked(idx, onion.force_white_modulate);
		} break;
		case ONION_SKINNING_INCLUDE_GIZMOS: {
			onion.include_gizmos = !onion.include_gizmos;
			menu->set_item_checked(idx, onion.include_gizmos);
		} break;
	}
}

void PlayerEditor::_unhandled_key_input(const Ref<InputEvent> &p_ev) {
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

void PlayerEditor::_editor_visibility_changed() {
	if (is_visible() && animation->get_item_count()) {
		_start_onion_skinning();
	}
}

bool PlayerEditor::_are_onion_layers_valid() {
	return false;
}

void PlayerEditor::_allocate_onion_layers() {
	return;
}

void PlayerEditor::_prepare_onion_layers_1() {
	return;
}

void PlayerEditor::_prepare_onion_layers_1_deferred() {
	call_deferred("_prepare_onion_layers_1");
}

void PlayerEditor::_prepare_onion_layers_2() {
	return;
}

void PlayerEditor::_start_onion_skinning() {
	return;
}

void PlayerEditor::_stop_onion_skinning() {
	return;
}

void PlayerEditor::_pin_pressed() {
	//SceneTreeDock::get_singleton()->get_tree_editor()->update_tree();
}

void PlayerEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_animation_new"), &PlayerEditor::_animation_new);
	ClassDB::bind_method(D_METHOD("_animation_rename"), &PlayerEditor::_animation_rename);
	ClassDB::bind_method(D_METHOD("_animation_remove"), &PlayerEditor::_animation_remove);
	ClassDB::bind_method(D_METHOD("_animation_blend"), &PlayerEditor::_animation_blend);
	ClassDB::bind_method(D_METHOD("_animation_edit"), &PlayerEditor::_animation_edit);
	ClassDB::bind_method(D_METHOD("_animation_resource_edit"), &PlayerEditor::_animation_resource_edit);
	ClassDB::bind_method(D_METHOD("_animation_player_changed"), &PlayerEditor::_animation_player_changed);
	ClassDB::bind_method(D_METHOD("_list_changed"), &PlayerEditor::_list_changed);
	ClassDB::bind_method(D_METHOD("_animation_duplicate"), &PlayerEditor::_animation_duplicate);
}

PlayerEditor *PlayerEditor::singleton = nullptr;

AnimationPlayer *PlayerEditor::get_player() const {
	return player;
}

PlayerEditor::PlayerEditor() {
	singleton = this;

	updating = false;

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

	scale = memnew(LineEdit);
	hb->add_child(scale);
	scale->set_h_size_flags(SIZE_EXPAND_FILL);
	scale->set_stretch_ratio(1);
	scale->set_tooltip(TTR("Scale animation playback globally for the node."));
	scale->hide();

	delete_dialog = memnew(ConfirmationDialog);
	add_child(delete_dialog);
	delete_dialog->connect("confirmed", this, "_animation_remove_confirmed");

	tool_anim = memnew(MenuButton);
	//tool_anim->set_shortcut_context(this);
	tool_anim->set_flat(false);
	tool_anim->set_tooltip(TTR("Animation Tools"));
	tool_anim->set_text(TTR("Animation"));
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/new_animation", TTR("New")), TOOL_NEW_ANIM);
	tool_anim->get_popup()->add_separator();
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/animation_libraries", TTR("Manage Animations...")), TOOL_ANIM_LIBRARY);
	tool_anim->get_popup()->add_separator();
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/duplicate_animation", TTR("Duplicate...")), TOOL_DUPLICATE_ANIM);
	tool_anim->get_popup()->add_separator();
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/rename_animation", TTR("Rename...")), TOOL_RENAME_ANIM);
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/edit_transitions", TTR("Edit Transitions...")), TOOL_EDIT_TRANSITIONS);
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/open_animation_in_inspector", TTR("Open in Inspector")), TOOL_EDIT_RESOURCE);
	tool_anim->get_popup()->add_separator();
	tool_anim->get_popup()->add_shortcut(ED_SHORTCUT("animation_player_editor/remove_animation", TTR("Remove")), TOOL_REMOVE_ANIM);
	tool_anim->set_disabled(true);
	hb->add_child(tool_anim);

	animation = memnew(OptionButton);
	hb->add_child(animation);
	animation->set_h_size_flags(SIZE_EXPAND_FILL);
	animation->set_tooltip(TTR("Display list of animations in player."));
	animation->set_clip_text(true);

	autoplay = memnew(Button);
	autoplay->set_flat(true);
	hb->add_child(autoplay);
	autoplay->set_tooltip(TTR("Autoplay on Load"));

	hb->add_child(memnew(VSeparator));

	track_editor = memnew(AnimationTrackEditor);

	hb->add_child(track_editor->get_edit_menu());

	hb->add_child(memnew(VSeparator));

	onion_toggle = memnew(Button);
	onion_toggle->set_flat(true);
	onion_toggle->set_toggle_mode(true);
	onion_toggle->set_tooltip(TTR("Enable Onion Skinning"));
	onion_toggle->connect("pressed", this, "_onion_skinning_menu"), varray(ONION_SKINNING_ENABLE);
	hb->add_child(onion_toggle);

	onion_skinning = memnew(MenuButton);
	onion_skinning->set_tooltip(TTR("Onion Skinning Options"));
	onion_skinning->get_popup()->add_separator(TTR("Directions"));
	onion_skinning->get_popup()->add_check_item(TTR("Past"), ONION_SKINNING_PAST);
	onion_skinning->get_popup()->set_item_checked(-1, true);
	onion_skinning->get_popup()->add_check_item(TTR("Future"), ONION_SKINNING_FUTURE);
	onion_skinning->get_popup()->add_separator(TTR("Depth"));
	onion_skinning->get_popup()->add_radio_check_item(TTR("1 step"), ONION_SKINNING_1_STEP);
	onion_skinning->get_popup()->set_item_checked(-1, true);
	onion_skinning->get_popup()->add_radio_check_item(TTR("2 steps"), ONION_SKINNING_2_STEPS);
	onion_skinning->get_popup()->add_radio_check_item(TTR("3 steps"), ONION_SKINNING_3_STEPS);
	onion_skinning->get_popup()->add_separator();
	onion_skinning->get_popup()->add_check_item(TTR("Differences Only"), ONION_SKINNING_DIFFERENCES_ONLY);
	onion_skinning->get_popup()->add_check_item(TTR("Force White Modulate"), ONION_SKINNING_FORCE_WHITE_MODULATE);
	onion_skinning->get_popup()->add_check_item(TTR("Include Gizmos (3D)"), ONION_SKINNING_INCLUDE_GIZMOS);
	hb->add_child(onion_skinning);

	hb->add_child(memnew(VSeparator));

	pin = memnew(Button);
	pin->set_flat(true);
	pin->set_toggle_mode(true);
	pin->set_tooltip(TTR("Pin AnimationPlayer"));
	hb->add_child(pin);
	pin->connect("pressed", this, "_pin_pressed");

	file = memnew(EditorFileDialog);
	add_child(file);

	name_dialog = memnew(ConfirmationDialog);
	name_dialog->set_title(TTR("Create New Animation"));
	name_dialog->set_hide_on_ok(false);
	add_child(name_dialog);
	VBoxContainer *vb = memnew(VBoxContainer);
	name_dialog->add_child(vb);

	name_title = memnew(Label(TTR("Animation Name:")));
	vb->add_child(name_title);

	HBoxContainer *name_hb = memnew(HBoxContainer);
	name = memnew(LineEdit);
	name_hb->add_child(name);
	name->set_h_size_flags(SIZE_EXPAND_FILL);
	library = memnew(OptionButton);
	name_hb->add_child(library);
	library->hide();
	vb->add_child(name_hb);
	name_dialog->register_text_enter(name);

	error_dialog = memnew(ConfirmationDialog);
	error_dialog->get_ok()->set_text(TTR("Close"));
	error_dialog->set_title(TTR("Error!"));
	add_child(error_dialog);

	name_dialog->connect("confirmed", this, "_animation_name_edited");

	blend_editor.dialog = memnew(AcceptDialog);
	add_child(blend_editor.dialog);
	blend_editor.dialog->get_ok()->set_text(TTR("Close"));
	blend_editor.dialog->set_hide_on_ok(true);
	VBoxContainer *blend_vb = memnew(VBoxContainer);
	blend_editor.dialog->add_child(blend_vb);
	blend_editor.tree = memnew(Tree);
	blend_editor.tree->set_columns(2);
	blend_vb->add_margin_child(TTR("Blend Times:"), blend_editor.tree, true);
	blend_editor.next = memnew(OptionButton);
	blend_vb->add_margin_child(TTR("Next (Auto Queue):"), blend_editor.next);
	blend_editor.dialog->set_title(TTR("Cross-Animation Blend Times"));
	updating_blends = false;

	blend_editor.tree->connect("item_edited", this, "_blend_edited");

	autoplay->connect("pressed", this, "_autoplay_pressed");
	autoplay->set_toggle_mode(true);
	play->connect("pressed", this, "_play_pressed");
	play_from->connect("pressed", this, "_play_from_pressed");
	play_bw->connect("pressed", this, "_play_bw_pressed");
	play_bw_from->connect("pressed", this, "_play_bw_from_pressed");
	stop->connect("pressed", this, "_stop_pressed");

	animation->connect("item_selected", this, "_animation_selected");

	frame->connect("value_changed", this, "_seek_value_changed", make_binds(true, false));
	scale->connect("text_submitted", this, "_scale_changed");

	last_active = false;
	timeline_position = 0;

	set_process_unhandled_key_input(true);

	add_child(track_editor);
	track_editor->set_v_size_flags(SIZE_EXPAND_FILL);
	track_editor->connect("timeline_changed", this, "_animation_key_editor_seek");
	track_editor->connect("animation_len_changed", this, "_animation_key_editor_anim_len_changed");

	_update_player();
}

PlayerEditor::~PlayerEditor() {
}
