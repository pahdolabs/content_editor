#ifndef TRACK_EDIT_PLUGIN_H
#define TRACK_EDIT_PLUGIN_H

#include <core/reference.h>

class TrackEdit;

class TrackEditPlugin : public Reference {
	GDCLASS(TrackEditPlugin, Reference);

public:
	virtual TrackEdit* create_value_track_edit(Object* p_object, Variant::Type p_type, const String& p_property, PropertyHint p_hint, const String& p_hint_string, int p_usage);
	virtual TrackEdit* create_audio_track_edit();
	virtual TrackEdit* create_animation_track_edit(Object* p_object);
};

#endif
