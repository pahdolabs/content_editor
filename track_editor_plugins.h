#ifndef TRACK_EDITOR_PLUGINS_H
#define TRACK_EDITOR_PLUGINS_H

#include "track_edit.h"
#include "track_edit_plugin.h"

class TrackEditBool : public TrackEdit {
	GDCLASS(TrackEditBool, TrackEdit);
	Ref<Texture> icon_checked;
	Ref<Texture> icon_unchecked;

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;
};

class TrackEditColor : public TrackEdit {
	GDCLASS(TrackEditColor, TrackEdit);

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;
	virtual void draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) override;
};

class TrackEditAudio : public TrackEdit {
	GDCLASS(TrackEditAudio, TrackEdit);

	ObjectID id;

	void _preview_changed(ObjectID p_which);

protected:
	static void _bind_methods();

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;

	void set_node(Object* p_object);

	TrackEditAudio();
};

class TrackEditSpriteFrame : public TrackEdit {
	GDCLASS(TrackEditSpriteFrame, TrackEdit);

	ObjectID id;
	bool is_coords = false;

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;

	void set_node(Object* p_object);
	void set_as_coords();

	TrackEditSpriteFrame() {}
};

class TrackEditSubAnim : public TrackEdit {
	GDCLASS(TrackEditSubAnim, TrackEdit);

	ObjectID id;

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;

	void set_node(Object* p_object);
};

class TrackEditTypeAudio : public TrackEdit {
	GDCLASS(TrackEditTypeAudio, TrackEdit);

	void _preview_changed(ObjectID p_which);

	bool len_resizing = false;
	bool len_resizing_start;
	int len_resizing_index;
	float len_resizing_from_px;
	float len_resizing_rel;
	bool over_drag_position = false;

protected:
	static void _bind_methods();

public:
	void _gui_input(const Ref<InputEvent>& p_event);

	virtual bool can_drop_data(const Point2& p_point, const Variant& p_data) const override;
	virtual void drop_data(const Point2& p_point, const Variant& p_data) override;

	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;

	virtual CursorShape get_cursor_shape(const Point2& p_pos) const override;

	TrackEditTypeAudio();
};

class TrackEditTypeAnimation : public TrackEdit {
	GDCLASS(TrackEditTypeAnimation, TrackEdit);

	ObjectID id;

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;

	void set_node(Object* p_object);
	TrackEditTypeAnimation();
};

class TrackEditVolumeDB : public TrackEdit {
	GDCLASS(TrackEditVolumeDB, TrackEdit);

public:
	virtual void draw_bg(int p_clip_left, int p_clip_right) override;
	virtual void draw_fg(int p_clip_left, int p_clip_right) override;
	virtual int get_key_height() const override;
	virtual void draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) override;
};

class TrackEditDefaultPlugin : public TrackEditPlugin {
	GDCLASS(TrackEditDefaultPlugin, TrackEditPlugin);

public:
	virtual TrackEdit* create_value_track_edit(Object* p_object, Variant::Type p_type, const String& p_property, PropertyHint p_hint, const String& p_hint_string, int p_usage) override;
	virtual TrackEdit* create_audio_track_edit() override;
	virtual TrackEdit* create_animation_track_edit(Object* p_object) override;
};

#endif