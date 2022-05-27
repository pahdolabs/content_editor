#ifndef TRACK_EDIT_VOLUME_DB_H
#define TRACK_EDIT_VOLUME_DB_H

#include "track_edit.h"

class TrackEditVolumeDB : public TrackEdit {
	GDCLASS(TrackEditVolumeDB, TrackEdit);

public:
	virtual void draw_bg(int p_clip_left, int p_clip_right) override;
	virtual void draw_fg(int p_clip_left, int p_clip_right) override;
	virtual int get_key_height() const override;
	virtual void draw_key_link(int p_index, float p_pixels_sec, int p_x, int p_next_x, int p_clip_left, int p_clip_right) override;
};

#endif
