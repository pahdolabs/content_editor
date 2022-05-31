#ifndef TRACK_EDIT_DEFAULT_PLUGIN_H
#define TRACK_EDIT_DEFAULT_PLUGIN_H

#include "track_edit_plugin.h"

class TrackEdit;

class TrackEditDefaultPlugin : public TrackEditPlugin {
	GDCLASS(TrackEditDefaultPlugin, TrackEditPlugin);

public:
	virtual TrackEdit* create_value_track_edit(Object* p_object, Variant::Type p_type, const String& p_property, PropertyHint p_hint, const String& p_hint_string, int p_usage) override;
	virtual TrackEdit* create_audio_track_edit() override;
	virtual TrackEdit* create_animation_track_edit(Object* p_object) override;
	virtual TrackEdit* create_tag_track_edit() override;
};

#endif
