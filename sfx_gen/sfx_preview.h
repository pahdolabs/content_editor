#ifndef SFX_PREVIEW_H
#define SFX_PREVIEW_H

#include "core/reference.h"

class SFXPreview : public Reference {
	GDCLASS(SFXPreview, Reference);

	friend class _SFXPreviewGenerator;
	Vector<uint8_t> preview;
	float length;

protected:
	static void _bind_methods();

public:
	float get_length() const;
	float get_max(float p_time, float p_time_next) const;
	float get_min(float p_time, float p_time_next) const;

	SFXPreview();
};

#endif
