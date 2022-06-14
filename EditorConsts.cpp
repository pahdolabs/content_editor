#include "EditorConsts.h"

constexpr float EditorConsts::CONTRAST = 0.25f;
const Color EditorConsts::BASE_COLOR = Color(0.2, 0.23, 0.31);
const Color EditorConsts::DARK_COLOR_2 = BASE_COLOR.linear_interpolate(Color(0, 0, 0), CONTRAST * 1.5f);
const Color EditorConsts::ACCENT_COLOR = Color(0.41, 0.61, 0.91);
const Color EditorConsts::BOX_SELECTION_FILL_COLOR(ACCENT_COLOR* Color(1, 1, 1, 0.3));
const Color EditorConsts::BOX_SELECTION_STROKE_COLOR(ACCENT_COLOR* Color(1, 1, 1, 0.8));
const Color EditorConsts::TAG_TIMELINE_COLOR(Color(0.7, 0.7, 0.7));
const Color EditorConsts::TAG_HEADER_COLOR(Color(0.4, 0.57, 0.4));

EditorConsts* EditorConsts::singleton;

EditorConsts* EditorConsts::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(EditorConsts);
	}
	return singleton;
}

float EditorConsts::named_const(const String& p_name, float p_default) {
	if (consts.has(p_name)) {
		return consts[p_name];
	}
	consts.insert(p_name, p_default);
	return p_default;
}

Color EditorConsts::named_color(const String& p_name, const Color& p_default) {
	if (colors.has(p_name)) {
		return colors[p_name];
	}
	colors.insert(p_name, p_default);
	return p_default;
}

void EditorConsts::submit_color(const String& p_name, const Color& p_color) {
	colors.insert(p_name, p_color);
}

void EditorConsts::submit_const(const String& p_name, float p_const) {
	consts.insert(p_name, p_const);
}

bool EditorConsts::has_color(const String& p_name) const {
	return colors.has(p_name);
}

bool EditorConsts::has_const(const String& p_name) const {
	return consts.has(p_name);
}

void EditorConsts::_bind_methods() {
	ClassDB::bind_method(D_METHOD("named_color", "name", "default"), &EditorConsts::named_color, DEFVAL(Color(1, 1, 1)));
	ClassDB::bind_method(D_METHOD("named_const", "name", "default"), &EditorConsts::named_const, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("submit_color", "name", "color"), &EditorConsts::submit_color);
	ClassDB::bind_method(D_METHOD("submit_const", "name", "const"), &EditorConsts::submit_const);
	ClassDB::bind_method(D_METHOD("has_color", "name"), &EditorConsts::has_color);
	ClassDB::bind_method(D_METHOD("has_const", "name"), &EditorConsts::has_const);
}

EditorConsts::EditorConsts() {
	consts.insert("CONTRAST", CONTRAST);
	colors.insert("BASE_COLOR", BASE_COLOR);
	colors.insert("DARK_COLOR_2", DARK_COLOR_2);
	colors.insert("ACCENT_COLOR", ACCENT_COLOR);
	colors.insert("BOX_SELECTION_FILL_COLOR", BOX_SELECTION_FILL_COLOR);
	colors.insert("BOX_SELECTION_STROKE_COLOR", BOX_SELECTION_STROKE_COLOR);
	colors.insert("TAG_TIMELINE_COLOR", TAG_TIMELINE_COLOR);
	colors.insert("TAG_HEADER_COLOR", TAG_HEADER_COLOR);
}
