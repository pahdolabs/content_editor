#include "consts.h"

constexpr float Colors::CONTRAST = 0.25f;
const Color Colors::BASE_COLOR = Color(0.2, 0.23, 0.31);
const Color Colors::DARK_COLOR_2 = BASE_COLOR.linear_interpolate(Color(0, 0, 0), CONTRAST * 1.5f);
const Color Colors::ACCENT_COLOR = Color(0.41, 0.61, 0.91);
const Color Colors::BOX_SELECTION_FILL_COLOR(ACCENT_COLOR * Color(1, 1, 1, 0.3));
const Color Colors::BOX_SELECTION_STROKE_COLOR(ACCENT_COLOR * Color(1, 1, 1, 0.8));
