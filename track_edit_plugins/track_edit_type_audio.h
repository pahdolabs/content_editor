#ifndef TRACK_EDIT_TYPE_AUDIO_H
#define TRACK_EDIT_TYPE_AUDIO_H

#include "track_edit.h"

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

#endif
