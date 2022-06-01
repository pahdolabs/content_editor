#ifndef CONSTS_H
#define CONSTS_H

#include "core/color.h"
#include "core/reference.h"

class Colors : public Object {
	GDCLASS(Colors, Object);

	Map<String, Color> colors;

	static Colors *singleton;

protected:
	static void _bind_methods();

public:
	static Colors* get_singleton();

	static const float CONTRAST;
	static const Color BASE_COLOR;
	static const Color DARK_COLOR_2;
	static const Color ACCENT_COLOR;
	static const Color BOX_SELECTION_FILL_COLOR;
	static const Color BOX_SELECTION_STROKE_COLOR;
	static const Color TAG_TIMELINE_COLOR;
	static const Color TAG_HEADER_COLOR;

	Color named(String p_name);
	float contrast();

	Colors();
};

#endif
