#include "track_edit_default_plugin.h"

#include "../track_editor/track_editor.h"
#include "../track_editor/timeline_edit.h"
#include <core/undo_redo.h>
#include <core/os/input_event.h>
#include <servers/audio/audio_stream.h>
#include <scene/2d/sprite.h>
#include <scene/2d/animated_sprite.h>
#include <scene/3d/sprite_3d.h>
#include <scene/animation/animation_player.h>

#include "../consts.h"
#include "../icons_cache.h"

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
