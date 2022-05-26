#include "consts.h"

const float Colors::CONTRAST = 0.25f;
const Color Colors::base_color = Color(0.2, 0.23, 0.31);
const Color Colors::dark_color_2 = Colors::base_color.linear_interpolate(Color(0, 0, 0), Colors::CONTRAST * 1.5f);
const Color Colors::accent_color = Color(0.41, 0.61, 0.91);
const Color Colors::box_selection_fill_color(Colors::accent_color* Color(1, 1, 1, 0.3));
const Color Colors::box_selection_stroke_color(Colors::accent_color* Color(1, 1, 1, 0.8));
