#include "editor_consts.h"

constexpr float _EditorConsts::CONTRAST = 0.25f;
const Color _EditorConsts::BASE_COLOR = Color(0.2, 0.23, 0.31);
const Color _EditorConsts::DARK_COLOR_2 = BASE_COLOR.linear_interpolate(Color(0, 0, 0), CONTRAST * 1.5f);
const Color _EditorConsts::ACCENT_COLOR = Color(0.41, 0.61, 0.91);
const Color _EditorConsts::BOX_SELECTION_FILL_COLOR(ACCENT_COLOR* Color(1, 1, 1, 0.3));
const Color _EditorConsts::BOX_SELECTION_STROKE_COLOR(ACCENT_COLOR* Color(1, 1, 1, 0.8));
const Color _EditorConsts::TAG_TIMELINE_COLOR(Color(0.7, 0.7, 0.7));
const Color _EditorConsts::TAG_HEADER_COLOR(Color(0.4, 0.57, 0.4));

_EditorConsts* _EditorConsts::singleton;

_EditorConsts* _EditorConsts::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(_EditorConsts);
	}
	return singleton;
}

float _EditorConsts::named_const(const String& p_name, float p_default) {
	if (consts.has(p_name)) {
		return consts[p_name];
	}
	consts.insert(p_name, p_default);
	return p_default;
}

Color _EditorConsts::named_color(const String& p_name, const Color& p_default) {
	if (colors.has(p_name)) {
		return colors[p_name];
	}
	colors.insert(p_name, p_default);
	return p_default;
}

void _EditorConsts::submit_color(const String& p_name, const Color& p_color) {
	colors.insert(p_name, p_color);
}

void _EditorConsts::submit_const(const String& p_name, float p_const) {
	consts.insert(p_name, p_const);
}

bool _EditorConsts::has_color(const String& p_name) const {
	return colors.has(p_name);
}

bool _EditorConsts::has_const(const String& p_name) const {
	return consts.has(p_name);
}

void _EditorConsts::_bind_methods() {
	ClassDB::bind_method(D_METHOD("named_color", "name", "default"), &_EditorConsts::named_color, DEFVAL(Color(1, 1, 1)));
	ClassDB::bind_method(D_METHOD("named_const", "name", "default"), &_EditorConsts::named_const, DEFVAL(0.0));
	ClassDB::bind_method(D_METHOD("submit_color", "name", "color"), &_EditorConsts::submit_color);
	ClassDB::bind_method(D_METHOD("submit_const", "name", "const"), &_EditorConsts::submit_const);
	ClassDB::bind_method(D_METHOD("has_color", "name"), &_EditorConsts::has_color);
	ClassDB::bind_method(D_METHOD("has_const", "name"), &_EditorConsts::has_const);
}

_EditorConsts::_EditorConsts() {
	consts.insert("CONTRAST", CONTRAST);
	colors.insert("BASE_COLOR", BASE_COLOR);
	colors.insert("DARK_COLOR_2", DARK_COLOR_2);
	colors.insert("ACCENT_COLOR", ACCENT_COLOR);
	colors.insert("BOX_SELECTION_FILL_COLOR", BOX_SELECTION_FILL_COLOR);
	colors.insert("BOX_SELECTION_STROKE_COLOR", BOX_SELECTION_STROKE_COLOR);
	colors.insert("TAG_TIMELINE_COLOR", TAG_TIMELINE_COLOR);
	colors.insert("TAG_HEADER_COLOR", TAG_HEADER_COLOR);
}
