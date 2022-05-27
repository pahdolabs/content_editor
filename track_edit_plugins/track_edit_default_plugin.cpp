#include "track_edit_default_plugin.h"

#include "core/undo_redo.h"
#include "servers/audio/audio_stream.h"

#include "track_edit.h"
#include "track_edit_audio.h"
#include "track_edit_bool.h"
#include "track_edit_color.h"
#include "track_edit_type_animation.h"
#include "track_edit_type_audio.h"
#include "track_edit_type_sprite_frame.h"
#include "track_edit_type_sub_anim.h"
#include "track_edit_volume_db.h"

TrackEdit* TrackEditDefaultPlugin::create_value_track_edit(Object* p_object, Variant::Type p_type, const String& p_property, PropertyHint p_hint, const String& p_hint_string, int p_usage) {
	if (p_property == "playing" && (p_object->is_class("AudioStreamPlayer") || p_object->is_class("AudioStreamPlayer2D") || p_object->is_class("AudioStreamPlayer3D"))) {
		TrackEditAudio* audio = memnew(TrackEditAudio);
		audio->set_node(p_object);
		return audio;
	}

	if (p_property == "frame" && (p_object->is_class("Sprite2D") || p_object->is_class("Sprite3D") || p_object->is_class("AnimatedSprite2D") || p_object->is_class("AnimatedSprite3D"))) {
		TrackEditSpriteFrame* sprite = memnew(TrackEditSpriteFrame);
		sprite->set_node(p_object);
		return sprite;
	}

	if (p_property == "frame_coords" && (p_object->is_class("Sprite2D") || p_object->is_class("Sprite3D"))) {
		TrackEditSpriteFrame* sprite = memnew(TrackEditSpriteFrame);
		sprite->set_as_coords();
		sprite->set_node(p_object);
		return sprite;
	}

	if (p_property == "current_animation" && (p_object->is_class("AnimationPlayer"))) {
		TrackEditSubAnim* player = memnew(TrackEditSubAnim);
		player->set_node(p_object);
		return player;
	}

	if (p_property == "volume_db") {
		TrackEditVolumeDB* vu = memnew(TrackEditVolumeDB);
		return vu;
	}

	if (p_type == Variant::BOOL) {
		return memnew(TrackEditBool);
	}
	if (p_type == Variant::COLOR) {
		return memnew(TrackEditColor);
	}

	return nullptr;
}

TrackEdit* TrackEditDefaultPlugin::create_audio_track_edit() {
	return memnew(TrackEditTypeAudio);
}

TrackEdit* TrackEditDefaultPlugin::create_animation_track_edit(Object* p_object) {
	TrackEditTypeAnimation* an = memnew(TrackEditTypeAnimation);
	an->set_node(p_object);
	return an;
}
