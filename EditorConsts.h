#ifndef CONSTS_H
#define CONSTS_H

#include "core/color.h"
#include "core/reference.h"

class EditorConsts : public Object {
	GDCLASS(EditorConsts, Object);

	Map<String, Color> colors;
	Map<String, float> consts;

	static EditorConsts *singleton;

protected:
	static void _bind_methods();

public:
	static EditorConsts* get_singleton();

	static const float CONTRAST;
	static const Color BASE_COLOR;
	static const Color DARK_COLOR_2;
	static const Color ACCENT_COLOR;
	static const Color BOX_SELECTION_FILL_COLOR;
	static const Color BOX_SELECTION_STROKE_COLOR;
	static const Color TAG_TIMELINE_COLOR;
	static const Color TAG_HEADER_COLOR;

	void submit_color(const String& p_name, const Color& p_color);
	void submit_const(const String& p_name, float p_const);
	bool has_color(const String& p_name) const;
	bool has_const(const String& p_name) const;
	Color named_color(const String &p_name, const Color &p_default = Color(1,1,1));
	float named_const(const String& p_name, float p_default = 0);

	EditorConsts();
};

#endif
