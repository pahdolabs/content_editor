#ifndef TRACK_EDIT_TYPE_SPRITE_FRAME_H
#define TRACK_EDIT_TYPE_SPRITE_FRAME_H

#include "track_edit.h"

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

#endif
