#include "consts.h"

constexpr float Colors::CONTRAST = 0.25f;
const Color Colors::BASE_COLOR = Color(0.2, 0.23, 0.31);
const Color Colors::DARK_COLOR_2 = BASE_COLOR.linear_interpolate(Color(0, 0, 0), CONTRAST * 1.5f);
const Color Colors::ACCENT_COLOR = Color(0.41, 0.61, 0.91);
const Color Colors::BOX_SELECTION_FILL_COLOR(ACCENT_COLOR * Color(1, 1, 1, 0.3));
const Color Colors::BOX_SELECTION_STROKE_COLOR(ACCENT_COLOR * Color(1, 1, 1, 0.8));
const Color Colors::TAG_TIMELINE_COLOR(Color(0.7, 0.7, 0.7));
const Color Colors::TAG_HEADER_COLOR(Color(0.4, 0.57, 0.4));

Colors* Colors::singleton;

Colors* Colors::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(Colors);
	}
	return singleton;
}

Color Colors::named(const String p_name) {
	if(colors.has(p_name)) {
		return colors[p_name];
	}
	return Color::named("white");
}

float Colors::contrast() {
	return CONTRAST;
}

void Colors::_bind_methods() {
	ClassDB::bind_method(D_METHOD("named", "name"), &Colors::named);
	ClassDB::bind_method("contrast", &Colors::contrast);
}

Colors::Colors() {
	colors.insert("BASE_COLOR", BASE_COLOR);
	colors.insert("DARK_COLOR_2", DARK_COLOR_2);
	colors.insert("ACCENT_COLOR", ACCENT_COLOR);
	colors.insert("BOX_SELECTION_FILL_COLOR", BOX_SELECTION_FILL_COLOR);
	colors.insert("BOX_SELECTION_STROKE_COLOR", BOX_SELECTION_STROKE_COLOR);
	colors.insert("TAG_TIMELINE_COLOR", TAG_TIMELINE_COLOR);
	colors.insert("TAG_HEADER_COLOR", TAG_HEADER_COLOR);
}
