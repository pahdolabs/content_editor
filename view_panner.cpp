#include "view_panner.h"

#include <core/os/input_event.h>

#include "core/os/input.h"
#include "scene/gui/shortcut.h"
#include "core/os/keyboard.h"

bool ViewPanner::gui_input(const Ref<InputEvent>& p_event, Rect2 p_canvas_rect) {
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		Vector2 scroll_vec = Vector2((mb->get_button_index() == BUTTON_WHEEL_RIGHT) - (mb->get_button_index() == BUTTON_WHEEL_LEFT), (mb->get_button_index() == BUTTON_WHEEL_DOWN) - (mb->get_button_index() == BUTTON_WHEEL_UP));
		if (scroll_vec != Vector2()) {
			if (control_scheme == SCROLL_PANS) {
				if (mb->get_control()) {
					scroll_vec.y *= mb->get_factor();
					callback_helper(zoom_callback_object, zoom_callback_function, varray(scroll_vec, mb->get_position(), mb->get_alt()));
					return true;
				}
				else {
					Vector2 panning;
					if (mb->get_shift()) {
						panning.x += mb->get_factor() * scroll_vec.y;
						panning.y += mb->get_factor() * scroll_vec.x;
					}
					else {
						panning.y += mb->get_factor() * scroll_vec.y;
						panning.x += mb->get_factor() * scroll_vec.x;
					}
					callback_helper(scroll_callback_object, scroll_callback_function, varray(panning, mb->get_alt()));
					return true;
				}
			}
			else {
				if (mb->get_control()) {
					Vector2 panning;
					if (mb->get_shift()) {
						panning.x += mb->get_factor() * scroll_vec.y;
						panning.y += mb->get_factor() * scroll_vec.x;
					}
					else {
						panning.y += mb->get_factor() * scroll_vec.y;
						panning.x += mb->get_factor() * scroll_vec.x;
					}
					callback_helper(scroll_callback_object, scroll_callback_function, varray(panning, mb->get_alt()));
					return true;
				}
				else if (!mb->get_shift()) {
					scroll_vec.y *= mb->get_factor();
					callback_helper(zoom_callback_object, zoom_callback_function, varray(scroll_vec, mb->get_position(), mb->get_alt()));
					return true;
				}
			}
		}

		// Alt is not used for button presses, so ignore it.
		if (mb->get_alt()) {
			return false;
		}

		bool is_drag_event = mb->get_button_index() == BUTTON_MIDDLE ||
			(enable_rmb && mb->get_button_index() == BUTTON_RIGHT) ||
			(!simple_panning_enabled && mb->get_button_index() == BUTTON_LEFT && is_panning()) ||
			(force_drag && mb->get_button_index() == BUTTON_LEFT);

		if (is_drag_event) {
			if (mb->is_pressed()) {
				is_dragging = true;
			}
			else {
				is_dragging = false;
			}
			return mb->get_button_index() != BUTTON_LEFT || mb->is_pressed(); // Don't consume LMB release events (it fixes some selection problems).
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		if (is_dragging) {
			if (p_canvas_rect != Rect2()) {
				callback_helper(pan_callback_object, pan_callback_function, varray(warp_mouse_motion(mm, p_canvas_rect)));
			}
			else {
				callback_helper(pan_callback_object, pan_callback_function, varray(mm->get_relative()));
			}
			return true;
		}
	}

	Ref<InputEventKey> k = p_event;
	if (k.is_valid()) {
		if (pan_view_shortcut.is_valid() && pan_view_shortcut->is_shortcut(k)) {
			pan_key_pressed = k->is_pressed();
			if (simple_panning_enabled || (Input::get_singleton()->get_mouse_button_mask() & BUTTON_LEFT) != 0) {
				is_dragging = pan_key_pressed;
			}
			return true;
		}
	}

	return false;
}

Vector2 ViewPanner::warp_mouse_motion(Ref<InputEventMouseMotion> p_event, Rect2 p_canvas_rect) {
	Vector2 rel_sign = Vector2(p_event->get_relative().x >= 0.0 ? 1 : -1, p_event->get_relative().y >= 0.0 ? 1 : -1);
	Vector2 warp_margin = p_canvas_rect.size * 0.5;
	Vector2 rel_warped = Vector2(
		fmod(p_event->get_relative().x + rel_sign.x * warp_margin.x, p_canvas_rect.size.x) - rel_sign.x * warp_margin.x,
		fmod(p_event->get_relative().y + rel_sign.y * warp_margin.y, p_canvas_rect.size.y) - rel_sign.y * warp_margin.y
	);

	Vector2 pos_local = p_event->get_global_position() - p_canvas_rect.position;
	Vector2 pos_warped = Vector2(Math::fposmod(pos_local.x, p_canvas_rect.size.x), Math::fposmod(pos_local.y, p_canvas_rect.size.y));
	if (pos_warped != pos_local) {
		Input::get_singleton()->warp_mouse_position(pos_warped + p_canvas_rect.position);
	}

	return rel_warped;
}

void ViewPanner::release_pan_key() {
	pan_key_pressed = false;
	is_dragging = false;
}

void ViewPanner::callback_helper(Object* p_callback_object, String p_callback_function, Vector<Variant> p_args) {
	const Variant** argptr = (const Variant**)alloca(sizeof(Variant*) * p_args.size());
	for (int i = 0; i < p_args.size(); i++) {
		argptr[i] = &p_args[i];
	}

	Variant::CallError ce;
	p_callback_object->call(p_callback_function, p_args, p_args.size(), &ce);
}

void ViewPanner::set_callbacks(Object* p_scroll_callback_object, String p_scroll_callback_function, Object* p_pan_callback_object, String p_pan_callback_function, Object* p_zoom_callback_object, String p_zoom_callback_function) {
	scroll_callback_object = p_scroll_callback_object;
	scroll_callback_function = p_scroll_callback_function;

	pan_callback_object = p_pan_callback_object;
	pan_callback_function = pan_callback_function;

	zoom_callback_object = p_zoom_callback_object;
	zoom_callback_function = zoom_callback_function;
}

void ViewPanner::set_control_scheme(ControlScheme p_scheme) {
	control_scheme = p_scheme;
}

void ViewPanner::set_enable_rmb(bool p_enable) {
	enable_rmb = p_enable;
}

void ViewPanner::set_pan_shortcut(Ref<ShortCut> p_shortcut) {
	pan_view_shortcut = p_shortcut;
	pan_key_pressed = false;
}

void ViewPanner::set_simple_panning_enabled(bool p_enabled) {
	simple_panning_enabled = p_enabled;
}

void ViewPanner::setup(ControlScheme p_scheme, Ref<ShortCut> p_shortcut, bool p_simple_panning) {
	set_control_scheme(p_scheme);
	set_pan_shortcut(p_shortcut);
	set_simple_panning_enabled(p_simple_panning);
}

bool ViewPanner::is_panning() const {
	return is_dragging || pan_key_pressed;
}

void ViewPanner::set_force_drag(bool p_force) {
	force_drag = p_force;
}

void ViewPanner::_bind_methods() {
	ClassDB::bind_method(D_METHOD("release_pan_key"), &ViewPanner::release_pan_key);
}

ViewPanner::ViewPanner() {
	Ref<InputEventKey> ek;
	ek.instance();
	ek->set_scancode(KEY_SPACE);
	ek->set_pressed(true);

	pan_view_shortcut.instance();
	pan_view_shortcut->set_shortcut(ek);
}
