#ifndef TRACK_EDIT_COLOR_H
#define TRACK_EDIT_COLOR_H

#include "track_edit.h"

class TrackEditColor : public TrackEdit {
	GDCLASS(TrackEditColor, TrackEdit);

public:
	virtual int get_key_height() const override;
	virtual Rect2 get_key_rect(int p_index, float p_pixels_sec) override;
	virtual bool is_key_selectable_by_distance() const override;
	virtual void draw_key(int p_index, float p_pixels_sec, int p_x, bool p_selected, int p_clip_left, int p_clip_right) override;
	virtual void draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) override;
};

#endif
