#ifndef VIEW_PANNER_H
#define VIEW_PANNER_H

#include "core/reference.h"

class InputEventMouseMotion;
class InputEvent;
class ShortCut;

class ViewPanner : public Reference {
	GDCLASS(ViewPanner, Reference);

public:
	enum ControlScheme {
		SCROLL_ZOOMS,
		SCROLL_PANS,
	};

private:
	bool is_dragging = false;
	bool pan_key_pressed = false;
	bool force_drag = false;

	bool enable_rmb = false;
	bool simple_panning_enabled = false;

	Ref<ShortCut> pan_view_shortcut;

	Object *scroll_callback_object;
	String scroll_callback_function;
	Object *pan_callback_object;
	String pan_callback_function;
	Object *zoom_callback_object;
	String zoom_callback_function;
	
	ControlScheme control_scheme = SCROLL_PANS;

	Vector2 warp_mouse_motion(Ref<InputEventMouseMotion> p_event, Rect2 p_canvas_rect);

protected:
	static void _bind_methods();

public:
	void set_callbacks(Object* p_scroll_callback_object, const String &p_scroll_callback_function, Object* p_pan_callback_object, const String &p_pan_callback_function, Object* p_zoom_callback_object, const String &p_zoom_callback_function);
	void set_control_scheme(ControlScheme p_scheme);
	void set_enable_rmb(bool p_enable);
	void set_pan_shortcut(Ref<ShortCut> p_shortcut);
	void set_simple_panning_enabled(bool p_enabled);

	void setup(ControlScheme p_scheme, Ref<ShortCut> p_shortcut, bool p_simple_panning);

	bool is_panning() const;
	void set_force_drag(bool p_force);

	bool gui_input(const Ref<InputEvent>& p_ev, Rect2 p_canvas_rect = Rect2());
	void release_pan_key();

	ViewPanner();
};

#endif