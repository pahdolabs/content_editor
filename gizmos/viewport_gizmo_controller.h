#ifndef VIEWPORT_GIZMO_CONTROLLER_H
#define VIEWPORT_GIZMO_CONTROLLER_H
#include "pahdo_spatial_gizmo.h"
#include "core/reference.h"

class ViewportGizmoController : public Reference {
	GDCLASS(ViewportGizmoController, Reference);

	const float MAX_Z = 1000000.0;
	const float GIZMO_CIRCLE_SIZE = 1.1;
	const float GIZMO_ARROW_OFFSET = GIZMO_CIRCLE_SIZE + 0.3;
	const float GIZMO_SCALE_OFFSET = GIZMO_ARROW_OFFSET + 0.3;
	const float GIZMO_ARROW_SIZE = 0.35;
	const float GIZMO_RING_HALF_WIDTH = 0.1;
	const float GIZMO_PLANE_SIZE = 0.2;
	const float GIZMO_PLANE_DST = 0.3;

	enum {
		NONE_MODE = 0,
		WIDGET_MODE = 1,
		MOVE_MODE = 2,
		ROTATE_MODE = 4
	};

	enum TransformType {
		TRANSFORM_NONE,
		TRANSFORM_ROTATE,
		TRANSFORM_TRANSLATE,
	};

	enum TransformPlane {
		TRANSFORM_VIEW,
		TRANSFORM_X_AXIS,
		TRANSFORM_Y_AXIS,
		TRANSFORM_Z_AXIS,
		TRANSFORM_YZ,
		TRANSFORM_XZ,
		TRANSFORM_XY,
	};

	struct Gizmo {
		bool visible;
		float scale;
	} gizmo;

	struct EditData {
		int mode;
		int plane;
		Transform original;
		Vector3 click_ray;
		Vector3 click_ray_pos;
		Vector3 center;
		Vector3 orig_gizmo_pos;
		int edited_gizmo;
		Vector2 mouse_pos;
		Vector2 original_mouse_pos;
		bool snap;
		Ref<PahdoSpatialGizmo> gizmo;
		int gizmo_handle;
		int gizmo_initial_value;
		Vector3 gizmo_initial_pos;
	} _edit;

	Node* controller;

	Transform original_transform;

	RID origin;
	RID origin_instance;
	Ref<ArrayMesh> move_gizmo[3];
	RID move_gizmo_instance[3];
	Ref<ArrayMesh> rotate_gizmo[4];
	RID rotate_gizmo_instance[4];
	Ref<Material> move_gizmo_color[3];
	Ref<Material> move_gizmo_color_hl[3];
	Ref<Material> rotate_gizmo_color[4];
	Ref<Material> rotate_gizmo_color_hl[4];
	Ref<SpatialMaterial> indicator_mat;
	float gizmo_scale = 1.0;
	
	int over_gizmo_handle;
	bool self_emitted;

	Vector<Ref<PahdoSpatialGizmoPlugin>> gizmos_by_priority;
	Map<String, Ref<PahdoSpatialGizmo>> gizmos_by_name;

	struct GizmoInfo {
		bool gizmo_dirty;
		Ref<PahdoSpatialGizmo> gizmo;
	};

	Map<Spatial*, GizmoInfo> gizmos_by_node;

private:
	void _init_origin();
	void _init_translate(const Vector3& nivec, const Vector3& ivec, const Ref<SpatialMaterial>& p_mat, int idx);
	void _init_rotate(const Vector3& ivec, const Vector3& ivec2, const Color& col, const Color& albedo, int idx);
	void _init_transform();
	void _init_indicators();
	void _init_gizmo_instance();
	void select_gizmo_highlight_axis(int axis);
	void _finish_indicators();
	void _finish_gizmo_instances();
	int get_selected_count();
	bool is_gizmo_visible();
	Vector3 _get_ray_pos(const Vector2& p_screenpos);
	Vector3 _get_ray(const Vector2& p_screenpos);
	Transform get_gizmo_transform();
	Transform get_global_gizmo_transform();
	void _compute_edit(const Vector2& p_screenpos);
	Vector3 _get_camera_normal();
	bool _gizmo_select(const Vector2& p_screenpos, bool p_highlight_only = false);
	void _request_gizmo(Spatial* p_spatial);
	void update_gizmo(Spatial* p_spatial);
	void _update_all_gizmos(const Object* p_node);
	void update_transform_gizmo_view();

protected:
	static void _bind_methods();

public:
	void _update_gizmo(const Object* p_spatial);

	void update_all_gizmos(const Object* p_node = nullptr);
	void update_transform_gizmo();
	void gui_input(Ref<InputEvent> p_event);
	void set_viewport_controller(const Object* p_controller);
	void add_gizmo_plugin(const Ref<PahdoSpatialGizmoPlugin> p_plugin);
	void remove_gizmos_for(const Object* p_node);
	void edit(const Object* p_node);
	void _on_other_transform_changed();

	ViewportGizmoController();
	~ViewportGizmoController();
};

#endif