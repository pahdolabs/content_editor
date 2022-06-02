#include "track_editor.h"

#include "core/os/input.h"
#include "scene/scene_string_names.h"
#include "scene/animation/animation_player.h"
#include "scene/main/viewport.h"
#include "core/os/keyboard.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/separator.h"
#include "scene/3d/spatial.h"

#include "../consts.h"
#include "../icons_cache.h"
#include "player_editor_control.h"
#include "../track_edit_plugins/track_edit.h"
#include "timeline_edit.h"
#include "../track_edit_plugins/track_edit_default_plugin.h"
#include "../track_edit_plugins/track_edit_plugin.h"
#include "track_key_edit.h"
#include "view_panner.h"

void TrackEditor::add_track_edit_plugin(const Ref<TrackEditPlugin>& p_plugin) {
	if (track_edit_plugins.find(p_plugin) != -1) {
		return;
	}
	track_edit_plugins.push_back(p_plugin);
}

void TrackEditor::remove_track_edit_plugin(const Ref<TrackEditPlugin>& p_plugin) {
	track_edit_plugins.erase(p_plugin);
}

void TrackEditor::set_animation(const Ref<Animation>& p_anim) {
	if (animation != p_anim && _get_track_selected() >= 0) {
		track_edits[_get_track_selected()]->release_focus();
	}
	if (animation.is_valid()) {
		animation->disconnect("changed", this, "_animation_changed");
		_clear_selection();
	}
	animation = p_anim;
	timeline->set_animation(p_anim);

	_update_tracks();

	if (animation.is_valid()) {
		animation->connect("changed", this, "_animation_changed");

		hscroll->show();
		step->set_block_signals(true);

		_update_step_spinbox();
		step->set_block_signals(false);
		//step->set_read_only(false);
		snap->set_disabled(false);
		snap_mode->set_disabled(false);

		imported_anim_warning->hide();
		bool import_warning_done = false;
		bool bezier_done = false;
		for (int i = 0; i < animation->get_track_count(); i++) {
			if (animation->track_is_imported(i)) {
				imported_anim_warning->show();
				import_warning_done = true;
			}
			if (animation->track_get_type(i) == Animation::TrackType::TYPE_BEZIER) {
				bezier_done = true;
			}
			if (import_warning_done && bezier_done) {
				break;
			}
		}

	}
	else {
		hscroll->hide();
		step->set_block_signals(true);
		step->set_value(0);
		step->set_block_signals(false);
		//step->set_read_only(true);
		snap->set_disabled(true);
		snap_mode->set_disabled(true);
	}
}

Ref<Animation> TrackEditor::get_current_animation() const {
	return animation;
}

void TrackEditor::_root_removed(Node* p_root) {
	root = nullptr;
}

void TrackEditor::set_root(Node* p_root) {
	if (root) {
		root->disconnect("tree_exiting", this, "_root_removed");
	}

	root = p_root;

	if (root) {
		root->connect("tree_exiting", this, "_root_removed", make_binds(), CONNECT_ONESHOT);
	}

	_update_tracks();
}

Node* TrackEditor::get_root() const {
	return root;
}

void TrackEditor::update_keying() {
	bool keying_enabled = false;

	/*EditorSelectionHistory* editor_history = EditorNode::get_singleton()->get_editor_selection_history();
	if (is_visible_in_tree() && animation.is_valid() && editor_history->get_path_size() > 0) {
		Object* obj = ObjectDB::get_instance(editor_history->get_path_object(0));
		keying_enabled = Object::cast_to<Node>(obj) != nullptr;
	}*/

	if (keying_enabled == keying) {
		return;
	}

	keying = keying_enabled;

	emit_signal("keying_changed");
}

bool TrackEditor::has_keying() const {
	return keying;
}

Dictionary TrackEditor::get_state() const {
	Dictionary state;
	state["fps_mode"] = timeline->is_using_fps();
	state["zoom"] = zoom->get_value();
	state["offset"] = timeline->get_value();
	state["v_scroll"] = scroll->get_v_scrollbar()->get_value();
	return state;
}

void TrackEditor::set_state(const Dictionary& p_state) {
	if (p_state.has("fps_mode")) {
		bool fps_mode = p_state["fps_mode"];
		if (fps_mode) {
			snap_mode->select(1);
		}
		else {
			snap_mode->select(0);
		}
		_snap_mode_changed(snap_mode->get_selected());
	}
	else {
		snap_mode->select(0);
		_snap_mode_changed(snap_mode->get_selected());
	}
	if (p_state.has("zoom")) {
		zoom->set_value(p_state["zoom"]);
	}
	else {
		zoom->set_value(1.0);
	}
	if (p_state.has("offset")) {
		timeline->set_value(p_state["offset"]);
	}
	else {
		timeline->set_value(0);
	}
	if (p_state.has("v_scroll")) {
		scroll->get_v_scrollbar()->set_value(p_state["v_scroll"]);
	}
	else {
		scroll->get_v_scrollbar()->set_value(0);
	}
}

void TrackEditor::cleanup() {
	set_animation(Ref<Animation>());
}

void TrackEditor::_name_limit_changed() {
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}
}

void TrackEditor::_timeline_changed(float p_new_pos, bool p_drag, bool p_timeline_only) {
	emit_signal("timeline_changed", p_new_pos, p_drag, p_timeline_only);
}

void TrackEditor::_track_remove_request(int p_track) {
	int idx = p_track;
	if (idx >= 0 && idx < animation->get_track_count()) {
		undo_redo->create_action(TTR("Remove Anim Track"));
		undo_redo->add_do_method(this, "_clear_selection", false);
		undo_redo->add_do_method(animation.ptr(), "remove_track", idx);
		undo_redo->add_undo_method(animation.ptr(), "add_track", animation->track_get_type(idx), idx);
		undo_redo->add_undo_method(animation.ptr(), "track_set_path", idx, animation->track_get_path(idx));

		// TODO interpolation.
		for (int i = 0; i < animation->track_get_key_count(idx); i++) {
			Variant v = animation->track_get_key_value(idx, i);
			float time = animation->track_get_key_time(idx, i);
			float trans = animation->track_get_key_transition(idx, i);

			undo_redo->add_undo_method(animation.ptr(), "track_insert_key", idx, time, v);
			undo_redo->add_undo_method(animation.ptr(), "track_set_key_transition", idx, i, trans);
		}

		undo_redo->add_undo_method(animation.ptr(), "track_set_interpolation_type", idx, animation->track_get_interpolation_type(idx));
		if (animation->track_get_type(idx) == Animation::TYPE_VALUE) {
			undo_redo->add_undo_method(animation.ptr(), "value_track_set_update_mode", idx, animation->value_track_get_update_mode(idx));
		}

		undo_redo->commit_action();
	}
}

void TrackEditor::_track_grab_focus(int p_track) {
	// Don't steal focus if not working with the track editor.
	/*if (Object::cast_to<TrackEdit>(get_viewport()->gui_get_focus_owner())) {
		track_edits[p_track]->grab_focus();
	}*/
}

void TrackEditor::set_anim_pos(float p_pos) {
	timeline->set_play_position(p_pos);
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->set_play_position(p_pos);
	}
}

static bool track_type_is_resettable(Animation::TrackType p_type) {
	switch (p_type) {
	case Animation::TYPE_VALUE:
		return true;
	default:
		return false;
	}
}

void TrackEditor::make_insert_queue() {
	insert_data.clear();
	insert_queue = true;
}

void TrackEditor::commit_insert_queue() {
	bool reset_allowed = true;
	AnimationPlayer* player = PlayerEditorControl::get_singleton()->get_player();
	if (player->has_animation("RESET") && player->get_animation("RESET") == animation) {
		// Avoid messing with the reset animation itself.
		reset_allowed = false;
	}
	else {
		bool some_resettable = false;
		for (int i = 0; i < insert_data.size(); i++) {
			if (track_type_is_resettable(insert_data[i].type)) {
				some_resettable = true;
				break;
			}
		}
		if (!some_resettable) {
			reset_allowed = false;
		}
	}

	// Organize insert data.
	int num_tracks = 0;
	String last_track_query;
	bool all_bezier = true;
	for (int i = 0; i < insert_data.size(); i++) {
		if (insert_data[i].type != Animation::TYPE_VALUE && insert_data[i].type != Animation::TYPE_BEZIER) {
			all_bezier = false;
		}

		if (insert_data[i].track_idx == -1) {
			++num_tracks;
			last_track_query = insert_data[i].query;
		}

		if (insert_data[i].type != Animation::TYPE_VALUE) {
			continue;
		}

		switch (insert_data[i].value.get_type()) {
		case Variant::INT:
		case Variant::REAL:
		case Variant::VECTOR2:
		case Variant::VECTOR3:
		case Variant::QUAT:
		case Variant::PLANE:
		case Variant::COLOR: {
			// Valid.
		} break;
		default: {
			all_bezier = false;
		}
		}
	}

	if (num_tracks > 0) {
		// Potentially a new key, does not exist.
		if (num_tracks == 1) {
			// TRANSLATORS: %s will be replaced by a phrase describing the target of track.
			insert_confirm_text->set_text(vformat(TTR("Create new track for %s and insert key?"), last_track_query));
		}
		else {
			insert_confirm_text->set_text(vformat(TTR("Create %d new tracks and insert keys?"), num_tracks));
		}

		insert_confirm_bezier->set_visible(all_bezier);
		insert_confirm_reset->set_visible(reset_allowed);

		insert_confirm->get_ok()->set_text(TTR("Create"));
		insert_confirm->popup_centered();
	}
	else {
		_insert_track(reset_allowed, all_bezier);
	}

	insert_queue = false;
}

void TrackEditor::_query_insert(const InsertData& p_id) {
	if (!insert_queue) {
		insert_data.clear();
	}

	for (List<InsertData>::Element* E = insert_data.front(); E; E = E->next()) {
		// Prevent insertion of multiple tracks.
		if (E->get().path == p_id.path && E->get().type == p_id.type) {
			return; // Already inserted a track this frame.
		}
	}

	insert_data.push_back(p_id);

	// Without queue, commit immediately.
	if (!insert_queue) {
		commit_insert_queue();
	}
}

void TrackEditor::_insert_track(bool p_create_reset, bool p_create_beziers) {
	undo_redo->create_action(TTR("Anim Insert"));

	Ref<Animation> reset_anim;
	if (p_create_reset) {
		reset_anim = _create_and_get_reset_animation();
	}

	TrackIndices next_tracks(animation.ptr(), reset_anim.ptr());
	bool advance = false;
	while (insert_data.size()) {
		if (insert_data.front()->get().advance) {
			advance = true;
		}
		next_tracks = _confirm_insert(insert_data.front()->get(), next_tracks, p_create_reset, reset_anim, p_create_beziers);
		insert_data.pop_front();
	}

	undo_redo->commit_action();

	if (advance) {
		float step = animation->get_step();
		if (step == 0) {
			step = 1;
		}

		float pos = timeline->get_play_position();

		pos = Math::stepify(pos + step, step);
		if (pos > animation->get_length()) {
			pos = animation->get_length();
		}
		set_anim_pos(pos);
		emit_signal("timeline_changed", pos, true, false);
	}
}

void TrackEditor::insert_transform_key(Spatial* p_node, const String& p_sub, const Animation::TrackType p_type, const Variant p_value) {
	ERR_FAIL_COND(!root);
	if (!keying) {
		return;
	}
	if (!animation.is_valid()) {
		return;
	}

	// Let's build a node path.
	String path = root->get_path_to(p_node);
	if (!p_sub.empty()) {
		path += ":" + p_sub;
	}

	NodePath np = path;

	int track_idx = -1;

	for (int i = 0; i < animation->get_track_count(); i++) {
		if (animation->track_get_path(i) != np) {
			continue;
		}
		if (animation->track_get_type(i) != p_type) {
			continue;
		}
		track_idx = i;
	}

	InsertData id;
	id.path = np;
	// TRANSLATORS: This describes the target of new animation track, will be inserted into another string.
	id.query = vformat(TTR("node '%s'"), p_node->get_name());
	id.advance = false;
	id.track_idx = track_idx;
	id.value = p_value;
	id.type = p_type;
	_query_insert(id);
}

bool TrackEditor::has_track(Spatial* p_node, const String& p_sub, const Animation::TrackType p_type) {
	ERR_FAIL_COND_V(!root, false);
	if (!keying) {
		return false;
	}
	if (!animation.is_valid()) {
		return false;
	}

	// Let's build a node path.
	String path = root->get_path_to(p_node);
	if (!p_sub.empty()) {
		path += ":" + p_sub;
	}

	int track_id = animation->find_track(path);
	if (track_id >= 0) {
		return true;
	}
	return false;
}

void TrackEditor::_insert_animation_key(NodePath p_path, const Variant& p_value) {
	String path = p_path;

	// Animation property is a special case, always creates an animation track.
	for (int i = 0; i < animation->get_track_count(); i++) {
		String np = animation->track_get_path(i);

		if (path == np && animation->track_get_type(i) == Animation::TYPE_ANIMATION) {
			// Exists.
			InsertData id;
			id.path = path;
			id.track_idx = i;
			id.value = p_value;
			id.type = Animation::TYPE_ANIMATION;
			// TRANSLATORS: This describes the target of new animation track, will be inserted into another string.
			id.query = TTR("animation");
			id.advance = false;
			// Dialog insert.
			_query_insert(id);
			return;
		}
	}

	InsertData id;
	id.path = path;
	id.track_idx = -1;
	id.value = p_value;
	id.type = Animation::TYPE_ANIMATION;
	id.query = TTR("animation");
	id.advance = false;
	// Dialog insert.
	_query_insert(id);
}

void TrackEditor::insert_node_value_key(Node* p_node, const String& p_property, const Variant& p_value, bool p_only_if_exists) {
	ERR_FAIL_COND(!root);
	// Let's build a node path.

	Node* node = p_node;

	String path = root->get_path_to(node);

	if (Object::cast_to<AnimationPlayer>(node) && p_property == "current_animation") {
		if (node == PlayerEditorControl::get_singleton()->get_player()) {
			//EditorNode::get_singleton()->show_warning(TTR("AnimationPlayer can't animate itself, only other players."));
			return;
		}
		_insert_animation_key(path, p_value);
		return;
	}

	/*EditorSelectionHistory* history = EditorNode::get_singleton()->get_editor_selection_history();
	for (int i = 1; i < history->get_path_size(); i++) {
		String prop = history->get_path_property(i);
		ERR_FAIL_COND(prop.empty());
		path += ":" + prop;
	}*/

	path += ":" + p_property;

	NodePath np = path;

	// Locate track.

	bool inserted = false;

	for (int i = 0; i < animation->get_track_count(); i++) {
		if (animation->track_get_type(i) == Animation::TYPE_VALUE) {
			if (animation->track_get_path(i) != np) {
				continue;
			}

			InsertData id;
			id.path = np;
			id.track_idx = i;
			id.value = p_value;
			id.type = Animation::TYPE_VALUE;
			// TRANSLATORS: This describes the target of new animation track, will be inserted into another string.
			id.query = vformat(TTR("property '%s'"), p_property);
			id.advance = false;
			// Dialog insert.
			_query_insert(id);
			inserted = true;
		}
		else if (animation->track_get_type(i) == Animation::TYPE_BEZIER) {
			Variant value;
			String track_path = animation->track_get_path(i);
			if (track_path == np) {
				value = p_value; // All good.
			}
			else {
				int sep = track_path.rfind(":");
				if (sep != -1) {
					String base_path = track_path.substr(0, sep);
					if (base_path == np) {
						String value_name = track_path.substr(sep + 1);
						value = p_value.get(value_name);
					}
					else {
						continue;
					}
				}
				else {
					continue;
				}
			}

			InsertData id;
			id.path = animation->track_get_path(i);
			id.track_idx = i;
			id.value = value;
			id.type = Animation::TYPE_BEZIER;
			id.query = vformat(TTR("property '%s'"), p_property);
			id.advance = false;
			// Dialog insert.
			_query_insert(id);
			inserted = true;
		}
	}

	if (inserted || p_only_if_exists) {
		return;
	}
	InsertData id;
	id.path = np;
	id.track_idx = -1;
	id.value = p_value;
	id.type = Animation::TYPE_VALUE;
	id.query = vformat(TTR("property '%s'"), p_property);
	id.advance = false;
	// Dialog insert.
	_query_insert(id);
}

Ref<Animation> TrackEditor::_create_and_get_reset_animation() {
	AnimationPlayer* player = PlayerEditorControl::get_singleton()->get_player();
	if (player->has_animation("RESET")) {
		return player->get_animation("RESET");
	}
	else {
		Ref<Animation> reset_anim;
		reset_anim.instance();
		reset_anim->set_length(ANIM_MIN_LENGTH);
		undo_redo->add_do_method(player, "add_animation", "RESET", reset_anim);
		undo_redo->add_do_method(PlayerEditorControl::get_singleton(), "_animation_player_changed", player);
		undo_redo->add_undo_method(player, "remove_animation", "RESET");
		undo_redo->add_undo_method(PlayerEditorControl::get_singleton(), "_animation_player_changed", player);
		return reset_anim;
	}
}

void TrackEditor::_confirm_insert_list() {
	undo_redo->create_action(TTR("Anim Create & Insert"));

	bool create_reset = insert_confirm_reset->is_visible() && insert_confirm_reset->is_pressed();
	Ref<Animation> reset_anim;
	if (create_reset) {
		reset_anim = _create_and_get_reset_animation();
	}

	TrackIndices next_tracks(animation.ptr(), reset_anim.ptr());
	while (insert_data.size()) {
		next_tracks = _confirm_insert(insert_data.front()->get(), next_tracks, create_reset, reset_anim, insert_confirm_bezier->is_pressed());
		insert_data.pop_front();
	}

	undo_redo->commit_action();
}

PropertyInfo TrackEditor::_find_hint_for_track(int p_idx, NodePath& r_base_path, Variant* r_current_val) {
	r_base_path = NodePath();
	ERR_FAIL_COND_V(!animation.is_valid(), PropertyInfo());
	ERR_FAIL_INDEX_V(p_idx, animation->get_track_count(), PropertyInfo());

	if (!root) {
		return PropertyInfo();
	}

	NodePath path = animation->track_get_path(p_idx);

	if (!root->has_node_and_resource(path)) {
		return PropertyInfo();
	}

	RES res;
	Vector<StringName> leftover_path;
	Node* node = root->get_node_and_resource(path, res, leftover_path, true);

	if (node) {
		r_base_path = node->get_path();
	}

	if (leftover_path.empty()) {
		if (r_current_val) {
			if (res.is_valid()) {
				*r_current_val = res;
			}
			else if (node) {
				*r_current_val = node;
			}
		}
		return PropertyInfo();
	}

	Variant property_info_base;
	if (res.is_valid()) {
		property_info_base = res;
		if (r_current_val) {
			*r_current_val = res->get_indexed(leftover_path);
		}
	}
	else if (node) {
		property_info_base = node;
		if (r_current_val) {
			*r_current_val = node->get_indexed(leftover_path);
		}
	}

	for (int i = 0; i < leftover_path.size() - 1; i++) {
		bool valid;
		property_info_base = property_info_base.get_named(leftover_path[i], &valid);
	}

	List<PropertyInfo> pinfo;
	property_info_base.get_property_list(&pinfo);

	for (List<PropertyInfo>::Element* E = pinfo.front(); E; E = E->next()) {
		if (E->get().name == leftover_path[leftover_path.size() - 1]) {
			return E->get();
		}
	}

	return PropertyInfo();
}

static Vector<String> _get_bezier_subindices_for_type(Variant::Type p_type, bool* r_valid = nullptr) {
	Vector<String> subindices;
	if (r_valid) {
		*r_valid = true;
	}
	switch (p_type) {
	case Variant::INT: {
		subindices.push_back("");
	} break;
	case Variant::REAL: {
		subindices.push_back("");
	} break;
	case Variant::VECTOR2: {
		subindices.push_back(":x");
		subindices.push_back(":y");
	} break;
	case Variant::VECTOR3: {
		subindices.push_back(":x");
		subindices.push_back(":y");
		subindices.push_back(":z");
	} break;
	case Variant::QUAT: {
		subindices.push_back(":x");
		subindices.push_back(":y");
		subindices.push_back(":z");
		subindices.push_back(":w");
	} break;
	case Variant::COLOR: {
		subindices.push_back(":r");
		subindices.push_back(":g");
		subindices.push_back(":b");
		subindices.push_back(":a");
	} break;
	case Variant::PLANE: {
		subindices.push_back(":x");
		subindices.push_back(":y");
		subindices.push_back(":z");
		subindices.push_back(":d");
	} break;
	default: {
		if (r_valid) {
			*r_valid = false;
		}
	}
	}

	return subindices;
}

TrackEditor::TrackIndices TrackEditor::_confirm_insert(InsertData p_id, TrackIndices p_next_tracks, bool p_create_reset, Ref<Animation> p_reset_anim, bool p_create_beziers) {
	bool created = false;
	if (p_id.track_idx < 0) {
		if (p_create_beziers) {
			bool valid;
			Vector<String> subindices = _get_bezier_subindices_for_type(p_id.value.get_type(), &valid);
			if (valid) {
				for (int i = 0; i < subindices.size(); i++) {
					InsertData id = p_id;
					id.type = Animation::TYPE_BEZIER;
					id.value = p_id.value.get(subindices[i].substr(1, subindices[i].length()));
					id.path = String(p_id.path) + subindices[i];
					p_next_tracks = _confirm_insert(id, p_next_tracks, p_create_reset, p_reset_anim, false);
				}

				return p_next_tracks;
			}
		}
		created = true;
		undo_redo->create_action(TTR("Anim Insert Track & Key"));
		Animation::UpdateMode update_mode = Animation::UPDATE_DISCRETE;

		if (p_id.type == Animation::TYPE_VALUE || p_id.type == Animation::TYPE_BEZIER) {
			// Wants a new track.

			{
				// Hack.
				NodePath np;
				animation->add_track(p_id.type);
				animation->track_set_path(animation->get_track_count() - 1, p_id.path);
				PropertyInfo h = _find_hint_for_track(animation->get_track_count() - 1, np);
				animation->remove_track(animation->get_track_count() - 1); // Hack.

				if (h.type == Variant::REAL ||
					h.type == Variant::VECTOR2 ||
					h.type == Variant::RECT2 ||
					h.type == Variant::VECTOR3 ||
					h.type == Variant::AABB ||
					h.type == Variant::QUAT ||
					h.type == Variant::COLOR ||
					h.type == Variant::PLANE ||
					h.type == Variant::TRANSFORM2D ||
					h.type == Variant::TRANSFORM) {
					update_mode = Animation::UPDATE_CONTINUOUS;
				}

				if (h.usage & PROPERTY_USAGE_ANIMATE_AS_TRIGGER) {
					update_mode = Animation::UPDATE_TRIGGER;
				}
			}
		}

		p_id.track_idx = p_next_tracks.normal;

		undo_redo->add_do_method(animation.ptr(), "add_track", p_id.type);
		undo_redo->add_do_method(animation.ptr(), "track_set_path", p_id.track_idx, p_id.path);
		if (p_id.type == Animation::TYPE_VALUE) {
			undo_redo->add_do_method(animation.ptr(), "value_track_set_update_mode", p_id.track_idx, update_mode);
		}

	}
	else {
		undo_redo->create_action(TTR("Anim Insert Key"));
	}

	float time = timeline->get_play_position();
	Variant value;

	switch (p_id.type) {
	case Animation::TYPE_VALUE: {
		value = p_id.value;

	} break;
	case Animation::TYPE_BEZIER: {
		Array array;
		array.resize(5);
		array[0] = p_id.value;
		array[1] = -0.25;
		array[2] = 0;
		array[3] = 0.25;
		array[4] = 0;
		value = array;

	} break;
	case Animation::TYPE_ANIMATION: {
		value = p_id.value;
	} break;
	default: {
	}
	}

	undo_redo->add_do_method(animation.ptr(), "track_insert_key", p_id.track_idx, time, value);

	if (created) {
		// Just remove the track.
		undo_redo->add_undo_method(this, "_clear_selection", false);
		undo_redo->add_undo_method(animation.ptr(), "remove_track", animation->get_track_count());
		p_next_tracks.normal++;
	}
	else {
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", p_id.track_idx, time);
		int existing = animation->track_find_key(p_id.track_idx, time, true);
		if (existing != -1) {
			Variant v = animation->track_get_key_value(p_id.track_idx, existing);
			float trans = animation->track_get_key_transition(p_id.track_idx, existing);
			undo_redo->add_undo_method(animation.ptr(), "track_insert_key", p_id.track_idx, time, v, trans);
		}
	}

	if (p_create_reset && track_type_is_resettable(p_id.type)) {
		bool create_reset_track = true;
		Animation* reset_anim = p_reset_anim.ptr();
		for (int i = 0; i < reset_anim->get_track_count(); i++) {
			if (reset_anim->track_get_path(i) == p_id.path) {
				create_reset_track = false;
				break;
			}
		}
		if (create_reset_track) {
			undo_redo->add_do_method(reset_anim, "add_track", p_id.type);
			undo_redo->add_do_method(reset_anim, "track_set_path", p_next_tracks.reset, p_id.path);
			undo_redo->add_do_method(reset_anim, "track_insert_key", p_next_tracks.reset, 0.0f, value);
			undo_redo->add_undo_method(reset_anim, "remove_track", reset_anim->get_track_count());
			p_next_tracks.reset++;
		}
	}

	undo_redo->commit_action();

	return p_next_tracks;
}

void TrackEditor::show_select_node_warning(bool p_show) {
	info_message->set_visible(p_show);
}

bool TrackEditor::is_key_selected(int p_track, int p_key) const {
	SelectedKey sk;
	sk.key = p_key;
	sk.track = p_track;

	return selection.has(sk);
}

bool TrackEditor::is_selection_active() const {
	return selection.size();
}

bool TrackEditor::is_snap_enabled() const {
	return snap->is_pressed() ^ Input::get_singleton()->is_key_pressed(KEY_CONTROL);
}

void TrackEditor::_update_tracks() {
	int selected = _get_track_selected();

	while (track_vbox->get_child_count()) {
		memdelete(track_vbox->get_child(0));
	}

	track_edits.clear();
	track_edit_headers.clear();

	if (animation.is_null()) {
		return;
	}

	track_edits.resize(animation->get_track_count());

	Vector<int> completed_tracks;

	Map<String, VBoxContainer*> group_sort;

	bool use_filter = selected_filter->is_pressed();

	for (Map<Ref<Script>, Ref<Script>>::Element* E = track_edit_groups.front(); E; E = E->next()) {
		Ref<Script> track_header_class = E->key();
		TrackEdit* track_edit_header = memnew(TrackEdit);
		ScriptInstance* track_edit_header_script_instance = track_header_class->instance_create(track_edit_header);
		if (track_edit_header_script_instance) {
			track_edit_header->set_script_instance(track_edit_header_script_instance);
			add_track_edit(track_edit_header, 0, true);
		}
		else {
			memdelete(track_edit_header);
			continue;
		}

		for (int i = 0; i < animation->get_track_count(); i++) {
			const bool track_belongs_to_header = track_edit_header->call(_does_track_belong_to_header, i);
			if (track_belongs_to_header) {
				Ref<Script> track_edit_script = E->value();
				TrackEdit* track_edit = memnew(TrackEdit);
				ScriptInstance* track_edit_script_instance = track_edit_script->instance_create(track_edit);

				if (track_edit_script_instance) {
					track_edit->set_script_instance(track_edit_script_instance);
					add_track_edit(track_edit, i, false);
					completed_tracks.push_back(i);
				}
				else {
					memdelete(track_edit);
				}
			}
		}
	}
	
	for (int i = 0; i < animation->get_track_count(); i++) {
		if (completed_tracks.find(i) > -1) {
			continue;
		}
		TrackEdit* track_edit = nullptr;

		// Find hint and info for plugin.

		if (use_filter) {
			NodePath path = animation->track_get_path(i);

			if (root && root->has_node(path)) {
				Node* node = root->get_node(path);
				if (!node) {
					continue; // No node, no filter.
				}
				//if (!EditorNode::get_singleton()->get_editor_selection()->is_selected(node)) {
				//	continue; // Skip track due to not selected.
				//}
			}
		}

		if (animation->track_get_type(i) == Animation::TYPE_VALUE) {
			NodePath path = animation->track_get_path(i);

			if (root && root->has_node_and_resource(path)) {
				RES res;
				NodePath base_path;
				Vector<StringName> leftover_path;
				Node* node = root->get_node_and_resource(path, res, leftover_path, true);
				PropertyInfo pinfo = _find_hint_for_track(i, base_path);

				Object* object = node;
				if (res.is_valid()) {
					object = res.ptr();
				}

				if (object && !leftover_path.empty()) {
					if (pinfo.name.empty()) {
						pinfo.name = leftover_path[leftover_path.size() - 1];
					}

					for (int j = 0; j < track_edit_plugins.size(); j++) {
						track_edit = track_edit_plugins.write[j]->create_value_track_edit(object, pinfo.type, pinfo.name, pinfo.hint, pinfo.hint_string, pinfo.usage);
						if (track_edit) {
							break;
						}
					}
				}
			}
		}
		if (animation->track_get_type(i) == Animation::TYPE_AUDIO) {
			for (int j = 0; j < track_edit_plugins.size(); j++) {
				track_edit = track_edit_plugins.write[j]->create_audio_track_edit();
				if (track_edit) {
					break;
				}
			}
		}

		if (animation->track_get_type(i) == Animation::TYPE_ANIMATION) {
			NodePath path = animation->track_get_path(i);

			Node* node = nullptr;
			if (root && root->has_node(path)) {
				node = root->get_node(path);
			}

			if (node && Object::cast_to<AnimationPlayer>(node)) {
				for (int j = 0; j < track_edit_plugins.size(); j++) {
					track_edit = track_edit_plugins.write[j]->create_animation_track_edit(node);
					if (track_edit) {
						break;
					}
				}
			}
		}

		if (track_edit == nullptr) {
			// No valid plugin_found.
			track_edit = memnew(TrackEdit);
		}

		add_track_edit(track_edit, i, false);
		if (selected == i) {
			track_edit->grab_focus();
		}


	}
}

void TrackEditor::set_track_edit_type(const Ref<Script> p_header_class, const Ref<Script> p_track_edit_class) {
	track_edit_groups[p_header_class] = p_track_edit_class;
}

void TrackEditor::add_track_edit(TrackEdit* p_track_edit, int p_track, bool p_is_header) {
	if (p_is_header) {
		track_edit_headers.push_back(p_track_edit);
	}
	else {
		track_edits.write[p_track] = p_track_edit;
	}

	p_track_edit->set_in_group(false);
	p_track_edit->set_editor(this);
	p_track_edit->set_timeline(timeline);
	p_track_edit->set_undo_redo(undo_redo);
	p_track_edit->set_root(root);
	p_track_edit->set_animation_and_track(animation, p_track);
	p_track_edit->set_play_position(timeline->get_play_position());
	track_vbox->add_child(p_track_edit);
	
	p_track_edit->connect("timeline_changed", this, "_timeline_changed");

	if (!p_is_header) {
		p_track_edit->connect("remove_request", this, "_track_remove_request", varray(), CONNECT_DEFERRED);
		p_track_edit->connect("dropped", this, "_dropped_track", varray(), CONNECT_DEFERRED);
		p_track_edit->connect("insert_key", this, "_insert_key_from_track", varray(p_track), CONNECT_DEFERRED);
		p_track_edit->connect("select_key", this, "_key_selected", varray(p_track), CONNECT_DEFERRED);
		p_track_edit->connect("deselect_key", this, "_key_deselected", varray(p_track), CONNECT_DEFERRED);
		p_track_edit->connect("move_selection_begin", this, "_move_selection_begin");
		p_track_edit->connect("move_selection", this, "_move_selection");
		p_track_edit->connect("move_selection_commit", this, "_move_selection_commit");
		p_track_edit->connect("move_selection_cancel", this, "_move_selection_cancel");
		p_track_edit->connect("duplicate_request", this, "_edit_menu_pressed", varray(EDIT_DUPLICATE_SELECTION), CONNECT_DEFERRED);
		p_track_edit->connect("duplicate_transpose_request", this, "_edit_menu_pressed", varray(EDIT_DUPLICATE_TRANSPOSED), CONNECT_DEFERRED);
		p_track_edit->connect("create_reset_request", this, "_edit_menu_pressed", varray(EDIT_ADD_RESET_KEY), CONNECT_DEFERRED);
		p_track_edit->connect("delete_request", this, "_edit_menu_pressed", varray(EDIT_DELETE_SELECTION), CONNECT_DEFERRED);
	}
}

void TrackEditor::_animation_changed() {
	if (animation_changing_awaiting_update) {
		return; // All will be updated, don't bother with anything.
	}

	if (key_edit && key_edit->setting) {
		// If editing a key, just update the edited track, makes refresh less costly.
		if (key_edit->track < track_edits.size()) {
			track_edits[key_edit->track]->update();
		}
		return;
	}

	animation_changing_awaiting_update = true;
	call_deferred("_animation_update");
}

void TrackEditor::_snap_mode_changed(int p_mode) {
	timeline->set_use_fps(p_mode == 1);
	if (key_edit) {
		key_edit->set_use_fps(p_mode == 1);
	}
	_update_step_spinbox();
}

void TrackEditor::_update_step_spinbox() {
	if (!animation.is_valid()) {
		return;
	}
	step->set_block_signals(true);

	if (timeline->is_using_fps()) {
		if (animation->get_step() == 0) {
			step->set_value(0);
		}
		else {
			step->set_value(1.0 / animation->get_step());
		}

	}
	else {
		step->set_value(animation->get_step());
	}

	step->set_block_signals(false);
}

void TrackEditor::_animation_update() {
	timeline->update();

	bool same = true;

	if (animation.is_null()) {
		return;
	}

	if (track_edits.size() == animation->get_track_count()) {
		// Check tracks are the same.

		for (int i = 0; i < track_edits.size(); i++) {
			if (track_edits[i]->get_path() != animation->track_get_path(i)) {
				same = false;
				break;
			}
		}
	}
	else {
		same = false;
	}

	if (same) {
		for (int i = 0; i < track_edits.size(); i++) {
			track_edits[i]->update();
		}
	}
	else {
		_update_tracks();
	}

	_update_step_spinbox();
	emit_signal("animation_step_changed", animation->get_step());
	emit_signal("animation_len_changed", animation->get_length());

	animation_changing_awaiting_update = false;
}

void TrackEditor::_notification(int p_what) {
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		//panner->setup((ViewPanner::ControlScheme)EDITOR_GET("editors/panning/animation_editors_panning_scheme").operator int(), ED_GET_SHORTCUT("canvas_item_editor/pan_view"), bool(EditorSettings::get_singleton()->get("editors/panning/simple_panning")));
	}
	case NOTIFICATION_THEME_CHANGED: {
		IconsCache* icons = IconsCache::get_singleton();
		zoom_icon->set_texture(icons->get_icon("Zoom"));
		snap->set_icon(icons->get_icon("Snap"));
		view_group->set_icon(icons->get_icon(view_group->is_pressed() ? "AnimationTrackList" : "AnimationTrackGroup"));
		selected_filter->set_icon(icons->get_icon("AnimationFilter"));
		imported_anim_warning->set_icon(icons->get_icon("NodeWarning"));
		main_panel->add_style_override("panel", get_stylebox("bg", "Tree"));
	} break;

	case NOTIFICATION_READY: {
		//EditorNode::get_singleton()->get_editor_selection()->connect("selection_changed", this, "_selection_changed");
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		update_keying();
	} break;
	}
}

void TrackEditor::_update_scroll(double) {
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}
}

void TrackEditor::_update_step(double p_new_step) {
	undo_redo->create_action(TTR("Change Animation Step"));
	float step_value = p_new_step;
	if (timeline->is_using_fps()) {
		if (step_value != 0.0) {
			step_value = 1.0 / step_value;
		}
	}
	undo_redo->add_do_method(animation.ptr(), "set_step", step_value);
	undo_redo->add_undo_method(animation.ptr(), "set_step", animation->get_step());
	step->set_block_signals(true);
	undo_redo->commit_action();
	step->set_block_signals(false);
	emit_signal("animation_step_changed", step_value);
}

void TrackEditor::_update_length(double p_new_len) {
	emit_signal("animation_len_changed", p_new_len);
}

void TrackEditor::_dropped_track(int p_from_track, int p_to_track) {
	if (p_from_track == p_to_track || p_from_track == p_to_track - 1) {
		return;
	}

	_clear_selection();
	undo_redo->create_action(TTR("Rearrange Tracks"));
	undo_redo->add_do_method(animation.ptr(), "track_move_to", p_from_track, p_to_track);
	// Take into account that the position of the tracks that come after the one removed will change.
	int to_track_real = p_to_track > p_from_track ? p_to_track - 1 : p_to_track;
	undo_redo->add_undo_method(animation.ptr(), "track_move_to", to_track_real, p_to_track > p_from_track ? p_from_track : p_from_track + 1);
	undo_redo->add_do_method(this, "_track_grab_focus", to_track_real);
	undo_redo->add_undo_method(this, "_track_grab_focus", p_from_track);
	undo_redo->commit_action();
}

void TrackEditor::_new_track_node_selected(NodePath p_path) {
	ERR_FAIL_COND(!root);
	Node* node = get_node(p_path);
	ERR_FAIL_COND(!node);
	NodePath path_to = root->get_path_to(node);

	switch (adding_track_type) {
	case Animation::TYPE_VALUE: {
		adding_track_path = path_to;
		//prop_selector->set_type_filter(Vector<Variant::Type>());
		//prop_selector->select_property_from_instance(node);
	} break;

	case Animation::TYPE_AUDIO: {
		if (!node->is_class("AudioStreamPlayer") && !node->is_class("AudioStreamPlayer2D") && !node->is_class("AudioStreamPlayer3D")) {
			//EditorNode::get_singleton()->show_warning(TTR("Audio tracks can only point to nodes of type:\n-AudioStreamPlayer\n-AudioStreamPlayer2D\n-AudioStreamPlayer3D"));
			return;
		}

		undo_redo->create_action(TTR("Add Track"));
		undo_redo->add_do_method(animation.ptr(), "add_track", adding_track_type);
		undo_redo->add_do_method(animation.ptr(), "track_set_path", animation->get_track_count(), path_to);
		undo_redo->add_undo_method(animation.ptr(), "remove_track", animation->get_track_count());
		undo_redo->commit_action();

	} break;
	case Animation::TYPE_ANIMATION: {
		if (!node->is_class("AnimationPlayer")) {
			//EditorNode::get_singleton()->show_warning(TTR("Animation tracks can only point to AnimationPlayer nodes."));
			return;
		}

		if (node == PlayerEditorControl::get_singleton()->get_player()) {
			//EditorNode::get_singleton()->show_warning(TTR("AnimationPlayer can't animate itself, only other players."));
			return;
		}

		undo_redo->create_action(TTR("Add Track"));
		undo_redo->add_do_method(animation.ptr(), "add_track", adding_track_type);
		undo_redo->add_do_method(animation.ptr(), "track_set_path", animation->get_track_count(), path_to);
		undo_redo->add_undo_method(animation.ptr(), "remove_track", animation->get_track_count());
		undo_redo->commit_action();

	} break;
	}
}

void TrackEditor::_add_track(int p_type) {
	if (!root) {
		//EditorNode::get_singleton()->show_warning(TTR("Not possible to add a new track without a root"));
		return;
	}
	adding_track_type = p_type;
	/*pick_track->popup_centered();
	pick_track->get_filter_line_edit()->clear();
	pick_track->get_filter_line_edit()->grab_focus();*/
}

void TrackEditor::_new_track_property_selected(String p_name) {
	String full_path = String(adding_track_path) + ":" + p_name;

	if (adding_track_type == Animation::TYPE_VALUE) {
		Animation::UpdateMode update_mode = Animation::UPDATE_DISCRETE;
		{
			// Hack.
			NodePath np;
			animation->add_track(Animation::TYPE_VALUE);
			animation->track_set_path(animation->get_track_count() - 1, full_path);
			PropertyInfo h = _find_hint_for_track(animation->get_track_count() - 1, np);
			animation->remove_track(animation->get_track_count() - 1); // Hack.
			if (h.type == Variant::REAL ||
				h.type == Variant::VECTOR2 ||
				h.type == Variant::RECT2 ||
				h.type == Variant::VECTOR3 ||
				h.type == Variant::AABB ||
				h.type == Variant::QUAT ||
				h.type == Variant::COLOR ||
				h.type == Variant::PLANE ||
				h.type == Variant::TRANSFORM2D ||
				h.type == Variant::TRANSFORM) {
				update_mode = Animation::UPDATE_CONTINUOUS;
			}

			if (h.usage & PROPERTY_USAGE_ANIMATE_AS_TRIGGER) {
				update_mode = Animation::UPDATE_TRIGGER;
			}
		}

		undo_redo->create_action(TTR("Add Track"));
		undo_redo->add_do_method(animation.ptr(), "add_track", adding_track_type);
		undo_redo->add_do_method(animation.ptr(), "track_set_path", animation->get_track_count(), full_path);
		undo_redo->add_do_method(animation.ptr(), "value_track_set_update_mode", animation->get_track_count(), update_mode);
		undo_redo->add_undo_method(animation.ptr(), "remove_track", animation->get_track_count());
		undo_redo->commit_action();
	}
	else {
		Vector<String> subindices;
		{
			// Hack.
			NodePath np;
			animation->add_track(Animation::TYPE_VALUE);
			animation->track_set_path(animation->get_track_count() - 1, full_path);
			PropertyInfo h = _find_hint_for_track(animation->get_track_count() - 1, np);
			animation->remove_track(animation->get_track_count() - 1); // Hack.
			bool valid;
			subindices = _get_bezier_subindices_for_type(h.type, &valid);
			if (!valid) {
				//EditorNode::get_singleton()->show_warning(TTR("Invalid track for Bezier (no suitable sub-properties)"));
				return;
			}
		}

		undo_redo->create_action(TTR("Add Bezier Track"));
		int base_track = animation->get_track_count();
		for (int i = 0; i < subindices.size(); i++) {
			undo_redo->add_do_method(animation.ptr(), "add_track", adding_track_type);
			undo_redo->add_do_method(animation.ptr(), "track_set_path", base_track + i, full_path + subindices[i]);
			undo_redo->add_undo_method(animation.ptr(), "remove_track", base_track);
		}
		undo_redo->commit_action();
	}
}

void TrackEditor::_timeline_value_changed(double) {
	timeline->update_play_position();

	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
		track_edits[i]->update_play_position();
	}
}

int TrackEditor::_get_track_selected() {
	for (int i = 0; i < track_edits.size(); i++) {
		if (track_edits[i]->has_focus()) {
			return i;
		}
	}

	return -1;
}

void TrackEditor::_insert_key_from_track(float p_ofs, int p_track) {
	ERR_FAIL_INDEX(p_track, animation->get_track_count());

	if (snap->is_pressed() && step->get_value() != 0) {
		p_ofs = snap_time(p_ofs);
	}
	while (animation->track_find_key(p_track, p_ofs, true) != -1) { // Make sure insertion point is valid.
		p_ofs += 0.001;
	}

	switch (animation->track_get_type(p_track)) {
	case Animation::TYPE_TRANSFORM: {
		if (!root->has_node(animation->track_get_path(p_track))) {
			return;
		}
		Spatial* base = Object::cast_to<Spatial>(root->get_node(animation->track_get_path(p_track)));

		if (!base) {
			return;
		}

		Transform xf = base->get_transform();

		Vector3 loc = xf.get_origin();
		Vector3 scale = xf.basis.get_scale_local();
		Quat rot = xf.basis;

		undo_redo->create_action(TTR("Add Transform Track Key"));
		undo_redo->add_do_method(animation.ptr(), "transform_track_insert_key", p_track, p_ofs, loc, rot, scale);
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_position", p_track, p_ofs);
		undo_redo->commit_action();

	} break;
	case Animation::TYPE_VALUE: {
		NodePath bp;
		Variant value;
		_find_hint_for_track(p_track, bp, &value);

		undo_redo->create_action(TTR("Add Track Key"));
		undo_redo->add_do_method(animation.ptr(), "track_insert_key", p_track, p_ofs, value);
		undo_redo->add_undo_method(this, "_clear_selection_for_anim", animation);
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", p_track, p_ofs);
		undo_redo->commit_action();

	} break;
	case Animation::TYPE_METHOD: {
		if (!root->has_node(animation->track_get_path(p_track))) {
			//EditorNode::get_singleton()->show_warning(TTR("Track path is invalid, so can't add a method key."));
			return;
		}
		//method_selector->select_method_from_instance(base);

		insert_key_from_track_call_ofs = p_ofs;
		insert_key_from_track_call_track = p_track;

	} break;
	case Animation::TYPE_BEZIER: {
		NodePath bp;
		Variant value;
		_find_hint_for_track(p_track, bp, &value);
		Array arr;
		arr.resize(6);
		arr[0] = value;
		arr[1] = -0.25;
		arr[2] = 0;
		arr[3] = 0.25;
		arr[4] = 0;
		arr[5] = 0;

		undo_redo->create_action(TTR("Add Track Key"));
		undo_redo->add_do_method(animation.ptr(), "track_insert_key", p_track, p_ofs, arr);
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", p_track, p_ofs);
		undo_redo->commit_action();

	} break;
	case Animation::TYPE_AUDIO: {
		Dictionary ak;
		ak["stream"] = RES();
		ak["start_offset"] = 0;
		ak["end_offset"] = 0;

		undo_redo->create_action(TTR("Add Track Key"));
		undo_redo->add_do_method(animation.ptr(), "track_insert_key", p_track, p_ofs, ak);
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", p_track, p_ofs);
		undo_redo->commit_action();
	} break;
	case Animation::TYPE_ANIMATION: {
		StringName anim = "[stop]";

		undo_redo->create_action(TTR("Add Track Key"));
		undo_redo->add_do_method(animation.ptr(), "track_insert_key", p_track, p_ofs, anim);
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", p_track, p_ofs);
		undo_redo->commit_action();
	} break;
	}
}

void TrackEditor::_key_selected(int p_key, bool p_single, int p_track) {
	ERR_FAIL_INDEX(p_track, animation->get_track_count());
	ERR_FAIL_INDEX(p_key, animation->track_get_key_count(p_track));

	SelectedKey sk;
	sk.key = p_key;
	sk.track = p_track;

	if (p_single) {
		_clear_selection();
	}

	KeyInfo ki;
	ki.pos = animation->track_get_key_time(p_track, p_key);
	selection[sk] = ki;

	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}

	_update_key_edit();
}

void TrackEditor::_key_deselected(int p_key, int p_track) {
	ERR_FAIL_INDEX(p_track, animation->get_track_count());
	ERR_FAIL_INDEX(p_key, animation->track_get_key_count(p_track));

	SelectedKey sk;
	sk.key = p_key;
	sk.track = p_track;

	selection.erase(sk);

	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}

	_update_key_edit();
}

void TrackEditor::_move_selection_begin() {
	moving_selection = true;
	moving_selection_offset = 0;
}

void TrackEditor::_move_selection(float p_offset) {
	moving_selection_offset = p_offset;

	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}
}

struct _AnimMoveRestore {
	int track = 0;
	float time = 0;
	Variant key;
	float transition = 0;
};
// Used for undo/redo.

void TrackEditor::_clear_key_edit() {
	if (key_edit) {
		// If key edit is the object being inspected, remove it first.
		/*if (InspectorDock::get_inspector_singleton()->get_edited_object() == key_edit) {
			EditorNode::get_singleton()->push_item(nullptr);
		}*/

		// Then actually delete it.
		memdelete(key_edit);
		key_edit = nullptr;
	}

	if (multi_key_edit) {
		/*if (InspectorDock::get_inspector_singleton()->get_edited_object() == multi_key_edit) {
			EditorNode::get_singleton()->push_item(nullptr);
		}*/

		memdelete(multi_key_edit);
		multi_key_edit = nullptr;
	}
}

void TrackEditor::_clear_selection(bool p_update) {
	selection.clear();

	if (p_update) {
		for (int i = 0; i < track_edits.size(); i++) {
			track_edits[i]->update();
		}
	}

	_clear_key_edit();
}

void TrackEditor::_update_key_edit() {
	_clear_key_edit();
	if (!animation.is_valid()) {
		return;
	}

	if (selection.size() == 1) {
		key_edit = memnew(TrackKeyEdit);
		key_edit->animation = animation;
		key_edit->track = selection.front()->key().track;
		key_edit->use_fps = timeline->is_using_fps();

		float ofs = animation->track_get_key_time(key_edit->track, selection.front()->key().key);
		key_edit->key_ofs = ofs;
		key_edit->root_path = root;

		NodePath np;
		key_edit->hint = _find_hint_for_track(key_edit->track, np);
		key_edit->undo_redo = undo_redo;
		key_edit->base = np;

		//EditorNode::get_singleton()->push_item(key_edit);
	}
	else if (selection.size() > 1) {
		multi_key_edit = memnew(MultiTrackKeyEdit);
		multi_key_edit->animation = animation;

		Map<int, List<float>> key_ofs_map;
		Map<int, NodePath> base_map;
		int first_track = -1;
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.front(); E; E = E->next()) {
			int track = E->key().track;
			if (first_track < 0) {
				first_track = track;
			}

			if (!key_ofs_map.has(track)) {
				key_ofs_map[track] = List<float>();
				base_map[track] = NodePath();
			}

			key_ofs_map[track].push_back(animation->track_get_key_time(track, E->key().key));
		}
		multi_key_edit->key_ofs_map = key_ofs_map;
		multi_key_edit->base_map = base_map;
		multi_key_edit->hint = _find_hint_for_track(first_track, base_map[first_track]);

		multi_key_edit->use_fps = timeline->is_using_fps();

		multi_key_edit->root_path = root;

		multi_key_edit->undo_redo = undo_redo;

		//EditorNode::get_singleton()->push_item(multi_key_edit);
	}
}

void TrackEditor::_clear_selection_for_anim(const Ref<Animation>& p_anim) {
	if (animation != p_anim) {
		return;
	}

	_clear_selection();
}

void TrackEditor::_select_at_anim(const Ref<Animation>& p_anim, int p_track, float p_pos) {
	if (animation != p_anim) {
		return;
	}

	int idx = animation->track_find_key(p_track, p_pos, true);
	ERR_FAIL_COND(idx < 0);

	SelectedKey sk;
	sk.track = p_track;
	sk.key = idx;
	KeyInfo ki;
	ki.pos = p_pos;

	selection.insert(sk, ki);
}

void TrackEditor::_move_selection_commit() {
	undo_redo->create_action(TTR("Anim Move Keys"));

	List<_AnimMoveRestore> to_restore;

	float motion = moving_selection_offset;
	// 1 - remove the keys.
	for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
		undo_redo->add_do_method(animation.ptr(), "track_remove_key", E->key().track, E->key().key);
	}
	// 2 - Remove overlapped keys.
	for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
		float newtime = snap_time(E->get().pos + motion);
		int idx = animation->track_find_key(E->key().track, newtime, true);
		if (idx == -1) {
			continue;
		}
		SelectedKey sk;
		sk.key = idx;
		sk.track = E->key().track;
		if (selection.has(sk)) {
			continue; // Already in selection, don't save.
		}

		undo_redo->add_do_method(animation.ptr(), "track_remove_key_at_time", E->key().track, newtime);
		_AnimMoveRestore amr;

		amr.key = animation->track_get_key_value(E->key().track, idx);
		amr.track = E->key().track;
		amr.time = newtime;
		amr.transition = animation->track_get_key_transition(E->key().track, idx);

		to_restore.push_back(amr);
	}

	// 3 - Move the keys (Reinsert them).
	for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
		float newpos = snap_time(E->get().pos + motion);
		undo_redo->add_do_method(animation.ptr(), "track_insert_key", E->key().track, newpos, animation->track_get_key_value(E->key().track, E->key().key), animation->track_get_key_transition(E->key().track, E->key().key));
	}

	// 4 - (Undo) Remove inserted keys.
	for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
		float newpos = snap_time(E->get().pos + motion);
		undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", E->key().track, newpos);
	}

	// 5 - (Undo) Reinsert keys.
	for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
		undo_redo->add_undo_method(animation.ptr(), "track_insert_key", E->key().track, E->get().pos, animation->track_get_key_value(E->key().track, E->key().key), animation->track_get_key_transition(E->key().track, E->key().key));
	}

	// 6 - (Undo) Reinsert overlapped keys.
	for (List<_AnimMoveRestore>::Element* amr = to_restore.front(); amr; amr = amr->next()) {
		undo_redo->add_undo_method(animation.ptr(), "track_insert_key", amr->get().track, amr->get().time, amr->get().key, amr->get().transition);
	}

	undo_redo->add_do_method(this, "_clear_selection_for_anim", animation);
	undo_redo->add_undo_method(this, "_clear_selection_for_anim", animation);

	// 7 - Reselect.
	for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
		float oldpos = E->get().pos;
		float newpos = snap_time(oldpos + motion);

		undo_redo->add_do_method(this, "_select_at_anim", animation, E->key().track, newpos);
		undo_redo->add_undo_method(this, "_select_at_anim", animation, E->key().track, oldpos);
	}

	undo_redo->commit_action();

	moving_selection = false;
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}

	_update_key_edit();
}

void TrackEditor::_move_selection_cancel() {
	moving_selection = false;
	for (int i = 0; i < track_edits.size(); i++) {
		track_edits[i]->update();
	}
}

bool TrackEditor::is_moving_selection() const {
	return moving_selection;
}

float TrackEditor::get_moving_selection_offset() const {
	return moving_selection_offset;
}

void TrackEditor::_box_selection_draw() {
	const Rect2 selection_rect = Rect2(Point2(), box_selection->get_size());
	box_selection->draw_rect(selection_rect, Colors::BOX_SELECTION_FILL_COLOR);
	box_selection->draw_rect(selection_rect, Colors::BOX_SELECTION_STROKE_COLOR, false, Math::round(1.0));
}

void TrackEditor::_scroll_input(const Ref<InputEvent>& p_event) {
	if (panner->gui_input(p_event)) {
		scroll->accept_event();
		return;
	}

	Ref<InputEventMouseButton> mb = p_event;

	if (mb.is_valid() && mb->get_button_index() == BUTTON_LEFT) {
		if (mb->is_pressed()) {
			box_selecting = true;
			box_selecting_from = scroll->get_global_transform().xform(mb->get_position());
			box_select_rect = Rect2();
		}
		else if (box_selecting) {
			if (box_selection->is_visible_in_tree()) {
				// Only if moved.
				for (int i = 0; i < track_edits.size(); i++) {
					Rect2 local_rect = box_select_rect;
					local_rect.position -= track_edits[i]->get_global_position();
					track_edits[i]->append_to_selection(local_rect, mb->get_command());
				}

				if (_get_track_selected() == -1 && track_edits.size() > 0) { // Minimal hack to make shortcuts work.
					track_edits[track_edits.size() - 1]->grab_focus();
				}
			}
			else {
				_clear_selection(); // Clear it.
			}

			box_selection->hide();
			box_selecting = false;
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;

	if (mm.is_valid() && box_selecting) {
		if ((mm->get_button_mask() & BUTTON_MASK_LEFT) == 0) {
			// No longer.
			box_selection->hide();
			box_selecting = false;
			return;
		}

		if (!box_selection->is_visible_in_tree()) {
			if (!mm->get_command() && !mm->get_shift()) {
				_clear_selection();
			}
			box_selection->show();
		}

		Vector2 from = box_selecting_from;
		Vector2 to = scroll->get_global_transform().xform(mm->get_position());

		if (from.x > to.x) {
			SWAP(from.x, to.x);
		}

		if (from.y > to.y) {
			SWAP(from.y, to.y);
		}

		Rect2 rect(from, to - from);
		Rect2 scroll_rect = Rect2(scroll->get_global_position(), scroll->get_size());
		rect = scroll_rect.clip(rect);
		box_selection->set_position(rect.position);
		box_selection->set_size(rect.size);

		box_select_rect = rect;
	}
}

void TrackEditor::_scroll_callback(Vector2 p_scroll_vec, bool p_alt) {
	if (p_alt) {
		if (p_scroll_vec.x < 0 || p_scroll_vec.y < 0) {
			goto_prev_step(true);
		}
		else {
			goto_next_step(true);
		}
	}
	else {
		_pan_callback(-p_scroll_vec * 32);
	}
}

void TrackEditor::_pan_callback(Vector2 p_scroll_vec) {
	timeline->set_value(timeline->get_value() - p_scroll_vec.x / timeline->get_zoom_scale());
	scroll->set_v_scroll(scroll->get_v_scroll() - p_scroll_vec.y);
}

void TrackEditor::_zoom_callback(Vector2 p_scroll_vec, Vector2 p_origin, bool p_alt) {
	double new_zoom_value;
	double current_zoom_value = timeline->get_zoom()->get_value();
	if (current_zoom_value <= 0.1) {
		new_zoom_value = MAX(0.01, current_zoom_value - 0.01 * SGN(p_scroll_vec.y));
	}
	else {
		new_zoom_value = p_scroll_vec.y > 0 ? MAX(0.01, current_zoom_value / 1.05) : current_zoom_value * 1.05;
	}
	timeline->get_zoom()->set_value(new_zoom_value);
}

void TrackEditor::_anim_duplicate_keys(bool transpose) {
	// Duplicait!
	if (selection.size() && animation.is_valid() && (!transpose || (_get_track_selected() >= 0 && _get_track_selected() < animation->get_track_count()))) {
		int top_track = 0x7FFFFFFF;
		float top_time = 1e10;
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			const SelectedKey& sk = E->key();

			float t = animation->track_get_key_time(sk.track, sk.key);
			if (t < top_time) {
				top_time = t;
			}
			if (sk.track < top_track) {
				top_track = sk.track;
			}
		}
		ERR_FAIL_COND(top_track == 0x7FFFFFFF || top_time == 1e10);

		//

		int start_track = transpose ? _get_track_selected() : top_track;

		undo_redo->create_action(TTR("Anim Duplicate Keys"));

		List<Pair<int, float>> new_selection_values;

		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			const SelectedKey& sk = E->key();

			float t = animation->track_get_key_time(sk.track, sk.key);

			float dst_time = t + (timeline->get_play_position() - top_time);
			int dst_track = sk.track + (start_track - top_track);

			if (dst_track < 0 || dst_track >= animation->get_track_count()) {
				continue;
			}

			if (animation->track_get_type(dst_track) != animation->track_get_type(sk.track)) {
				continue;
			}

			int existing_idx = animation->track_find_key(dst_track, dst_time, true);

			undo_redo->add_do_method(animation.ptr(), "track_insert_key", dst_track, dst_time, animation->track_get_key_value(E->key().track, E->key().key), animation->track_get_key_transition(E->key().track, E->key().key));
			undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", dst_track, dst_time);

			Pair<int, float> p;
			p.first = dst_track;
			p.second = dst_time;
			new_selection_values.push_back(p);

			if (existing_idx != -1) {
				undo_redo->add_undo_method(animation.ptr(), "track_insert_key", dst_track, dst_time, animation->track_get_key_value(dst_track, existing_idx), animation->track_get_key_transition(dst_track, existing_idx));
			}
		}

		undo_redo->commit_action();

		// Reselect duplicated.

		Map<SelectedKey, KeyInfo> new_selection;
		for (List<Pair<int, float>>::Element* E = new_selection_values.front(); E; E = E->next()) {
			int track = E->get().first;
			float time = E->get().second;

			int existing_idx = animation->track_find_key(track, time, true);

			if (existing_idx == -1) {
				continue;
			}
			SelectedKey sk2;
			sk2.track = track;
			sk2.key = existing_idx;

			KeyInfo ki;
			ki.pos = time;

			new_selection[sk2] = ki;
		}

		selection = new_selection;
		_update_tracks();
		_update_key_edit();
	}
}

void TrackEditor::_edit_menu_about_to_popup() {
	//edit->get_popup()->set_item_disabled(edit->get_popup()->get_item_index(EDIT_APPLY_RESET), !player->has_animation("RESET") || player->get_assigned_animation() != "RESET");
}

void TrackEditor::goto_prev_step(bool p_from_mouse_event) {
	if (animation.is_null()) {
		return;
	}
	float step = animation->get_step();
	if (step == 0) {
		step = 1;
	}
	if (p_from_mouse_event && Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
		// Use more precise snapping when holding Shift.
		// This is used when scrobbling the timeline using Alt + Mouse wheel.
		step *= 0.25;
	}

	float pos = timeline->get_play_position();
	pos = Math::stepify(pos - step, step);
	if (pos < 0) {
		pos = 0;
	}
	set_anim_pos(pos);
	emit_signal("timeline_changed", pos, true, false);
}

void TrackEditor::goto_next_step(bool p_from_mouse_event) {
	if (animation.is_null()) {
		return;
	}
	float step = animation->get_step();
	if (step == 0) {
		step = 1;
	}
	if (p_from_mouse_event && Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
		// Use more precise snapping when holding Shift.
		// This is used when scrobbling the timeline using Alt + Mouse wheel.
		// Do not use precise snapping when using the menu action or keyboard shortcut,
		// as the default keyboard shortcut requires pressing Shift.
		step *= 0.25;
	}

	float pos = timeline->get_play_position();

	pos = Math::stepify(pos + step, step);
	if (pos > animation->get_length()) {
		pos = animation->get_length();
	}
	set_anim_pos(pos);

	emit_signal("timeline_changed", pos, true, false);
}

void TrackEditor::_edit_menu_pressed(int p_option) {
	last_menu_track_opt = p_option;
	IconsCache* icons = IconsCache::get_singleton();
	switch (p_option) {
	case EDIT_COPY_TRACKS: {
		track_copy_select->clear();
		TreeItem* troot = track_copy_select->create_item();

		for (int i = 0; i < animation->get_track_count(); i++) {
			NodePath path = animation->track_get_path(i);
			Node* node = nullptr;

			if (root && root->has_node(path)) {
				node = root->get_node(path);
			}

			String text;
			Ref<Texture> icon = icons->get_icon("Node");
			if (node) {
				if (icons->has_icon(node->get_class())) {
					icon = icons->get_icon(node->get_class());
				}

				text = node->get_name();
				Vector<StringName> sn = path.get_subnames();
				for (int j = 0; j < sn.size(); j++) {
					text += ".";
					text += sn[j];
				}

				path = NodePath(node->get_path().get_names(), path.get_subnames(), true); // Store full path instead for copying.
			}
			else {
				text = path;
				int sep = text.find(":");
				if (sep != -1) {
					text = text.substr(sep + 1, text.length());
				}
			}

			String track_type;
			switch (animation->track_get_type(i)) {
			case Animation::TYPE_METHOD:
				track_type = TTR("Methods");
				break;
			case Animation::TYPE_BEZIER:
				track_type = TTR("Bezier");
				break;
			case Animation::TYPE_AUDIO:
				track_type = TTR("Audio");
				break;
			default: {
			};
			}
			if (!track_type.empty()) {
				text += vformat(" (%s)", track_type);
			}

			TreeItem* it = track_copy_select->create_item(troot);
			it->set_editable(0, true);
			it->set_selectable(0, true);
			it->set_cell_mode(0, TreeItem::CELL_MODE_CHECK);
			it->set_icon(0, icon);
			it->set_text(0, text);
			Dictionary md;
			md["track_idx"] = i;
			md["path"] = path;
			it->set_metadata(0, md);
		}

		track_copy_dialog->popup_centered(Size2(350, 500) * 1.0);
	} break;
	case EDIT_COPY_TRACKS_CONFIRM: {
		track_clipboard.clear();
		TreeItem* root = track_copy_select->get_root();
		if (root) {
			TreeItem* it = root->get_children();
			while (it) {
				Dictionary md = it->get_metadata(0);
				int idx = md["track_idx"];
				if (it->is_checked(0) && idx >= 0 && idx < animation->get_track_count()) {
					TrackClipboard tc;
					tc.base_path = animation->track_get_path(idx);
					tc.full_path = md["path"];
					tc.track_type = animation->track_get_type(idx);
					tc.interp_type = animation->track_get_interpolation_type(idx);
					if (tc.track_type == Animation::TYPE_VALUE) {
						tc.update_mode = animation->value_track_get_update_mode(idx);
					}
					tc.loop_wrap = animation->track_get_interpolation_loop_wrap(idx);
					tc.enabled = animation->track_is_enabled(idx);
					for (int i = 0; i < animation->track_get_key_count(idx); i++) {
						TrackClipboard::Key k;
						k.time = animation->track_get_key_time(idx, i);
						k.value = animation->track_get_key_value(idx, i);
						k.transition = animation->track_get_key_transition(idx, i);
						tc.keys.push_back(k);
					}
					track_clipboard.push_back(tc);
				}
				it = it->get_next();
			}
		}
	} break;
	case EDIT_PASTE_TRACKS: {
		if (track_clipboard.size() == 0) {
			//EditorNode::get_singleton()->show_warning(TTR("Clipboard is empty!"));
			break;
		}

		int base_track = animation->get_track_count();
		undo_redo->create_action(TTR("Paste Tracks"));
		for (int i = 0; i < track_clipboard.size(); i++) {
			undo_redo->add_do_method(animation.ptr(), "add_track", track_clipboard[i].track_type);
			Node* exists = nullptr;
			NodePath path = track_clipboard[i].base_path;

			if (root) {
				NodePath np = track_clipboard[i].full_path;
				exists = root->get_node(np);
				if (exists) {
					path = NodePath(root->get_path_to(exists).get_names(), track_clipboard[i].full_path.get_subnames(), false);
				}
			}

			undo_redo->add_do_method(animation.ptr(), "track_set_path", base_track, path);
			undo_redo->add_do_method(animation.ptr(), "track_set_interpolation_type", base_track, track_clipboard[i].interp_type);
			undo_redo->add_do_method(animation.ptr(), "track_set_interpolation_loop_wrap", base_track, track_clipboard[i].loop_wrap);
			undo_redo->add_do_method(animation.ptr(), "track_set_enabled", base_track, track_clipboard[i].enabled);
			if (track_clipboard[i].track_type == Animation::TYPE_VALUE) {
				undo_redo->add_do_method(animation.ptr(), "value_track_set_update_mode", base_track, track_clipboard[i].update_mode);
			}

			for (int j = 0; j < track_clipboard[i].keys.size(); j++) {
				undo_redo->add_do_method(animation.ptr(), "track_insert_key", base_track, track_clipboard[i].keys[j].time, track_clipboard[i].keys[j].value, track_clipboard[i].keys[j].transition);
			}

			undo_redo->add_undo_method(animation.ptr(), "remove_track", animation->get_track_count());

			base_track++;
		}

		undo_redo->commit_action();
	} break;

	case EDIT_SCALE_SELECTION:
	case EDIT_SCALE_FROM_CURSOR: {
		scale_dialog->popup_centered(Size2(200, 100) * 1.0);
	} break;
	case EDIT_SCALE_CONFIRM: {
		if (selection.empty()) {
			return;
		}

		float from_t = 1e20;
		float to_t = -1e20;
		float len = -1e20;
		float pivot = 0;

		for (Map<SelectedKey, KeyInfo>::Element* E = selection.front(); E; E = E->next()) {
			float t = animation->track_get_key_time(E->key().track, E->key().key);
			if (t < from_t) {
				from_t = t;
			}
			if (t > to_t) {
				to_t = t;
			}
		}

		len = to_t - from_t;
		if (last_menu_track_opt == EDIT_SCALE_FROM_CURSOR) {
			pivot = timeline->get_play_position();

		}
		else {
			pivot = from_t;
		}

		float s = scale->get_value();
		if (s == 0) {
			ERR_PRINT("Can't scale to 0");
		}

		undo_redo->create_action(TTR("Anim Scale Keys"));

		List<_AnimMoveRestore> to_restore;

		// 1 - Remove the keys.
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			undo_redo->add_do_method(animation.ptr(), "track_remove_key", E->key().track, E->key().key);
		}
		// 2 - Remove overlapped keys.
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			float newtime = (E->get().pos - from_t) * s + from_t;
			int idx = animation->track_find_key(E->key().track, newtime, true);
			if (idx == -1) {
				continue;
			}
			SelectedKey sk;
			sk.key = idx;
			sk.track = E->key().track;
			if (selection.has(sk)) {
				continue; // Already in selection, don't save.
			}

			undo_redo->add_do_method(animation.ptr(), "track_remove_key_at_time", E->key().track, newtime);
			_AnimMoveRestore amr;

			amr.key = animation->track_get_key_value(E->key().track, idx);
			amr.track = E->key().track;
			amr.time = newtime;
			amr.transition = animation->track_get_key_transition(E->key().track, idx);

			to_restore.push_back(amr);
		}

#define NEW_POS(m_ofs) (((s > 0) ? m_ofs : from_t + (len - (m_ofs - from_t))) - pivot) * ABS(s) + from_t
		// 3 - Move the keys (re insert them).
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			float newpos = NEW_POS(E->get().pos);
			undo_redo->add_do_method(animation.ptr(), "track_insert_key", E->key().track, newpos, animation->track_get_key_value(E->key().track, E->key().key), animation->track_get_key_transition(E->key().track, E->key().key));
		}

		// 4 - (Undo) Remove inserted keys.
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			float newpos = NEW_POS(E->get().pos);
			undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", E->key().track, newpos);
		}

		// 5 - (Undo) Reinsert keys.
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			undo_redo->add_undo_method(animation.ptr(), "track_insert_key", E->key().track, E->get().pos, animation->track_get_key_value(E->key().track, E->key().key), animation->track_get_key_transition(E->key().track, E->key().key));
		}

		// 6 - (Undo) Reinsert overlapped keys.
		for (List<_AnimMoveRestore>::Element* amr = to_restore.front(); amr; amr = amr->next()) {
			undo_redo->add_undo_method(animation.ptr(), "track_insert_key", amr->get().track, amr->get().time, amr->get().key, amr->get().transition);
		}

		undo_redo->add_do_method(this, "_clear_selection_for_anim", animation);
		undo_redo->add_undo_method(this, "_clear_selection_for_anim", animation);

		// 7-reselect.
		for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
			float oldpos = E->get().pos;
			float newpos = NEW_POS(oldpos);
			if (newpos >= 0) {
				undo_redo->add_do_method(this, "_select_at_anim", animation, E->key().track, newpos);
			}
			undo_redo->add_undo_method(this, "_select_at_anim", animation, E->key().track, oldpos);
		}
#undef NEW_POS
		undo_redo->commit_action();
	} break;
	case EDIT_DUPLICATE_SELECTION: {
		_anim_duplicate_keys(false);
	} break;
	case EDIT_DUPLICATE_TRANSPOSED: {
		_anim_duplicate_keys(true);
	} break;
	case EDIT_ADD_RESET_KEY: {
		undo_redo->create_action(TTR("Anim Add RESET Keys"));
		Ref<Animation> reset = _create_and_get_reset_animation();
		int reset_tracks = reset->get_track_count();
		Set<int> tracks_added;

		for (Map<SelectedKey, KeyInfo>::Element* E = selection.front(); E; E = E->next()) {
			const SelectedKey& sk = E->key();

			// Only add one key per track.
			if (tracks_added.has(sk.track)) {
				continue;
			}
			tracks_added.insert(sk.track);

			int dst_track = -1;

			const NodePath& path = animation->track_get_path(sk.track);
			for (int i = 0; i < reset->get_track_count(); i++) {
				if (reset->track_get_path(i) == path) {
					dst_track = i;
					break;
				}
			}

			int existing_idx = -1;
			if (dst_track == -1) {
				// If adding multiple tracks, make sure that correct track is referenced.
				dst_track = reset_tracks;
				reset_tracks++;

				undo_redo->add_do_method(reset.ptr(), "add_track", animation->track_get_type(sk.track));
				undo_redo->add_do_method(reset.ptr(), "track_set_path", dst_track, path);
				undo_redo->add_undo_method(reset.ptr(), "remove_track", dst_track);
			}
			else {
				existing_idx = reset->track_find_key(dst_track, 0, true);
			}

			undo_redo->add_do_method(reset.ptr(), "track_insert_key", dst_track, 0, animation->track_get_key_value(sk.track, sk.key), animation->track_get_key_transition(sk.track, sk.key));
			undo_redo->add_undo_method(reset.ptr(), "track_remove_key_at_time", dst_track, 0);

			if (existing_idx != -1) {
				undo_redo->add_undo_method(reset.ptr(), "track_insert_key", dst_track, 0, reset->track_get_key_value(dst_track, existing_idx), reset->track_get_key_transition(dst_track, existing_idx));
			}
		}

		undo_redo->commit_action();

	} break;
	case EDIT_DELETE_SELECTION: {
		if (selection.size()) {
			undo_redo->create_action(TTR("Anim Delete Keys"));

			for (Map<SelectedKey, KeyInfo>::Element* E = selection.back(); E; E = E->prev()) {
				undo_redo->add_do_method(animation.ptr(), "track_remove_key", E->key().track, E->key().key);
				undo_redo->add_undo_method(animation.ptr(), "track_insert_key", E->key().track, E->get().pos, animation->track_get_key_value(E->key().track, E->key().key), animation->track_get_key_transition(E->key().track, E->key().key));
			}
			undo_redo->add_do_method(this, "_clear_selection_for_anim", animation);
			undo_redo->add_undo_method(this, "_clear_selection_for_anim", animation);
			undo_redo->commit_action();
			_update_key_edit();
		}
	} break;
	case EDIT_GOTO_NEXT_STEP: {
		goto_next_step(false);
	} break;
	case EDIT_GOTO_PREV_STEP: {
		goto_prev_step(false);
	} break;
	case EDIT_APPLY_RESET: {
		//PlayerEditor::get_singleton()->get_player()->apply_reset(true);

	} break;
	case EDIT_OPTIMIZE_ANIMATION: {
		optimize_dialog->popup_centered(Size2(250, 180) * 1.0);

	} break;
	case EDIT_OPTIMIZE_ANIMATION_CONFIRM: {
		animation->optimize(optimize_linear_error->get_value(), optimize_angular_error->get_value(), optimize_max_angle->get_value());
		_update_tracks();
		undo_redo->clear_history();

	} break;
	case EDIT_CLEAN_UP_ANIMATION: {
		cleanup_dialog->popup_centered(Size2(300, 0) * 1.0);

	} break;
	case EDIT_CLEAN_UP_ANIMATION_CONFIRM: {
		if (cleanup_all->is_pressed()) {
			List<StringName> names;
			PlayerEditorControl::get_singleton()->get_player()->get_animation_list(&names);
			for (List<StringName>::Element* E = names.front(); E; E = E->next()) {
				_cleanup_animation(PlayerEditorControl::get_singleton()->get_player()->get_animation(E->get()));
			}
		}
		else {
			_cleanup_animation(animation);
		}

	} break;
	}
}

void TrackEditor::_cleanup_animation(Ref<Animation> p_animation) {
	for (int i = 0; i < p_animation->get_track_count(); i++) {
		bool prop_exists = false;
		Variant::Type valid_type = Variant::NIL;
		Object* obj = nullptr;

		RES res;
		Vector<StringName> leftover_path;

		Node* node = root->get_node_and_resource(p_animation->track_get_path(i), res, leftover_path);

		if (res.is_valid()) {
			obj = res.ptr();
		}
		else if (node) {
			obj = node;
		}

		if (obj && p_animation->track_get_type(i) == Animation::TYPE_VALUE) {
			valid_type = obj->get_static_property_type_indexed(leftover_path, &prop_exists);
		}

		if (!obj && cleanup_tracks->is_pressed()) {
			p_animation->remove_track(i);
			i--;
			continue;
		}

		if (!prop_exists || p_animation->track_get_type(i) != Animation::TYPE_VALUE || !cleanup_keys->is_pressed()) {
			continue;
		}

		for (int j = 0; j < p_animation->track_get_key_count(i); j++) {
			Variant v = p_animation->track_get_key_value(i, j);

			if (!Variant::can_convert(v.get_type(), valid_type)) {
				p_animation->track_remove_key(i, j);
				j--;
			}
		}

		if (p_animation->track_get_key_count(i) == 0 && cleanup_tracks->is_pressed()) {
			p_animation->remove_track(i);
			i--;
		}
	}

	undo_redo->clear_history();
	_update_tracks();
}

void TrackEditor::_view_group_toggle() {
	_update_tracks();
	view_group->set_icon(IconsCache::get_singleton()->get_icon(view_group->is_pressed() ? "AnimationTrackList" : "AnimationTrackGroup"));
}

bool TrackEditor::is_grouping_tracks() {
	if (!view_group) {
		return false;
	}

	return !view_group->is_pressed();
}

void TrackEditor::_selection_changed() {
	if (selected_filter->is_pressed()) {
		_update_tracks(); // Needs updatin.
	}
	else {
		for (int i = 0; i < track_edits.size(); i++) {
			track_edits[i]->update();
		}

	}
}

float TrackEditor::snap_time(float p_value, bool p_relative) {
	if (is_snap_enabled()) {
		double snap_increment;
		if (timeline->is_using_fps() && step->get_value() > 0) {
			snap_increment = 1.0 / step->get_value();
		}
		else {
			snap_increment = step->get_value();
		}

		if (Input::get_singleton()->is_key_pressed(KEY_SHIFT)) {
			// Use more precise snapping when holding Shift.
			snap_increment *= 0.25;
		}

		if (p_relative) {
			double rel = Math::fmod(timeline->get_value(), snap_increment);
			p_value = Math::stepify(p_value + rel, snap_increment) - rel;
		}
		else {
			p_value = Math::stepify(p_value, snap_increment);
		}
	}

	return p_value;
}

void TrackEditor::_show_imported_anim_warning() {
	// It looks terrible on a single line but the TTR extractor doesn't support line breaks yet.
	//EditorNode::get_singleton()->show_warning(TTR("This animation belongs to an imported scene, so changes to imported tracks will not be saved.\n\nTo enable the ability to add custom tracks, navigate to the scene's import settings and set\n\"Animation > Storage\" to \"Files\", enable \"Animation > Keep Custom Tracks\", then re-import.\nAlternatively, use an import preset that imports animations to separate files."),
		//TTR("Warning: Editing imported animation"));
}

void TrackEditor::_select_all_tracks_for_copy() {
	TreeItem* track = track_copy_select->get_root()->get_children();
	if (!track) {
		return;
	}

	bool all_selected = true;
	while (track) {
		if (!track->is_checked(0)) {
			all_selected = false;
		}

		track = track->get_next();
	}

	track = track_copy_select->get_root()->get_children();
	while (track) {
		track->set_checked(0, !all_selected);
		track = track->get_next();
	}
}

PlayerEditorControl* TrackEditor::get_control() {
	return PlayerEditorControl::get_singleton();
}

void TrackEditor::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_track_edit_type", "header", "track_edit_class"), &TrackEditor::set_track_edit_type);
	ClassDB::bind_method("is_selection_active", &TrackEditor::is_selection_active);
	ClassDB::bind_method("get_control", &TrackEditor::get_control);

	ClassDB::bind_method("_animation_update", &TrackEditor::_animation_update);
	ClassDB::bind_method("_track_grab_focus", &TrackEditor::_track_grab_focus);
	ClassDB::bind_method("_update_tracks", &TrackEditor::_update_tracks);
	ClassDB::bind_method("_clear_selection_for_anim", &TrackEditor::_clear_selection_for_anim);
	ClassDB::bind_method("_select_at_anim", &TrackEditor::_select_at_anim);

	ClassDB::bind_method("_key_selected", &TrackEditor::_key_selected); // Still used by some connect_compat.
	ClassDB::bind_method("_key_deselected", &TrackEditor::_key_deselected); // Still used by some connect_compat.
	ClassDB::bind_method("_clear_selection", &TrackEditor::_clear_selection); // Still used by some connect_compat.

	ClassDB::bind_method(D_METHOD("has_keying"), &TrackEditor::has_keying);

	ClassDB::bind_method(D_METHOD("_animation_changed"), &TrackEditor::_animation_changed);
	ClassDB::bind_method(D_METHOD("_root_removed"), &TrackEditor::_root_removed);
	ClassDB::bind_method(D_METHOD("_timeline_changed"), &TrackEditor::_timeline_changed);
	ClassDB::bind_method(D_METHOD("_track_remove_request"), &TrackEditor::_track_remove_request);
	ClassDB::bind_method(D_METHOD("_dropped_track"), &TrackEditor::_dropped_track);
	ClassDB::bind_method(D_METHOD("_insert_key_from_track"), &TrackEditor::_insert_key_from_track);
	ClassDB::bind_method(D_METHOD("_move_selection_begin"), &TrackEditor::_move_selection_begin);
	ClassDB::bind_method(D_METHOD("_move_selection"), &TrackEditor::_move_selection);
	ClassDB::bind_method(D_METHOD("_move_selection_commit"), &TrackEditor::_move_selection_commit);
	ClassDB::bind_method(D_METHOD("_move_selection_cancel"), &TrackEditor::_move_selection_cancel);
	ClassDB::bind_method(D_METHOD("_edit_menu_pressed"), &TrackEditor::_edit_menu_pressed);
	ClassDB::bind_method(D_METHOD("_name_limit_changed"), &TrackEditor::_name_limit_changed);
	ClassDB::bind_method(D_METHOD("_add_track"), &TrackEditor::_add_track);
	ClassDB::bind_method(D_METHOD("_timeline_value_changed"), &TrackEditor::_timeline_value_changed);
	ClassDB::bind_method(D_METHOD("_update_length"), &TrackEditor::_update_length);
	ClassDB::bind_method(D_METHOD("_scroll_callback"), &TrackEditor::_scroll_callback);
	ClassDB::bind_method(D_METHOD("_pan_callback"), &TrackEditor::_pan_callback);
	ClassDB::bind_method(D_METHOD("_zoom_callback"), &TrackEditor::_zoom_callback);
	ClassDB::bind_method(D_METHOD("_scroll_input"), &TrackEditor::_scroll_input);
	ClassDB::bind_method(D_METHOD("_update_scroll"), &TrackEditor::_update_scroll);
	ClassDB::bind_method(D_METHOD("_show_imported_anim_warning"), &TrackEditor::_show_imported_anim_warning);
	ClassDB::bind_method(D_METHOD("_view_group_toggle"), &TrackEditor::_view_group_toggle);
	ClassDB::bind_method(D_METHOD("_update_step"), &TrackEditor::_update_step);
	ClassDB::bind_method(D_METHOD("_snap_mode_changed"), &TrackEditor::_snap_mode_changed);
	ClassDB::bind_method(D_METHOD("_edit_menu_about_to_popup"), &TrackEditor::_edit_menu_about_to_popup);
	ClassDB::bind_method(D_METHOD("_confirm_insert_list"), &TrackEditor::_confirm_insert_list);
	ClassDB::bind_method(D_METHOD("_box_selection_draw"), &TrackEditor::_box_selection_draw);
	ClassDB::bind_method(D_METHOD("_select_all_tracks_for_copy"), &TrackEditor::_select_all_tracks_for_copy);

	ClassDB::bind_method("_icons_cache_changed", &TrackEditor::_icons_cache_changed);

	ADD_SIGNAL(MethodInfo("timeline_changed", PropertyInfo(Variant::REAL, "position"), PropertyInfo(Variant::BOOL, "drag"), PropertyInfo(Variant::BOOL, "timeline_only")));
	ADD_SIGNAL(MethodInfo("keying_changed"));
	ADD_SIGNAL(MethodInfo("animation_len_changed", PropertyInfo(Variant::REAL, "len")));
	ADD_SIGNAL(MethodInfo("animation_step_changed", PropertyInfo(Variant::REAL, "step")));
}

void TrackEditor::_icons_cache_changed() {
	_notification(NOTIFICATION_THEME_CHANGED);
	update();
}

void TrackEditor::_pick_track_filter_text_changed(const String& p_newtext) {
	/*TreeItem* root_item = pick_track->get_scene_tree()->get_scene_tree()->get_root();

	Vector<Node*> select_candidates;
	Node* to_select = nullptr;

	String filter = pick_track->get_filter_line_edit()->get_text();

	_pick_track_select_recursive(root_item, filter, select_candidates);

	if (!select_candidates.empty()) {
		for (int i = 0; i < select_candidates.size(); ++i) {
			Node* candidate = select_candidates[i];

			if (((String)candidate->get_name()).to_lower().begins_with(filter.to_lower())) {
				to_select = candidate;
				break;
			}
		}

		if (!to_select) {
			to_select = select_candidates[0];
		}
	}

	pick_track->get_scene_tree()->set_selected(to_select);*/
}

void TrackEditor::_pick_track_select_recursive(TreeItem* p_item, const String& p_filter, Vector<Node*>& p_select_candidates) {
	if (!p_item) {
		return;
	}

	NodePath np = p_item->get_metadata(0);
	Node* node = get_node(np);

	if (!p_filter.empty() && ((String)node->get_name()).findn(p_filter) != -1) {
		p_select_candidates.push_back(node);
	}

	TreeItem* c = p_item->get_children();

	while (c) {
		_pick_track_select_recursive(c, p_filter, p_select_candidates);
		c = c->get_next();
	}
}

void TrackEditor::_pick_track_filter_input(const Ref<InputEvent>& p_ie) {
	Ref<InputEventKey> k = p_ie;

	if (k.is_valid()) {
		switch (k->get_scancode()) {
		case KEY_UP:
		case KEY_DOWN:
		case KEY_PAGEUP:
		case KEY_PAGEDOWN: {
			//pick_track->get_scene_tree()->get_scene_tree()->gui_input(k);
			//pick_track->get_filter_line_edit()->accept_event();
		} break;
		default:
			break;
		}
	}
}

TrackEditor::TrackEditor() {
	root = nullptr;

	undo_redo = memnew(UndoRedo); // EditorNode::get_singleton()->get_undo_redo();

	main_panel = memnew(PanelContainer);
	main_panel->set_focus_mode(FOCUS_ALL); // Allow panel to have focus so that shortcuts work as expected.
	add_child(main_panel);
	main_panel->set_v_size_flags(SIZE_EXPAND_FILL);
	HBoxContainer* timeline_scroll = memnew(HBoxContainer);
	main_panel->add_child(timeline_scroll);
	timeline_scroll->set_v_size_flags(SIZE_EXPAND_FILL);

	VBoxContainer* timeline_vbox = memnew(VBoxContainer);
	timeline_scroll->add_child(timeline_vbox);
	timeline_vbox->set_v_size_flags(SIZE_EXPAND_FILL);
	timeline_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	timeline_vbox->add_constant_override("separation", 0);

	info_message = memnew(Label);
	info_message->set_text(TTR("Select an AnimationPlayer node to create and edit animations."));
	info_message->set_valign(Label::VALIGN_CENTER);
	info_message->set_align(Label::ALIGN_CENTER);
	info_message->set_autowrap(true);
	info_message->set_custom_minimum_size(Size2(100 * 1.0, 0));
	info_message->set_anchors_and_margins_preset(PRESET_WIDE, PRESET_MODE_KEEP_SIZE, 8 * 1.0);
	main_panel->add_child(info_message);

	timeline = memnew(TimelineEdit);
	timeline->set_undo_redo(undo_redo);
	timeline_vbox->add_child(timeline);
	timeline->connect("timeline_changed", this, "_timeline_changed");
	timeline->connect("name_limit_changed", this, "_name_limit_changed");
	timeline->connect("track_added", this, "_add_track");
	timeline->connect("value_changed", this, "_timeline_value_changed");
	timeline->connect("length_changed", this, "_update_length");

	panner.instance();
	panner->set_callbacks(this, "_scroll_callback", this, "_pan_callback", this, "_zoom_callback");

	scroll = memnew(ScrollContainer);
	timeline_vbox->add_child(scroll);
	scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	VScrollBar* sb = scroll->get_v_scrollbar();
	scroll->remove_child(sb);
	timeline_scroll->add_child(sb); // Move here so timeline and tracks are always aligned.
	scroll->set_focus_mode(FOCUS_CLICK);
	scroll->connect("gui_input", this, "_scroll_input");
	scroll->connect("focus_exited", panner.ptr(), "release_pan_key");

	timeline_vbox->set_custom_minimum_size(Size2(0, 150) * 1.0);

	hscroll = memnew(HScrollBar);
	hscroll->share(timeline);
	hscroll->hide();
	hscroll->connect("value_changed", this, "_update_scroll");
	timeline_vbox->add_child(hscroll);
	timeline->set_hscroll(hscroll);

	track_vbox = memnew(VBoxContainer);
	scroll->add_child(track_vbox);
	track_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
	scroll->set_enable_h_scroll(false);
	track_vbox->add_constant_override("separation", 0);

	HBoxContainer* bottom_hb = memnew(HBoxContainer);
	add_child(bottom_hb);

	imported_anim_warning = memnew(Button);
	imported_anim_warning->hide();
	imported_anim_warning->set_tooltip(TTR("Warning: Editing imported animation"));
	imported_anim_warning->connect("pressed", this, "_show_imported_anim_warning");
	bottom_hb->add_child(imported_anim_warning);

	bottom_hb->add_spacer();

	selected_filter = memnew(Button);
	selected_filter->set_flat(true);
	selected_filter->connect("pressed", this, "_view_group_toggle"); // Same function works the same.
	selected_filter->set_toggle_mode(true);
	selected_filter->set_tooltip(TTR("Only show tracks from nodes selected in tree."));

	bottom_hb->add_child(selected_filter);

	view_group = memnew(Button);
	view_group->set_flat(true);
	view_group->connect("pressed", this, "_view_group_toggle");
	view_group->set_toggle_mode(true);
	view_group->set_tooltip(TTR("Group tracks by node or display them as plain list."));

	bottom_hb->add_child(view_group);
	bottom_hb->add_child(memnew(VSeparator));

	snap = memnew(Button);
	snap->set_flat(true);
	snap->set_text("Snap: ");
	bottom_hb->add_child(snap);
	snap->set_disabled(true);
	snap->set_toggle_mode(true);
	snap->set_pressed(true);

	step = memnew(SpinBox);
	step->set_min(0);
	step->set_max(1000000);
	step->set_step(0.001);
	//step->set_hide_slider(true);
	step->set_custom_minimum_size(Size2(100, 0) * 1.0);
	step->set_tooltip(TTR("Animation step value."));
	bottom_hb->add_child(step);
	step->connect("value_changed", this, "_update_step");
	//step->set_read_only(true);

	snap_mode = memnew(OptionButton);
	snap_mode->add_item(TTR("Seconds"));
	snap_mode->add_item(TTR("FPS"));
	bottom_hb->add_child(snap_mode);
	snap_mode->connect("item_selected", this, "_snap_mode_changed");
	snap_mode->set_disabled(true);

	bottom_hb->add_child(memnew(VSeparator));

	zoom_icon = memnew(TextureRect);
	zoom_icon->set_v_size_flags(SIZE_SHRINK_CENTER);
	bottom_hb->add_child(zoom_icon);
	zoom = memnew(HSlider);
	zoom->set_step(0.01);
	zoom->set_min(0.0);
	zoom->set_max(2.0);
	zoom->set_value(1.0);
	zoom->set_custom_minimum_size(Size2(200, 0) * 1.0);
	zoom->set_v_size_flags(SIZE_SHRINK_CENTER);
	bottom_hb->add_child(zoom);
	timeline->set_zoom(zoom);

	insert_confirm = memnew(ConfirmationDialog);
	add_child(insert_confirm);
	insert_confirm->connect("confirmed", this, "_confirm_insert_list");
	VBoxContainer* icvb = memnew(VBoxContainer);
	insert_confirm->add_child(icvb);
	insert_confirm_text = memnew(Label);
	icvb->add_child(insert_confirm_text);
	HBoxContainer* ichb = memnew(HBoxContainer);
	icvb->add_child(ichb);
	insert_confirm_bezier = memnew(CheckBox);
	insert_confirm_bezier->set_text(TTR("Use Bezier Curves"));
	//insert_confirm_bezier->set_pressed(EDITOR_GET("editors/animation/default_create_bezier_tracks"));
	ichb->add_child(insert_confirm_bezier);
	insert_confirm_reset = memnew(CheckBox);
	insert_confirm_reset->set_text(TTR("Create RESET Track(s)"));
	//insert_confirm_reset->set_pressed(EDITOR_GET("editors/animation/default_create_reset_tracks"));
	ichb->add_child(insert_confirm_reset);
	key_edit = nullptr;
	multi_key_edit = nullptr;

	box_selection = memnew(Control);
	add_child(box_selection);
	box_selection->set_as_toplevel(true);
	box_selection->set_mouse_filter(MOUSE_FILTER_IGNORE);
	box_selection->hide();
	box_selection->connect("draw", this, "_box_selection_draw");

	// Default Plugins.

	Ref<TrackEditDefaultPlugin> def_plugin;
	def_plugin.instance();
	add_track_edit_plugin(def_plugin);

	// Dialogs.

	optimize_dialog = memnew(ConfirmationDialog);
	add_child(optimize_dialog);
	optimize_dialog->set_title(TTR("Anim. Optimizer"));
	VBoxContainer* optimize_vb = memnew(VBoxContainer);
	optimize_dialog->add_child(optimize_vb);

	optimize_linear_error = memnew(SpinBox);
	optimize_linear_error->set_max(1.0);
	optimize_linear_error->set_min(0.001);
	optimize_linear_error->set_step(0.001);
	optimize_linear_error->set_value(0.05);
	optimize_vb->add_margin_child(TTR("Max. Linear Error:"), optimize_linear_error);
	optimize_angular_error = memnew(SpinBox);
	optimize_angular_error->set_max(1.0);
	optimize_angular_error->set_min(0.001);
	optimize_angular_error->set_step(0.001);
	optimize_angular_error->set_value(0.01);

	optimize_vb->add_margin_child(TTR("Max. Angular Error:"), optimize_angular_error);
	optimize_max_angle = memnew(SpinBox);
	optimize_vb->add_margin_child(TTR("Max Optimizable Angle:"), optimize_max_angle);
	optimize_max_angle->set_max(360.0);
	optimize_max_angle->set_min(0.0);
	optimize_max_angle->set_step(0.1);
	optimize_max_angle->set_value(22);

	optimize_dialog->get_ok()->set_text(TTR("Optimize"));
	optimize_dialog->connect("confirmed", this, "_edit_menu_pressed", varray(EDIT_OPTIMIZE_ANIMATION_CONFIRM));

	//

	cleanup_dialog = memnew(ConfirmationDialog);
	add_child(cleanup_dialog);
	VBoxContainer* cleanup_vb = memnew(VBoxContainer);
	cleanup_dialog->add_child(cleanup_vb);

	cleanup_keys = memnew(CheckBox);
	cleanup_keys->set_text(TTR("Remove invalid keys"));
	cleanup_keys->set_pressed(true);
	cleanup_vb->add_child(cleanup_keys);

	cleanup_tracks = memnew(CheckBox);
	cleanup_tracks->set_text(TTR("Remove unresolved and empty tracks"));
	cleanup_tracks->set_pressed(true);
	cleanup_vb->add_child(cleanup_tracks);

	cleanup_all = memnew(CheckBox);
	cleanup_all->set_text(TTR("Clean-up all animations"));
	cleanup_vb->add_child(cleanup_all);

	cleanup_dialog->set_title(TTR("Clean-Up Animation(s) (NO UNDO!)"));
	cleanup_dialog->get_ok()->set_text(TTR("Clean-Up"));

	cleanup_dialog->connect("confirmed", this, "_edit_menu_pressed", varray(EDIT_CLEAN_UP_ANIMATION_CONFIRM));

	//
	scale_dialog = memnew(ConfirmationDialog);
	VBoxContainer* vbc = memnew(VBoxContainer);
	scale_dialog->add_child(vbc);

	scale = memnew(SpinBox);
	scale->set_min(-99999);
	scale->set_max(99999);
	scale->set_step(0.001);
	vbc->add_margin_child(TTR("Scale Ratio:"), scale);
	scale_dialog->connect("confirmed", this, "_edit_menu_pressed", varray(EDIT_SCALE_CONFIRM));
	add_child(scale_dialog);

	track_copy_dialog = memnew(ConfirmationDialog);
	add_child(track_copy_dialog);
	track_copy_dialog->set_title(TTR("Select Tracks to Copy"));
	track_copy_dialog->get_ok()->set_text(TTR("Copy"));

	VBoxContainer* track_vbox = memnew(VBoxContainer);
	track_copy_dialog->add_child(track_vbox);

	Button* select_all_button = memnew(Button);
	select_all_button->set_text(TTR("Select All/None"));
	select_all_button->connect("pressed", this, "_select_all_tracks_for_copy");
	track_vbox->add_child(select_all_button);

	track_copy_select = memnew(Tree);
	track_copy_select->set_h_size_flags(SIZE_EXPAND_FILL);
	track_copy_select->set_v_size_flags(SIZE_EXPAND_FILL);
	track_copy_select->set_hide_root(true);
	track_vbox->add_child(track_copy_select);
	track_copy_dialog->connect("confirmed", this, "_edit_menu_pressed", varray(EDIT_COPY_TRACKS_CONFIRM));

	IconsCache::get_singleton()->connect("icons_changed", this, "_icons_cache_changed");
}

TrackEditor::~TrackEditor() {
	if (undo_redo) {
		memdelete(undo_redo);
	}
	if (key_edit) {
		memdelete(key_edit);
	}
	if (multi_key_edit) {
		memdelete(multi_key_edit);
	}
}
