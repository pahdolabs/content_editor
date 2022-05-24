#include "track_edit_plugin.h"
#include "track_edit.h"

#include <core/script_language.h>

TrackEdit* TrackEditPlugin::create_value_track_edit(Object* p_object, Variant::Type p_type, const String& p_property, PropertyHint p_hint, const String& p_hint_string, int p_usage) {
	if (get_script_instance()) {
		Variant args[6] = {
			p_object,
			p_type,
			p_property,
			p_hint,
			p_hint_string,
			p_usage
		};

		Variant* argptrs[6] = {
			&args[0],
			&args[1],
			&args[2],
			&args[3],
			&args[4],
			&args[5]
		};

		Variant::CallError ce;
		return Object::cast_to<TrackEdit>(get_script_instance()->call("create_value_track_edit", (const Variant**)&argptrs, 6, ce).operator Object * ());
	}
	return nullptr;
}

TrackEdit* TrackEditPlugin::create_audio_track_edit() {
	if (get_script_instance()) {
		return Object::cast_to<TrackEdit>(get_script_instance()->call("create_audio_track_edit").operator Object * ());
	}
	return nullptr;
}

TrackEdit* TrackEditPlugin::create_animation_track_edit(Object* p_object) {
	if (get_script_instance()) {
		return Object::cast_to<TrackEdit>(get_script_instance()->call("create_animation_track_edit", p_object).operator Object * ());
	}
	return nullptr;
}
