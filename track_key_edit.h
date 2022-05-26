#ifndef TRACK_KEY_EDIT_H
#define TRACK_KEY_EDIT_H

#include <core/object.h>
#include <scene/main/viewport.h>
#include <servers/audio/audio_stream.h>

class TrackKeyEdit : public Object {
	GDCLASS(TrackKeyEdit, Object);

public:
	bool setting = false;

	bool _hide_script_from_inspector() {
		return true;
	}

	bool _dont_undo_redo() {
		return true;
	}

	static void _bind_methods() {
		ClassDB::bind_method("_update_obj", &TrackKeyEdit::_update_obj);
		ClassDB::bind_method("_key_ofs_changed", &TrackKeyEdit::_key_ofs_changed);
		ClassDB::bind_method("_hide_script_from_inspector", &TrackKeyEdit::_hide_script_from_inspector);
		ClassDB::bind_method("get_root_path", &TrackKeyEdit::get_root_path);
		ClassDB::bind_method("_dont_undo_redo", &TrackKeyEdit::_dont_undo_redo);
	}

	void _fix_node_path(Variant& value) {
		NodePath np = value;

		if (np == NodePath()) {
			return;
		}


		Viewport* root = SceneTree::get_singleton()->get_root();

		Node* np_node = root->get_node(np);
		ERR_FAIL_COND(!np_node);

		Node* edited_node = root->get_node(base);
		ERR_FAIL_COND(!edited_node);

		value = edited_node->get_path_to(np_node);
	}

	void _update_obj(const Ref<Animation>& p_anim) {
		if (setting || animation != p_anim) {
			return;
		}

		notify_change();
	}

	void _key_ofs_changed(const Ref<Animation>& p_anim, float from, float to) {
		if (animation != p_anim || from != key_ofs) {
			return;
		}

		key_ofs = to;

		if (setting) {
			return;
		}

		notify_change();
	}

	bool _set(const StringName& p_name, const Variant& p_value) {
		int key = animation->track_find_key(track, key_ofs, true);
		ERR_FAIL_COND_V(key == -1, false);

		String name = p_name;
		if (name == "time" || name == "frame") {
			float new_time = p_value;

			if (name == "frame") {
				float fps = animation->get_step();
				if (fps > 0) {
					fps = 1.0 / fps;
				}
				new_time /= fps;
			}

			if (new_time == key_ofs) {
				return true;
			}

			int existing = animation->track_find_key(track, new_time, true);

			setting = true;
			undo_redo->create_action(TTR("Anim Change Keyframe Time"), UndoRedo::MERGE_ENDS);

			Variant val = animation->track_get_key_value(track, key);
			float trans = animation->track_get_key_transition(track, key);

			undo_redo->add_do_method(animation.ptr(), "track_remove_key", track, key);
			undo_redo->add_do_method(animation.ptr(), "track_insert_key", track, new_time, val, trans);
			undo_redo->add_do_method(this, "_key_ofs_changed", animation, key_ofs, new_time);
			undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", track, new_time);
			undo_redo->add_undo_method(animation.ptr(), "track_insert_key", track, key_ofs, val, trans);
			undo_redo->add_undo_method(this, "_key_ofs_changed", animation, new_time, key_ofs);

			if (existing != -1) {
				Variant v = animation->track_get_key_value(track, existing);
				trans = animation->track_get_key_transition(track, existing);
				undo_redo->add_undo_method(animation.ptr(), "track_insert_key", track, new_time, v, trans);
			}
			undo_redo->commit_action();

			setting = false;
			return true;
		}

		if (name == "easing") {
			float val = p_value;
			float prev_val = animation->track_get_key_transition(track, key);
			setting = true;
			undo_redo->create_action(TTR("Anim Change Transition"), UndoRedo::MERGE_ENDS);
			undo_redo->add_do_method(animation.ptr(), "track_set_key_transition", track, key, val);
			undo_redo->add_undo_method(animation.ptr(), "track_set_key_transition", track, key, prev_val);
			undo_redo->add_do_method(this, "_update_obj", animation);
			undo_redo->add_undo_method(this, "_update_obj", animation);
			undo_redo->commit_action();

			setting = false;
			return true;
		}

		switch (animation->track_get_type(track)) {
		case Animation::TYPE_TRANSFORM: {
			Dictionary d_old = animation->track_get_key_value(track, key);
			Dictionary d_new = d_old.duplicate();
			d_new[p_name] = p_value;
			setting = true;
			undo_redo->create_action(TTR("Anim Change Transform"));
			undo_redo->add_do_method(animation.ptr(), "track_set_key_value", track, key, d_new);
			undo_redo->add_undo_method(animation.ptr(), "track_set_key_value", track, key, d_old);
			undo_redo->add_do_method(this, "_update_obj", animation);
			undo_redo->add_undo_method(this, "_update_obj", animation);
			undo_redo->commit_action();

			setting = false;
			return true;
		} break;

		case Animation::TYPE_VALUE: {
			if (name == "value") {
				Variant value = p_value;

				if (value.get_type() == Variant::NODE_PATH) {
					_fix_node_path(value);
				}

				setting = true;
				undo_redo->create_action(TTR("Anim Change Keyframe Value"), UndoRedo::MERGE_ENDS);
				Variant prev = animation->track_get_key_value(track, key);
				undo_redo->add_do_method(animation.ptr(), "track_set_key_value", track, key, value);
				undo_redo->add_undo_method(animation.ptr(), "track_set_key_value", track, key, prev);
				undo_redo->add_do_method(this, "_update_obj", animation);
				undo_redo->add_undo_method(this, "_update_obj", animation);
				undo_redo->commit_action();

				setting = false;
				return true;
			}
		} break;

		case Animation::TYPE_METHOD: {
			Dictionary d_old = animation->track_get_key_value(track, key);
			Dictionary d_new = d_old.duplicate();

			bool change_notify_deserved = false;
			bool mergeable = false;

			if (name == "name") {
				d_new["method"] = p_value;
			}
			else if (name == "arg_count") {
				Vector<Variant> args = d_old["args"];
				args.resize(p_value);
				d_new["args"] = args;
				change_notify_deserved = true;
			}
			else if (name.begins_with("args/")) {
				Vector<Variant> args = d_old["args"];
				int idx = name.get_slice("/", 1).to_int();
				ERR_FAIL_INDEX_V(idx, args.size(), false);

				String what = name.get_slice("/", 2);
				if (what == "type") {
					Variant::Type t = Variant::Type(int(p_value));

					if (t != args[idx].get_type()) {
						Variant::CallError err;
						if (Variant::can_convert(args[idx].get_type(), t)) {
							Variant old = args[idx];
							Variant* ptrs[1] = { &old };
							args.write[idx] = Variant::construct(t, (const Variant**)ptrs, 1, err);
						}
						else {
							args.write[idx] = Variant::construct(t, nullptr, 0, err);
						}
						change_notify_deserved = true;
						d_new["args"] = args;
					}
				}
				else if (what == "value") {
					Variant value = p_value;
					if (value.get_type() == Variant::NODE_PATH) {
						_fix_node_path(value);
					}

					args.write[idx] = value;
					d_new["args"] = args;
					mergeable = true;
				}
			}

			if (mergeable) {
				undo_redo->create_action(TTR("Anim Change Call"), UndoRedo::MERGE_ENDS);
			}
			else {
				undo_redo->create_action(TTR("Anim Change Call"));
			}

			setting = true;
			undo_redo->add_do_method(animation.ptr(), "track_set_key_value", track, key, d_new);
			undo_redo->add_undo_method(animation.ptr(), "track_set_key_value", track, key, d_old);
			undo_redo->add_do_method(this, "_update_obj", animation);
			undo_redo->add_undo_method(this, "_update_obj", animation);
			undo_redo->commit_action();

			setting = false;
			if (change_notify_deserved) {
				notify_change();
			}
			return true;
		} break;

		case Animation::TYPE_AUDIO: {
			if (name == "stream") {
				Ref<AudioStream> stream = p_value;

				setting = true;
				undo_redo->create_action(TTR("Anim Change Keyframe Value"), UndoRedo::MERGE_ENDS);
				RES prev = animation->audio_track_get_key_stream(track, key);
				undo_redo->add_do_method(animation.ptr(), "audio_track_set_key_stream", track, key, stream);
				undo_redo->add_undo_method(animation.ptr(), "audio_track_set_key_stream", track, key, prev);
				undo_redo->add_do_method(this, "_update_obj", animation);
				undo_redo->add_undo_method(this, "_update_obj", animation);
				undo_redo->commit_action();

				setting = false;
				return true;
			}

			if (name == "start_offset") {
				float value = p_value;

				setting = true;
				undo_redo->create_action(TTR("Anim Change Keyframe Value"), UndoRedo::MERGE_ENDS);
				float prev = animation->audio_track_get_key_start_offset(track, key);
				undo_redo->add_do_method(animation.ptr(), "audio_track_set_key_start_offset", track, key, value);
				undo_redo->add_undo_method(animation.ptr(), "audio_track_set_key_start_offset", track, key, prev);
				undo_redo->add_do_method(this, "_update_obj", animation);
				undo_redo->add_undo_method(this, "_update_obj", animation);
				undo_redo->commit_action();

				setting = false;
				return true;
			}

			if (name == "end_offset") {
				float value = p_value;

				setting = true;
				undo_redo->create_action(TTR("Anim Change Keyframe Value"), UndoRedo::MERGE_ENDS);
				float prev = animation->audio_track_get_key_end_offset(track, key);
				undo_redo->add_do_method(animation.ptr(), "audio_track_set_key_end_offset", track, key, value);
				undo_redo->add_undo_method(animation.ptr(), "audio_track_set_key_end_offset", track, key, prev);
				undo_redo->add_do_method(this, "_update_obj", animation);
				undo_redo->add_undo_method(this, "_update_obj", animation);
				undo_redo->commit_action();

				setting = false;
				return true;
			}
		} break;
		case Animation::TYPE_ANIMATION: {
			if (name == "animation") {
				StringName anim_name = p_value;

				setting = true;
				undo_redo->create_action(TTR("Anim Change Keyframe Value"), UndoRedo::MERGE_ENDS);
				StringName prev = animation->animation_track_get_key_animation(track, key);
				undo_redo->add_do_method(animation.ptr(), "animation_track_set_key_animation", track, key, anim_name);
				undo_redo->add_undo_method(animation.ptr(), "animation_track_set_key_animation", track, key, prev);
				undo_redo->add_do_method(this, "_update_obj", animation);
				undo_redo->add_undo_method(this, "_update_obj", animation);
				undo_redo->commit_action();

				setting = false;
				return true;
			}
		} break;
		}

		return false;
	}

	bool _get(const StringName& p_name, Variant& r_ret) const {
		int key = animation->track_find_key(track, key_ofs, true);
		ERR_FAIL_COND_V(key == -1, false);

		String name = p_name;
		if (name == "time") {
			r_ret = key_ofs;
			return true;
		}

		if (name == "frame") {
			float fps = animation->get_step();
			if (fps > 0) {
				fps = 1.0 / fps;
			}
			r_ret = key_ofs * fps;
			return true;
		}

		if (name == "easing") {
			r_ret = animation->track_get_key_transition(track, key);
			return true;
		}

		switch (animation->track_get_type(track)) {
		case Animation::TYPE_TRANSFORM: {
			Dictionary d = animation->track_get_key_value(track, key);
			ERR_FAIL_COND_V(!d.has(name), false);
			r_ret = d[p_name];
			return true;

		} break;

		case Animation::TYPE_VALUE: {
			if (name == "value") {
				r_ret = animation->track_get_key_value(track, key);
				return true;
			}

		} break;
		case Animation::TYPE_METHOD: {
			Dictionary d = animation->track_get_key_value(track, key);

			if (name == "name") {
				ERR_FAIL_COND_V(!d.has("method"), false);
				r_ret = d["method"];
				return true;
			}

			ERR_FAIL_COND_V(!d.has("args"), false);

			Vector<Variant> args = d["args"];

			if (name == "arg_count") {
				r_ret = args.size();
				return true;
			}

			if (name.begins_with("args/")) {
				int idx = name.get_slice("/", 1).to_int();
				ERR_FAIL_INDEX_V(idx, args.size(), false);

				String what = name.get_slice("/", 2);
				if (what == "type") {
					r_ret = args[idx].get_type();
					return true;
				}

				if (what == "value") {
					r_ret = args[idx];
					return true;
				}
			}

		} break;
		case Animation::TYPE_AUDIO: {
			if (name == "stream") {
				r_ret = animation->audio_track_get_key_stream(track, key);
				return true;
			}

			if (name == "start_offset") {
				r_ret = animation->audio_track_get_key_start_offset(track, key);
				return true;
			}

			if (name == "end_offset") {
				r_ret = animation->audio_track_get_key_end_offset(track, key);
				return true;
			}

		} break;
		case Animation::TYPE_ANIMATION: {
			if (name == "animation") {
				r_ret = animation->animation_track_get_key_animation(track, key);
				return true;
			}

		} break;
		}

		return false;
	}
	void _get_property_list(List<PropertyInfo>* p_list) const {
		if (animation.is_null()) {
			return;
		}

		ERR_FAIL_INDEX(track, animation->get_track_count());
		int key = animation->track_find_key(track, key_ofs, true);
		ERR_FAIL_COND(key == -1);

		if (use_fps && animation->get_step() > 0) {
			float max_frame = animation->get_length() / animation->get_step();
			p_list->push_back(PropertyInfo(Variant::REAL, "frame", PROPERTY_HINT_RANGE, "0," + rtos(max_frame) + ",1"));
		}
		else {
			p_list->push_back(PropertyInfo(Variant::REAL, "time", PROPERTY_HINT_RANGE, "0," + rtos(animation->get_length()) + ",0.01"));
		}

		switch (animation->track_get_type(track)) {
		case Animation::TYPE_TRANSFORM: {
			p_list->push_back(PropertyInfo(Variant::VECTOR3, "location"));
			p_list->push_back(PropertyInfo(Variant::QUAT, "rotation"));
			p_list->push_back(PropertyInfo(Variant::VECTOR3, "scale"));

		} break;

		case Animation::TYPE_VALUE: {
			Variant v = animation->track_get_key_value(track, key);

			if (hint.type != Variant::NIL) {
				PropertyInfo pi = hint;
				pi.name = "value";
				p_list->push_back(pi);
			}
			else {
				PropertyHint hint = PROPERTY_HINT_NONE;
				String hint_string;

				if (v.get_type() == Variant::OBJECT) {
					// Could actually check the object property if exists..? Yes I will!
					Ref<Resource> res = v;
					if (res.is_valid()) {
						hint = PROPERTY_HINT_RESOURCE_TYPE;
						hint_string = res->get_class();
					}
				}

				if (v.get_type() != Variant::NIL) {
					p_list->push_back(PropertyInfo(v.get_type(), "value", hint, hint_string));
				}
			}

		} break;
		case Animation::TYPE_METHOD: {
			p_list->push_back(PropertyInfo(Variant::STRING, "name"));
			p_list->push_back(PropertyInfo(Variant::INT, "arg_count", PROPERTY_HINT_RANGE, "0,32,1,or_greater"));

			Dictionary d = animation->track_get_key_value(track, key);
			ERR_FAIL_COND(!d.has("args"));
			Vector<Variant> args = d["args"];
			String vtypes;
			for (int i = 0; i < Variant::VARIANT_MAX; i++) {
				if (i > 0) {
					vtypes += ",";
				}
				vtypes += Variant::get_type_name(Variant::Type(i));
			}

			for (int i = 0; i < args.size(); i++) {
				p_list->push_back(PropertyInfo(Variant::INT, "args/" + itos(i) + "/type", PROPERTY_HINT_ENUM, vtypes));
				if (args[i].get_type() != Variant::NIL) {
					p_list->push_back(PropertyInfo(args[i].get_type(), "args/" + itos(i) + "/value"));
				}
			}

		} break;
		case Animation::TYPE_BEZIER: {
			p_list->push_back(PropertyInfo(Variant::REAL, "value"));
			p_list->push_back(PropertyInfo(Variant::VECTOR2, "in_handle"));
			p_list->push_back(PropertyInfo(Variant::VECTOR2, "out_handle"));
			p_list->push_back(PropertyInfo(Variant::INT, "handle_mode", PROPERTY_HINT_ENUM, "Free,Balanced"));

		} break;
		case Animation::TYPE_AUDIO: {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"));
			p_list->push_back(PropertyInfo(Variant::REAL, "start_offset", PROPERTY_HINT_RANGE, "0,3600,0.01,or_greater"));
			p_list->push_back(PropertyInfo(Variant::REAL, "end_offset", PROPERTY_HINT_RANGE, "0,3600,0.01,or_greater"));

		} break;
		case Animation::TYPE_ANIMATION: {
			String animations;

			if (root_path && root_path->has_node(animation->track_get_path(track))) {
				AnimationPlayer* ap = Object::cast_to<AnimationPlayer>(root_path->get_node(animation->track_get_path(track)));
				if (ap) {
					List<StringName> anims;
					ap->get_animation_list(&anims);
					for (List<StringName>::Element* E = anims.front(); E; E = E->next()) {
						if (!animations.empty()) {
							animations += ",";
						}

						animations += String(E->get());
					}
				}
			}

			if (!animations.empty()) {
				animations += ",";
			}
			animations += "[stop]";

			p_list->push_back(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_ENUM, animations));

		} break;
		}

		if (animation->track_get_type(track) == Animation::TYPE_VALUE) {
			p_list->push_back(PropertyInfo(Variant::REAL, "easing", PROPERTY_HINT_EXP_EASING));
		}
	}

	UndoRedo* undo_redo = nullptr;
	Ref<Animation> animation;
	int track = -1;
	float key_ofs = 0;
	Node* root_path = nullptr;

	PropertyInfo hint;
	NodePath base;
	bool use_fps = false;

	void notify_change() {
		property_list_changed_notify();
	}

	Node* get_root_path() {
		return root_path;
	}

	void set_use_fps(bool p_enable) {
		use_fps = p_enable;
		property_list_changed_notify();
	}
};

class MultiTrackKeyEdit : public Object {
	GDCLASS(MultiTrackKeyEdit, Object);

public:
	bool setting = false;

	bool _hide_script_from_inspector() {
		return true;
	}

	bool _dont_undo_redo() {
		return true;
	}

	static void _bind_methods() {
		ClassDB::bind_method("_update_obj", &MultiTrackKeyEdit::_update_obj);
		ClassDB::bind_method("_key_ofs_changed", &MultiTrackKeyEdit::_key_ofs_changed);
		ClassDB::bind_method("_hide_script_from_inspector", &MultiTrackKeyEdit::_hide_script_from_inspector);
		ClassDB::bind_method("get_root_path", &MultiTrackKeyEdit::get_root_path);
		ClassDB::bind_method("_dont_undo_redo", &MultiTrackKeyEdit::_dont_undo_redo);
	}

	void _fix_node_path(Variant& value, NodePath& base) {
		NodePath np = value;

		if (np == NodePath()) {
			return;
		}

		Node* root = SceneTree::get_singleton()->get_root();

		Node* np_node = root->get_node(np);
		ERR_FAIL_COND(!np_node);

		Node* edited_node = root->get_node(base);
		ERR_FAIL_COND(!edited_node);

		value = edited_node->get_path_to(np_node);
	}

	void _update_obj(const Ref<Animation>& p_anim) {
		if (setting || animation != p_anim) {
			return;
		}

		notify_change();
	}

	void _key_ofs_changed(const Ref<Animation>& p_anim, float from, float to) {
		if (animation != p_anim) {
			return;
		}

		for (Map<int, List<float>>::Element* E = key_ofs_map.front(); E; E = E->next()) {
			int key = 0;
			for (List<float>::Element* key_ofs = E->value().front(); key_ofs; key_ofs = key_ofs->next()) {
				if (from != key_ofs->get()) {
					key++;
					continue;
				}

				int track = E->key();
				key_ofs_map[track][key] = to;

				if (setting) {
					return;
				}

				notify_change();

				return;
			}
		}
	}

	bool _set(const StringName& p_name, const Variant& p_value) {
		bool update_obj = false;
		bool change_notify_deserved = false;
		for (Map<int, List<float>>::Element* E = key_ofs_map.front(); E; E = E->next()) {
			int track = E->key();
			for (List<float>::Element* key_ofs = E->value().front(); key_ofs; key_ofs = key_ofs->next()) {
				int key = animation->track_find_key(track, key_ofs->get(), true);
				ERR_FAIL_COND_V(key == -1, false);

				String name = p_name;
				if (name == "time" || name == "frame") {
					float new_time = p_value;

					if (name == "frame") {
						float fps = animation->get_step();
						if (fps > 0) {
							fps = 1.0 / fps;
						}
						new_time /= fps;
					}

					int existing = animation->track_find_key(track, new_time, true);

					if (!setting) {
						setting = true;
						undo_redo->create_action(TTR("Anim Multi Change Keyframe Time"), UndoRedo::MERGE_ENDS);
					}

					Variant val = animation->track_get_key_value(track, key);
					float trans = animation->track_get_key_transition(track, key);

					undo_redo->add_do_method(animation.ptr(), "track_remove_key", track, key);
					undo_redo->add_do_method(animation.ptr(), "track_insert_key", track, new_time, val, trans);
					undo_redo->add_do_method(this, "_key_ofs_changed", animation, key_ofs, new_time);
					undo_redo->add_undo_method(animation.ptr(), "track_remove_key_at_time", track, new_time);
					undo_redo->add_undo_method(animation.ptr(), "track_insert_key", track, key_ofs, val, trans);
					undo_redo->add_undo_method(this, "_key_ofs_changed", animation, new_time, key_ofs);

					if (existing != -1) {
						Variant v = animation->track_get_key_value(track, existing);
						trans = animation->track_get_key_transition(track, existing);
						undo_redo->add_undo_method(animation.ptr(), "track_insert_key", track, new_time, v, trans);
					}
				}
				else if (name == "easing") {
					float val = p_value;
					float prev_val = animation->track_get_key_transition(track, key);

					if (!setting) {
						setting = true;
						undo_redo->create_action(TTR("Anim Multi Change Transition"), UndoRedo::MERGE_ENDS);
					}
					undo_redo->add_do_method(animation.ptr(), "track_set_key_transition", track, key, val);
					undo_redo->add_undo_method(animation.ptr(), "track_set_key_transition", track, key, prev_val);
					update_obj = true;
				}

				switch (animation->track_get_type(track)) {
				case Animation::TYPE_TRANSFORM: {
					Dictionary d_old = animation->track_get_key_value(track, key);
					Dictionary d_new = d_old.duplicate();
					d_new[p_name] = p_value;

					if (!setting) {
						setting = true;
						undo_redo->create_action(TTR("Anim Multi Change Transform"));
					}
					undo_redo->add_do_method(animation.ptr(), "track_set_key_value", track, key, d_new);
					undo_redo->add_undo_method(animation.ptr(), "track_set_key_value", track, key, d_old);
					update_obj = true;
				} break;

				case Animation::TYPE_VALUE: {
					if (name == "value") {
						Variant value = p_value;

						if (value.get_type() == Variant::NODE_PATH) {
							_fix_node_path(value, base_map[track]);
						}

						if (!setting) {
							setting = true;
							undo_redo->create_action(TTR("Anim Multi Change Keyframe Value"), UndoRedo::MERGE_ENDS);
						}
						Variant prev = animation->track_get_key_value(track, key);
						undo_redo->add_do_method(animation.ptr(), "track_set_key_value", track, key, value);
						undo_redo->add_undo_method(animation.ptr(), "track_set_key_value", track, key, prev);
						update_obj = true;
					}
				} break;

				case Animation::TYPE_METHOD: {
					Dictionary d_old = animation->track_get_key_value(track, key);
					Dictionary d_new = d_old.duplicate();

					bool mergeable = false;

					if (name == "name") {
						d_new["method"] = p_value;
					}
					else if (name == "arg_count") {
						Vector<Variant> args = d_old["args"];
						args.resize(p_value);
						d_new["args"] = args;
						change_notify_deserved = true;
					}
					else if (name.begins_with("args/")) {
						Vector<Variant> args = d_old["args"];
						int idx = name.get_slice("/", 1).to_int();
						ERR_FAIL_INDEX_V(idx, args.size(), false);

						String what = name.get_slice("/", 2);
						if (what == "type") {
							Variant::Type t = Variant::Type(int(p_value));

							if (t != args[idx].get_type()) {
								Variant::CallError err;
								if (Variant::can_convert(args[idx].get_type(), t)) {
									Variant old = args[idx];
									Variant* ptrs[1] = { &old };
									args.write[idx] = Variant::construct(t, (const Variant**)ptrs, 1, err);
								}
								else {
									args.write[idx] = Variant::construct(t, nullptr, 0, err);
								}
								change_notify_deserved = true;
								d_new["args"] = args;
							}
						}
						else if (what == "value") {
							Variant value = p_value;
							if (value.get_type() == Variant::NODE_PATH) {
								_fix_node_path(value, base_map[track]);
							}

							args.write[idx] = value;
							d_new["args"] = args;
							mergeable = true;
						}
					}

					Variant prev = animation->track_get_key_value(track, key);

					if (!setting) {
						if (mergeable) {
							undo_redo->create_action(TTR("Anim Multi Change Call"), UndoRedo::MERGE_ENDS);
						}
						else {
							undo_redo->create_action(TTR("Anim Multi Change Call"));
						}

						setting = true;
					}

					undo_redo->add_do_method(animation.ptr(), "track_set_key_value", track, key, d_new);
					undo_redo->add_undo_method(animation.ptr(), "track_set_key_value", track, key, d_old);
					update_obj = true;
				} break;

				case Animation::TYPE_AUDIO: {
					if (name == "stream") {
						Ref<AudioStream> stream = p_value;

						if (!setting) {
							setting = true;
							undo_redo->create_action(TTR("Anim Multi Change Keyframe Value"), UndoRedo::MERGE_ENDS);
						}
						RES prev = animation->audio_track_get_key_stream(track, key);
						undo_redo->add_do_method(animation.ptr(), "audio_track_set_key_stream", track, key, stream);
						undo_redo->add_undo_method(animation.ptr(), "audio_track_set_key_stream", track, key, prev);
						update_obj = true;
					}
					else if (name == "start_offset") {
						float value = p_value;

						if (!setting) {
							setting = true;
							undo_redo->create_action(TTR("Anim Multi Change Keyframe Value"), UndoRedo::MERGE_ENDS);
						}
						float prev = animation->audio_track_get_key_start_offset(track, key);
						undo_redo->add_do_method(animation.ptr(), "audio_track_set_key_start_offset", track, key, value);
						undo_redo->add_undo_method(animation.ptr(), "audio_track_set_key_start_offset", track, key, prev);
						update_obj = true;
					}
					else if (name == "end_offset") {
						float value = p_value;

						if (!setting) {
							setting = true;
							undo_redo->create_action(TTR("Anim Multi Change Keyframe Value"), UndoRedo::MERGE_ENDS);
						}
						float prev = animation->audio_track_get_key_end_offset(track, key);
						undo_redo->add_do_method(animation.ptr(), "audio_track_set_key_end_offset", track, key, value);
						undo_redo->add_undo_method(animation.ptr(), "audio_track_set_key_end_offset", track, key, prev);
						update_obj = true;
					}
				} break;
				case Animation::TYPE_ANIMATION: {
					if (name == "animation") {
						StringName anim_name = p_value;

						if (!setting) {
							setting = true;
							undo_redo->create_action(TTR("Anim Multi Change Keyframe Value"), UndoRedo::MERGE_ENDS);
						}
						StringName prev = animation->animation_track_get_key_animation(track, key);
						undo_redo->add_do_method(animation.ptr(), "animation_track_set_key_animation", track, key, anim_name);
						undo_redo->add_undo_method(animation.ptr(), "animation_track_set_key_animation", track, key, prev);
						update_obj = true;
					}
				} break;
				}
			}
		}

		if (setting) {
			if (update_obj) {
				undo_redo->add_do_method(this, "_update_obj", animation);
				undo_redo->add_undo_method(this, "_update_obj", animation);
			}

			undo_redo->commit_action();
			setting = false;

			if (change_notify_deserved) {
				notify_change();
			}

			return true;
		}

		return false;
	}

	bool _get(const StringName& p_name, Variant& r_ret) const {
		for (Map<int, List<float>>::Element* E = key_ofs_map.front(); E; E = E->next()) {
			int track = E->key();
			for (List<float>::Element* key_ofs = E->value().front(); key_ofs; key_ofs = key_ofs->next()) {
				int key = animation->track_find_key(track, key_ofs->get(), true);
				ERR_CONTINUE(key == -1);

				String name = p_name;
				if (name == "time") {
					r_ret = key_ofs;
					return true;
				}

				if (name == "frame") {
					float fps = animation->get_step();
					if (fps > 0) {
						fps = 1.0 / fps;
					}
					r_ret = key_ofs->get() * fps;
					return true;
				}

				if (name == "easing") {
					r_ret = animation->track_get_key_transition(track, key);
					return true;
				}

				switch (animation->track_get_type(track)) {
				case Animation::TYPE_TRANSFORM: {
					Dictionary d = animation->track_get_key_value(track, key);
					ERR_FAIL_COND_V(!d.has(name), false);
					r_ret = d[p_name];
					return true;

				} break;

				case Animation::TYPE_VALUE: {
					if (name == "value") {
						r_ret = animation->track_get_key_value(track, key);
						return true;
					}

				} break;
				case Animation::TYPE_METHOD: {
					Dictionary d = animation->track_get_key_value(track, key);

					if (name == "name") {
						ERR_FAIL_COND_V(!d.has("method"), false);
						r_ret = d["method"];
						return true;
					}

					ERR_FAIL_COND_V(!d.has("args"), false);

					Vector<Variant> args = d["args"];

					if (name == "arg_count") {
						r_ret = args.size();
						return true;
					}

					if (name.begins_with("args/")) {
						int idx = name.get_slice("/", 1).to_int();
						ERR_FAIL_INDEX_V(idx, args.size(), false);

						String what = name.get_slice("/", 2);
						if (what == "type") {
							r_ret = args[idx].get_type();
							return true;
						}

						if (what == "value") {
							r_ret = args[idx];
							return true;
						}
					}

				} break;
				case Animation::TYPE_BEZIER: {
					if (name == "value") {
						r_ret = animation->bezier_track_get_key_value(track, key);
						return true;
					}

					if (name == "in_handle") {
						r_ret = animation->bezier_track_get_key_in_handle(track, key);
						return true;
					}

					if (name == "out_handle") {
						r_ret = animation->bezier_track_get_key_out_handle(track, key);
						return true;
					}


				} break;
				case Animation::TYPE_AUDIO: {
					if (name == "stream") {
						r_ret = animation->audio_track_get_key_stream(track, key);
						return true;
					}

					if (name == "start_offset") {
						r_ret = animation->audio_track_get_key_start_offset(track, key);
						return true;
					}

					if (name == "end_offset") {
						r_ret = animation->audio_track_get_key_end_offset(track, key);
						return true;
					}

				} break;
				case Animation::TYPE_ANIMATION: {
					if (name == "animation") {
						r_ret = animation->animation_track_get_key_animation(track, key);
						return true;
					}

				} break;
				}
			}
		}

		return false;
	}
	void _get_property_list(List<PropertyInfo>* p_list) const {
		if (animation.is_null()) {
			return;
		}

		int first_track = -1;
		float first_key = -1.0;

		bool show_time = true;
		bool same_track_type = true;
		bool same_key_type = true;
		for (Map<int, List<float>>::Element* E = key_ofs_map.front(); E; E = E->next()) {
			int track = E->key();
			ERR_FAIL_INDEX(track, animation->get_track_count());

			if (first_track < 0) {
				first_track = track;
			}

			if (show_time && E->value().size() > 1) {
				show_time = false;
			}

			if (same_track_type) {
				if (animation->track_get_type(first_track) != animation->track_get_type(track)) {
					same_track_type = false;
					same_key_type = false;
				}

				for (List<float>::Element* F = E->value().front(); F; F = F->next()) {
					int key = animation->track_find_key(track, F->get(), true);
					ERR_FAIL_COND(key == -1);
					if (first_key < 0) {
						first_key = key;
					}

					if (animation->track_get_key_value(first_track, first_key).get_type() != animation->track_get_key_value(track, key).get_type()) {
						same_key_type = false;
					}
				}
			}
		}

		if (show_time) {
			if (use_fps && animation->get_step() > 0) {
				float max_frame = animation->get_length() / animation->get_step();
				p_list->push_back(PropertyInfo(Variant::REAL, "frame", PROPERTY_HINT_RANGE, "0," + rtos(max_frame) + ",1"));
			}
			else {
				p_list->push_back(PropertyInfo(Variant::REAL, "time", PROPERTY_HINT_RANGE, "0," + rtos(animation->get_length()) + ",0.01"));
			}
		}

		if (same_track_type) {
			switch (animation->track_get_type(first_track)) {
			case Animation::TYPE_TRANSFORM: {
				p_list->push_back(PropertyInfo(Variant::VECTOR3, "location"));
				p_list->push_back(PropertyInfo(Variant::QUAT, "rotation"));
				p_list->push_back(PropertyInfo(Variant::VECTOR3, "scale"));
			} break;

			case Animation::TYPE_VALUE: {
				if (same_key_type) {
					Variant v = animation->track_get_key_value(first_track, first_key);

					if (hint.type != Variant::NIL) {
						PropertyInfo pi = hint;
						pi.name = "value";
						p_list->push_back(pi);
					}
					else {
						PropertyHint hint = PROPERTY_HINT_NONE;
						String hint_string;

						if (v.get_type() == Variant::OBJECT) {
							// Could actually check the object property if exists..? Yes I will!
							Ref<Resource> res = v;
							if (res.is_valid()) {
								hint = PROPERTY_HINT_RESOURCE_TYPE;
								hint_string = res->get_class();
							}
						}

						if (v.get_type() != Variant::NIL) {
							p_list->push_back(PropertyInfo(v.get_type(), "value", hint, hint_string));
						}
					}
				}

				p_list->push_back(PropertyInfo(Variant::REAL, "easing", PROPERTY_HINT_EXP_EASING));
			} break;
			case Animation::TYPE_METHOD: {
				p_list->push_back(PropertyInfo(Variant::STRING, "name"));

				p_list->push_back(PropertyInfo(Variant::INT, "arg_count", PROPERTY_HINT_RANGE, "0,32,1,or_greater"));

				Dictionary d = animation->track_get_key_value(first_track, first_key);
				ERR_FAIL_COND(!d.has("args"));
				Vector<Variant> args = d["args"];
				String vtypes;
				for (int i = 0; i < Variant::VARIANT_MAX; i++) {
					if (i > 0) {
						vtypes += ",";
					}
					vtypes += Variant::get_type_name(Variant::Type(i));
				}

				for (int i = 0; i < args.size(); i++) {
					p_list->push_back(PropertyInfo(Variant::INT, "args/" + itos(i) + "/type", PROPERTY_HINT_ENUM, vtypes));
					if (args[i].get_type() != Variant::NIL) {
						p_list->push_back(PropertyInfo(args[i].get_type(), "args/" + itos(i) + "/value"));
					}
				}
			} break;
			case Animation::TYPE_BEZIER: {
				p_list->push_back(PropertyInfo(Variant::REAL, "value"));
				p_list->push_back(PropertyInfo(Variant::VECTOR2, "in_handle"));
				p_list->push_back(PropertyInfo(Variant::VECTOR2, "out_handle"));
				p_list->push_back(PropertyInfo(Variant::INT, "handle_mode", PROPERTY_HINT_ENUM, "Free,Balanced"));
			} break;
			case Animation::TYPE_AUDIO: {
				p_list->push_back(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"));
				p_list->push_back(PropertyInfo(Variant::REAL, "start_offset", PROPERTY_HINT_RANGE, "0,3600,0.01,or_greater"));
				p_list->push_back(PropertyInfo(Variant::REAL, "end_offset", PROPERTY_HINT_RANGE, "0,3600,0.01,or_greater"));
			} break;
			case Animation::TYPE_ANIMATION: {
				if (key_ofs_map.size() > 1) {
					break;
				}

				String animations;

				if (root_path && root_path->has_node(animation->track_get_path(first_track))) {
					AnimationPlayer* ap = Object::cast_to<AnimationPlayer>(root_path->get_node(animation->track_get_path(first_track)));
					if (ap) {
						List<StringName> anims;
						ap->get_animation_list(&anims);
						for (List<StringName>::Element* G = anims.front(); G; G = G->next()) {
							if (!animations.empty()) {
								animations += ",";
							}

							animations += String(G->get());
						}
					}
				}

				if (!animations.empty()) {
					animations += ",";
				}
				animations += "[stop]";

				p_list->push_back(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_ENUM, animations));
			} break;
			}
		}
	}

	Ref<Animation> animation;

	Map<int, List<float>> key_ofs_map;
	Map<int, NodePath> base_map;
	PropertyInfo hint;

	Node* root_path = nullptr;

	bool use_fps = false;

	UndoRedo* undo_redo = nullptr;

	void notify_change() {
		property_list_changed_notify();
	}

	Node* get_root_path() {
		return root_path;
	}

	void set_use_fps(bool p_enable) {
		use_fps = p_enable;
		property_list_changed_notify();
	}
};

#endif