#include "viewport_gizmo_controller.h"

#include "pahdo_spatial_gizmo_plugin.h"
#include "content_editor/EditorConsts.h"
#include "scene/3d/camera.h"
#include "scene/gui/viewport_container.h"
#include "scene/resources/surface_tool.h"

void ViewportGizmoController::update_all_gizmos(const Object* p_node) {
	if (!p_node) {
		p_node = cast_to<Node>(controller->get("selected"));
	}
	if (!p_node) {
		return;
	}
	_update_all_gizmos(p_node);
}

void ViewportGizmoController::update_transform_gizmo() {
	AABB center;
	Basis gizmo_basis;
	bool local_gizmo_coords = false;

	if (controller->get("selected")) {
		Transform xf = get_global_gizmo_transform();

		center.position = xf.origin;
		if (local_gizmo_coords) {
			gizmo_basis = xf.basis;
			gizmo_basis = gizmo_basis.orthonormalized();
		}

		gizmo.visible = true;
	}
	update_transform_gizmo_view();
}

bool ViewportGizmoController::_gizmo_select(const Vector2& p_screenpos, bool p_highlight_only) {
	if (!is_gizmo_visible()) {
		return false;
	}

	if (get_selected_count() == 0) {
		if (p_highlight_only) {
			select_gizmo_highlight_axis(-1);
		}
		return false;
	}

	Vector3 ray_pos = _get_ray_pos(p_screenpos);
	Vector3 ray = _get_ray(p_screenpos);

	Transform gt = get_gizmo_transform();
	float gs = gizmo_scale;

	if (static_cast<int>(controller->get("current_mode")) & MOVE_MODE) {
		int col_axis = -1;
		float col_d = 1e20;

		for (int i = 0; i < 3; ++i) {
			Vector3 axis = gt.basis.get_axis(0);
			Vector3 grabber_pos = gt.origin + axis * gs * (GIZMO_ARROW_OFFSET + (GIZMO_ARROW_SIZE * 0.5));
			float grabber_radius = gs * GIZMO_ARROW_SIZE;

			Vector3 r_res;
			Vector3 r_norm;
			if (Geometry::segment_intersects_sphere(ray_pos, ray_pos + ray * MAX_Z, grabber_pos, grabber_radius, &r_res, &r_norm)) {
				float d = r_res.distance_to(ray_pos);
				if (d < col_d) {
					col_d = d;
					col_axis = i;
				}
			}
		}

		bool is_plane_translate = false;
		if (col_axis == -1) {
			col_d = 1e20;

			for (int i = 0; i < 3; ++i) {
				Vector3 ivec2 = gt.basis.get_axis(Math::wrapi(i + 1, 0, 3)).normalized();
				Vector3 ivec3 = gt.basis.get_axis(Math::wrapi(i + 2, 0, 3)).normalized();
				Vector3 axis = gt.basis.get_axis(i);

				Vector3 grabber_pos = gt.origin + (ivec2 + ivec3) * gs * (GIZMO_PLANE_SIZE + GIZMO_PLANE_DST * 0.6667);
				Plane plane(gt.origin, axis.normalized());

				Vector3 inters;
				if (plane.intersects_ray(ray_pos, ray, &inters)) {
					float dist = inters.distance_to(grabber_pos);
					if (dist < (gs * GIZMO_PLANE_SIZE * 1.5)) {
						float d = ray_pos.distance_to(inters);
						if (d < col_d) {
							col_d = d;
							col_axis = i;

							is_plane_translate = true;
						}
					}
				}
			}
		}

		if (col_axis != -1) {
			if (p_highlight_only) {
				select_gizmo_highlight_axis(col_axis + (is_plane_translate ? 6 : 0));
			}
			else {
				_edit.mode = TRANSFORM_TRANSLATE;
				_compute_edit(p_screenpos);
				_edit.plane = TRANSFORM_X_AXIS + col_axis + (is_plane_translate ? 3 : 0);
			}
			return true;
		}
	}

	if (static_cast<int>(controller->get("current_mode")) & ROTATE_MODE) {
		int col_axis = -1;
		float col_d = 1e20;

		for (int i = 0; i < 3; ++i) {
			Vector3 axis = gt.basis.get_axis(i);
			Plane plane(gt.origin, axis.normalized());
			Vector3 inters;
			if (!plane.intersects_ray(ray_pos, ray, &inters)) {
				continue;
			}

			float dist = inters.distance_to(gt.origin);
			Vector3 inters_dir = (inters - gt.origin).normalized();

			if (_get_camera_normal().dot(inters_dir) < 0.005f) {
				if (dist > gs * (GIZMO_CIRCLE_SIZE - GIZMO_RING_HALF_WIDTH) && dist < gs * (GIZMO_CIRCLE_SIZE + GIZMO_RING_HALF_WIDTH)) {
					float d = ray_pos.distance_to(inters);
					if (d < col_d) {
						col_d = d;
						col_axis = i;
					}
				}
			}
		}

		if (col_axis != -1) {
			if (p_highlight_only) {
				select_gizmo_highlight_axis(col_axis + 3);
			}
			else {
				_edit.mode = TRANSFORM_ROTATE;
				_compute_edit(p_screenpos);
				_edit.plane = TRANSFORM_X_AXIS + col_axis;
			}
			return true;
		}
	}

	if (p_highlight_only) {
		select_gizmo_highlight_axis(-1);
	}

	return false;
}

void ViewportGizmoController::_request_gizmo(Spatial* p_spatial) {
	if (p_spatial == nullptr) {
		return;
	}
	Ref<PahdoSpatialGizmo> spg;
	for (int i = 0; i < gizmos_by_priority.size(); ++i) {
		PahdoSpatialGizmoPlugin* plugin = const_cast<PahdoSpatialGizmoPlugin*>(gizmos_by_priority[i].ptr());
		spg = plugin->get_gizmo(p_spatial);

		if (spg.is_valid()) {
			gizmos_by_node.insert(p_spatial, GizmoInfo{ false, spg });

			if (p_spatial == controller->get("selected")) {
				spg->set_selected(true);
				update_gizmo(cast_to<Spatial>(controller->get("selected")));
				spg->create();
				if (p_spatial->is_visible_in_tree()) {
					spg->redraw();
				}
				spg->transform();
			}
			break;
		}
	}
}

void ViewportGizmoController::update_gizmo(Spatial* p_spatial) {
	if (!p_spatial->is_inside_world()) {
		return;
	}
	if (!gizmos_by_node.has(p_spatial)) {
		_request_gizmo(p_spatial);
	}
	if (!gizmos_by_node.has(p_spatial)) {
		return;
	}
	if (gizmos_by_node[p_spatial].gizmo_dirty) {
		return;
	}
	gizmos_by_node[p_spatial].gizmo_dirty = true;
	call_deferred("_update_gizmo", p_spatial);
}

void ViewportGizmoController::_update_all_gizmos(const Object* p_node) {
	Spatial* spatial = const_cast<Spatial*>(cast_to<Spatial>(p_node));
	if (spatial != nullptr) {
		update_gizmo(spatial);
	}
}

void ViewportGizmoController::update_transform_gizmo_view() {
	const ViewportContainer* vp = cast_to<ViewportContainer>(controller);
	if (!vp->is_visible_in_tree()) {
		return;
	}

	Transform xform = get_gizmo_transform();
	Transform camera_xform = cast_to<Camera>(vp->get("camera"))->get_transform();

	if (xform.origin.distance_squared_to(camera_xform.origin) < 0.01) {
		for (int i = 0; i < 3; ++i) {
			VS::get_singleton()->instance_set_visible(move_gizmo_instance[i], false);
			VS::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], false);
		}
		VS::get_singleton()->instance_set_visible(rotate_gizmo_instance[3], false);

		return;
	}

	Vector3 camz = -camera_xform.basis.get_axis(2).normalized();
	Vector3 camy = -camera_xform.basis.get_axis(1).normalized();
	Plane p(camera_xform.origin, camz);
	float gizmo_d = MAX(abs(p.distance_to(xform.origin)), 0.00001);
	float d0 = cast_to<Camera>(vp->get("camera"))->unproject_position(camera_xform.origin + camz * gizmo_d).y;
	float d1 = cast_to<Camera>(vp->get("camera"))->unproject_position(camera_xform.origin + camz * gizmo_d + camy).y;
	float dd = abs(d0 - d1);
	if (dd == 0) {
		dd = 0.0001;
	}

	float gizmo_size = EditorConsts::get_singleton()->named_const("gizmo_size", 80);
	int viewport_base_height = 400;
	gizmo_scale = gizmo_size / abs(dd) * 1 * MIN(viewport_base_height, vp->get_size().height) / viewport_base_height / vp->get_stretch_shrink();
	Vector3 scale = Vector3(1, 1, 1) * gizmo_scale;

	xform.basis = xform.basis.scaled(scale);

	for (int i = 0; i < 3; ++i) {
		VS::get_singleton()->instance_set_transform(move_gizmo_instance[i], xform);
		VS::get_singleton()->instance_set_visible(move_gizmo_instance[i], is_gizmo_visible() && static_cast<int>(controller->get("current_mode")) & MOVE_MODE);
		VS::get_singleton()->instance_set_transform(rotate_gizmo_instance[i], xform);
		VS::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], is_gizmo_visible() && static_cast<int>(controller->get("current_mode")) & ROTATE_MODE);
	}
	VS::get_singleton()->instance_set_transform(rotate_gizmo_instance[3], xform);
	VS::get_singleton()->instance_set_visible(rotate_gizmo_instance[3], is_gizmo_visible() && static_cast<int>(controller->get("current_mode")) & ROTATE_MODE);
}

void ViewportGizmoController::_update_gizmo(const Object* p_spatial) {
	Spatial* spatial = const_cast<Spatial*>(cast_to<Spatial>(p_spatial));
	if (p_spatial == nullptr) {
		return;
	}
	gizmos_by_node[spatial].gizmo_dirty = false;
	if (gizmos_by_node[spatial].gizmo.is_valid()) {
		if (spatial->is_visible_in_tree() && static_cast<int>(controller->get("current_mode")) & WIDGET_MODE) {
			gizmos_by_node[spatial].gizmo->redraw();
		}
		else {
			gizmos_by_node[spatial].gizmo->clear();
		}
	}
}

void ViewportGizmoController::_on_other_transform_changed() {
	if (!self_emitted) {
		update_all_gizmos(controller->get("selected"));
		update_transform_gizmo();
	}
}

void ViewportGizmoController::_init_origin() {
	indicator_mat.instance();
	indicator_mat->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	indicator_mat->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	indicator_mat->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	indicator_mat->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);

	PoolColorArray origin_colors;
	PoolVector3Array origin_points;

	for (int i = 0; i < 3; ++i) {
		Vector3 axis;
		axis[i] = 1;
		Color origin_color;
		switch (i) {
		case 0:
			origin_color = EditorConsts::get_singleton()->named_color("axis_x_color", Color(1, 0, 0));
			break;
		case 1:
			origin_color = EditorConsts::get_singleton()->named_color("axis_y_color", Color(0, 1, 0));
			break;
		case 2:
			origin_color = EditorConsts::get_singleton()->named_color("axis_z_color", Color(0, 0, 1));
			break;
		}

		origin_colors.push_back(origin_color);
		origin_colors.push_back(origin_color);
		origin_colors.push_back(origin_color);
		origin_colors.push_back(origin_color);
		origin_colors.push_back(origin_color);
		origin_colors.push_back(origin_color);

		origin_points.push_back(axis * 1048576);
		origin_points.push_back(axis * 1024);
		origin_points.push_back(axis * 1024);
		origin_points.push_back(axis * -1024);
		origin_points.push_back(axis * -1024);
		origin_points.push_back(axis * -1048576);
	}

	origin = VS::get_singleton()->mesh_create();
	Array d;
	d.resize(VS::ARRAY_MAX);
	d[VS::ARRAY_VERTEX] = origin_points;
	d[VS::ARRAY_COLOR] = origin_colors;

	VS::get_singleton()->mesh_add_surface_from_arrays(origin, VS::PRIMITIVE_LINES, d);
	VS::get_singleton()->mesh_surface_set_material(origin, 0, indicator_mat->get_rid());

	Ref<World> current_world = controller->get_tree()->get_root()->get_world();
	origin_instance = VS::get_singleton()->instance_create2(origin, current_world->get_scenario());
	VS::get_singleton()->instance_set_layer_mask(origin_instance, 1);
	VS::get_singleton()->instance_geometry_set_cast_shadows_setting(origin_instance, VS::SHADOW_CASTING_SETTING_OFF);
}

void ViewportGizmoController::_init_translate(const Vector3& nivec, const Vector3& ivec,
	const Ref<SpatialMaterial>& p_mat, int idx) {
	Ref<SurfaceTool> surf;
	surf.instance();

	surf->begin(Mesh::PRIMITIVE_TRIANGLES);
	const int arrow_points = 5;
	Vector3 arrow[5]{
		nivec * 0.0 + ivec * 0.0,
		nivec * 0.01 + ivec * 0.0,
		nivec * 0.01 + ivec * GIZMO_ARROW_OFFSET,
		nivec * 0.065 + ivec * GIZMO_ARROW_OFFSET,
		nivec * 0.0 + ivec * (GIZMO_ARROW_OFFSET + GIZMO_ARROW_SIZE)
	};

	const int arrow_sides = 16;

	for (int k = 0; k < arrow_sides; ++k) {
		Basis ma = Basis(ivec, Math_PI * 2 * static_cast<float>(k) / arrow_sides);
		Basis mb = Basis(ivec, Math_PI * 2 * static_cast<float>(k + 1) / arrow_sides);

		for (int j = 0; j < arrow_points - 1; ++j) {
			const Vector3 points[4]{
				ma.xform(arrow[j]),
				mb.xform(arrow[j]),
				mb.xform(arrow[j + 1]),
				ma.xform(arrow[j + 1])
			};

			surf->add_vertex(points[0]);
			surf->add_vertex(points[1]);
			surf->add_vertex(points[2]);

			surf->add_vertex(points[0]);
			surf->add_vertex(points[2]);
			surf->add_vertex(points[2]);
		}

		surf->set_material(p_mat);
		surf->commit(move_gizmo[idx]);
	}
}

void ViewportGizmoController::_init_rotate(const Vector3& ivec, const Vector3& ivec2, const Color& col,
	const Color& albedo, int idx) {
	Ref<SurfaceTool> surf;
	surf.instance();
	surf->begin(Mesh::PRIMITIVE_TRIANGLES);

	const int n = 128;
	const int m = 3;

	for (int j = 0; j < n; ++j) {
		Basis basis(ivec, (Math_PI * 2 * j) / n);
		Vector3 vertex = basis.xform(ivec2 * GIZMO_CIRCLE_SIZE);
		for (int k = 0; k < m; ++k) {
			Vector2 ofs(Math::cos((Math_PI * 2 * k) / m), Math::sin((Math_PI * 2 * k) / m));
			Vector3 normal = ivec * ofs.x + ivec * ofs.y;
			surf->add_normal(basis.xform(normal));
			surf->add_vertex(vertex);
		}
	}

	for (int j = 0; j < n; ++j) {
		for (int k = 0; k < m; ++k) {
			int current_ring = j * m;
			int next_ring = ((j + 1) % n) * m;
			int current_segment = k;
			int next_segment = (k + 1) % m;

			surf->add_index(current_ring + next_segment);
			surf->add_index(current_ring + current_segment);
			surf->add_index(next_ring + current_segment);

			surf->add_index(next_ring + current_segment);
			surf->add_index(next_ring + next_segment);
			surf->add_index(current_ring + next_segment);
		}
	}

	Ref<Shader> rotate_shader;
	rotate_shader.instance();
	rotate_shader->set_code("\
shader_type spatial ;\
render_mode unshaded, depth_test_disable;\
uniform vec4 albedo;\
\
mat3 orthonormalize(mat3 m) { \
	vec3 x = normalize(m[0]);\
	vec3 y = normalize(m[1] - x * dot(x, m[1]));\
	vec3 z = m[2] - x * dot(x, m[2]);\
	z = normalize(z - y * (dot(y,m[2])));\
	return mat3(x,y,z);\
} \
\
void vertex() { \
	mat3 mv = orthonormalize(mat3(MODELVIEW_MATRIX));\
	vec3 n = mv * VERTEX;\
	float orientation = dot(vec3(0,0,-1),n);\
	if (orientation <= 0.005) {\
		VERTEX += NORMAL*0.02;\
	} \
} \
\
void fragment() { \
	ALBEDO = albedo.rgb;\
	ALPHA = albedo.a;\
}");

	Ref<ShaderMaterial> rotate_mat;
	rotate_mat.instance();
	rotate_mat->set_render_priority(Material::RENDER_PRIORITY_MAX);
	rotate_mat->set_shader(rotate_shader);
	rotate_mat->set_shader_param("albedo", col);
	rotate_gizmo_color[idx] = rotate_mat;

	Array arrays = surf->commit_to_arrays();
	rotate_gizmo[idx]->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
	rotate_gizmo[idx]->surface_set_material(0, rotate_mat);

	Ref<ShaderMaterial> rotate_mat_hl = rotate_mat->duplicate();
	rotate_mat_hl->set_shader_param("albedo", albedo);
	rotate_gizmo_color_hl[idx] = rotate_mat_hl;

	if (idx == 2) {
		Ref<ShaderMaterial> border_mat = rotate_mat->duplicate();

		Ref<Shader> border_shader;
		border_shader.instance();
		border_shader->set_code("\
shader_type spatial;\
render_mode unshaded, depth_test_disable;\
uniform vec4 albedo;\
\
mat3 orthonormalize(mat3 m) {\
	vec3 x = normalize(m[0]);\
	vec3 y = normalize(m[1] - x * dot(x, m[1]));\
	vec3 z = m[2] - x * dot(x, m[2]);\
	z = normalize(z - y * (dot(y,m[2])));\
	return mat3(x,y,z);\
}\
\
void vertex() {\
	mat3 mv = orthonormalize(mat3(MODELVIEW_MATRIX));\
	mv = inverse(mv);\
	VERTEX += NORMAL*0.008;\
	vec3 camera_dir_local = mv * vec3(0,0,1);\
	vec3 camera_up_local = mv * vec3(0,1,0);\
	mat3 rotation_matrix = mat3(cross(camera_dir_local, camera_up_local), camera_up_local, camera_dir_local);\
	VERTEX = rotation_matrix * VERTEX;\
}\
\
void fragment() {\
	ALBEDO = albedo.rgb;\
	ALPHA = albedo.a;\
}");

		border_mat->set_shader(border_shader);
		border_mat->set_shader_param("albedo", Color(0.75, 0.75, 0.75, col.a / 3.0));

		rotate_gizmo[3].instance();
		rotate_gizmo[3]->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
		rotate_gizmo[3]->surface_set_material(0, border_mat);
	}
}

void ViewportGizmoController::_init_transform() {
	for (int i = 0; i < 3; ++i) {
		Color col;
		switch (i) {
		case 0:
			col = EditorConsts::get_singleton()->named_color("axis_x_color", Color(1, 0, 0));
			break;
		case 1:
			col = EditorConsts::get_singleton()->named_color("axis_y_color", Color(0, 1, 0));
			break;
		case 2:
			col = EditorConsts::get_singleton()->named_color("axis_z_color", Color(0, 0, 1));
			break;
		}
		col.a = EditorConsts::get_singleton()->named_const("gizmo_opacity", 0.9);

		move_gizmo[i] = Ref<ArrayMesh>();
		move_gizmo[i].instance();
		rotate_gizmo[i] = Ref<ArrayMesh>();
		rotate_gizmo[i].instance();

		Ref<SpatialMaterial> mat;
		mat.instance();
		mat->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
		mat->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
		mat->set_render_priority(Material::RENDER_PRIORITY_MAX);
		mat->set_flag(SpatialMaterial::FLAG_DISABLE_DEPTH_TEST, true);
		mat->set_albedo(col);
		move_gizmo_color[i] = mat;

		Ref<SpatialMaterial> mat_hl = mat->duplicate();
		Color albedo = col.from_hsv(col.get_h(), 0.25, 1.0, 1);
		mat_hl->set_albedo(albedo);
		move_gizmo_color_hl[i] = mat_hl;

		Vector3 ivec;
		ivec[i] = 1;

		Vector3 nivec;
		nivec[(i + 1) % 3] = 1;
		nivec[(i + 2) % 3] = 1;

		Vector3 ivec2;
		ivec2[(i + 1) % 3] = 1;

		Vector3 ivec3;
		ivec3[(i + 2) % 3] = 1;

		_init_translate(nivec, ivec, mat, i);
		_init_rotate(ivec, ivec2, col, albedo, i);
	}
}

void ViewportGizmoController::gui_input(const Ref<InputEvent> p_event) {
	Spatial* selected = cast_to<Spatial>(controller->get("selected"));
	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		switch (mb->get_button_index()) {
		case BUTTON_RIGHT:
			if (mb->is_pressed() && _edit.gizmo.is_valid()) {
				_edit.gizmo->commit_handle(_edit.gizmo_handle, _edit.gizmo_initial_value, true);
				_edit.gizmo = Ref<PahdoSpatialGizmo>();
			}

			if (_edit.mode != TRANSFORM_NONE && mb->is_pressed()) {
				_edit.mode = TRANSFORM_NONE;

				if (selected) {
					selected->set_transform(original_transform);
					self_emitted = true;
					emit_signal("transform_changed");
					self_emitted = false;
				}
			}
			break;
		case BUTTON_MIDDLE:
			if (mb->is_pressed() && _edit.mode != TRANSFORM_NONE) {
				switch (_edit.plane) {
				case TRANSFORM_VIEW:
					_edit.plane = TRANSFORM_X_AXIS;
					break;
				case TRANSFORM_X_AXIS:
					_edit.plane = TRANSFORM_Y_AXIS;
					break;
				case TRANSFORM_Y_AXIS:
					_edit.plane = TRANSFORM_Z_AXIS;
					break;
				case TRANSFORM_Z_AXIS:
					_edit.plane = TRANSFORM_VIEW;
					break;
				default:
					break;
				}
			}
			break;
		case BUTTON_LEFT: {
			if (mb->is_pressed()) {
				_edit.mouse_pos = mb->get_position();
				_edit.original_mouse_pos = mb->get_position();
				_edit.snap = false;
				_edit.mode = TRANSFORM_NONE;

				if (selected) {
					Ref<PahdoSpatialGizmo> seg;
					if (gizmos_by_node.has(selected)) {
						seg = gizmos_by_node[selected].gizmo;
					}
					if (seg.is_valid()) {
						Dictionary inters = seg->intersect_ray(controller->get("camera"), _edit.mouse_pos, mb->get_shift());
						if (!inters.empty() && static_cast<int>(inters["handle"]) != -1) {
							_edit.gizmo = seg;
							_edit.gizmo_handle = inters["handle"];
							_edit.gizmo_initial_value = seg->get_handle_value(_edit.gizmo_handle);
							return;
						}
					}
				}

				if (_gizmo_select(_edit.mouse_pos)) {
					return;
				}

				if (static_cast<int>(controller->get("current_mode")) & ROTATE_MODE) {
					if (get_selected_count() == 0) {
						return;
					}
					_edit.mode = TRANSFORM_ROTATE;
					_compute_edit(mb->get_position());
					return;
				}

				if (static_cast<int>(controller->get("current_mode")) & MOVE_MODE) {
					if (get_selected_count() == 0) {
						return;
					}
					_edit.mode = TRANSFORM_TRANSLATE;
					_compute_edit(mb->get_position());
					return;
				}
			}
			else {
				if (_edit.gizmo.is_valid()) {
					_edit.gizmo->commit_handle(_edit.gizmo_handle, _edit.gizmo_initial_value, false);
					_edit.gizmo = Ref<PahdoSpatialGizmo>();
				}

				if (_edit.mode != TRANSFORM_NONE) {
					if (!selected) {
						return;
					}

					selected->set_transform(get_global_gizmo_transform());
					self_emitted = true;
					emit_signal("transform_changed");
					self_emitted = false;
					_edit.mode = TRANSFORM_NONE;
				}
			}
		} break;
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		_edit.mouse_pos = mm->get_position();

		if (selected) {
			Ref<PahdoSpatialGizmo> seg = gizmos_by_node.has(selected) ? gizmos_by_node[selected].gizmo : nullptr;
			if (seg.is_valid()) {
				int selected_handle = -1;
				Dictionary inters = seg->intersect_ray(cast_to<Camera>(controller->get("camera")), _edit.mouse_pos, false);
				if (!inters.empty() && static_cast<int>(inters["handle"]) != -1) {
					selected_handle = inters["handle"];
				}

				if (selected_handle != over_gizmo_handle) {
					over_gizmo_handle = selected_handle;
					update_gizmo(selected);
					if (selected_handle != -1) {
						select_gizmo_highlight_axis(-1);
					}
				}
			}
		}

		if (over_gizmo_handle == -1 && (mm->get_button_mask() & 1) == 0 && !_edit.gizmo.is_valid()) {
			_gizmo_select(_edit.mouse_pos, true);
		}

		if (_edit.gizmo.is_valid()) {
			_edit.gizmo->set_handle(_edit.gizmo_handle, controller->get("camera"), mm->get_position());
			update_all_gizmos(selected);
		}
		else if (mm->get_button_mask() & BUTTON_MASK_LEFT) {
			if (_edit.mode == TRANSFORM_NONE) {
				return;
			}
			Vector3 ray_pos = _get_ray_pos(mm->get_position());
			Vector3 ray = _get_ray(mm->get_position());

			switch (_edit.mode) {
			case TRANSFORM_TRANSLATE: {
				Vector3 motion_mask;
				Plane plane;
				bool plane_mv = false;

				switch (_edit.plane) {
				case TRANSFORM_VIEW:
					plane = Plane(_edit.center, _get_camera_normal());
					break;
				case TRANSFORM_X_AXIS:
					motion_mask = get_gizmo_transform().basis.get_axis(0);
					plane = Plane(_edit.center, motion_mask.cross(motion_mask.cross(_get_camera_normal())));
					break;
				case TRANSFORM_Y_AXIS:
					motion_mask = get_gizmo_transform().basis.get_axis(1);
					plane = Plane(_edit.center, motion_mask.cross(motion_mask.cross(_get_camera_normal())));
					break;
				case TRANSFORM_Z_AXIS:
					motion_mask = get_gizmo_transform().basis.get_axis(2);
					plane = Plane(_edit.center, motion_mask.cross(motion_mask.cross(_get_camera_normal())));
					break;
				case TRANSFORM_YZ:
					plane = Plane(_edit.center, get_gizmo_transform().basis.get_axis(0));
					plane_mv = true;
					break;
				case TRANSFORM_XZ:
					plane = Plane(_edit.center, get_gizmo_transform().basis.get_axis(1));
					plane_mv = true;
					break;
				case TRANSFORM_XY:
					plane = Plane(_edit.center, get_gizmo_transform().basis.get_axis(2));
					plane_mv = true;
					break;
				}

				Vector3 inters;
				if (!plane.intersects_ray(ray_pos, ray, &inters)) {
					return;
				}

				Vector3 click;
				if (!plane.intersects_ray(_edit.click_ray_pos, _edit.click_ray, &click)) {
					return;
				}

				Vector3 motion = inters - click;
				if (_edit.plane != TRANSFORM_VIEW) {
					if (!plane_mv) {
						motion = motion_mask.dot(motion) * motion_mask;
					}
				}

				if (!selected) {
					return;
				}

				Transform t = original_transform;
				selected->set_transform(t);
				update_all_gizmos(selected);
				update_transform_gizmo();

			} break;
			case TRANSFORM_ROTATE: {
				Plane plane;
				Vector3 axis;

				switch (_edit.plane) {
				case TRANSFORM_VIEW:
					plane = Plane(_edit.center, _get_camera_normal());
					break;
				case TRANSFORM_X_AXIS:
					plane = Plane(_edit.center, get_gizmo_transform().basis.get_axis(0));
					axis = Vector3(1, 0, 0);
					break;
				case TRANSFORM_Y_AXIS:
					plane = Plane(_edit.center, get_gizmo_transform().basis.get_axis(1));
					axis = Vector3(0, 1, 0);
					break;
				case TRANSFORM_Z_AXIS:
					plane = Plane(_edit.center, get_gizmo_transform().basis.get_axis(2));
					axis = Vector3(0, 0, 1);
					break;
				}

				Vector3 inters;
				if (!plane.intersects_ray(ray_pos, ray, &inters)) {
					return;
				}

				Vector3 click;
				if (!plane.intersects_ray(_edit.click_ray_pos, _edit.click_ray, &click)) {
					return;
				}

				Vector3 y_axis = (click - _edit.center).normalized();
				Vector3 x_axis = plane.normal.cross(y_axis).normalized();

				float angle = Math::atan2(x_axis.dot(inters - _edit.center), y_axis.dot(inters - _edit.center));
				bool local_coords = _edit.plane != TRANSFORM_VIEW;

				Transform t;

				if (local_coords) {
					Transform original_local = original_transform;
					Basis rot = Basis(axis, angle);

					t.basis = original_local.basis.orthonormalized() * rot;
					t.origin = original_local.origin;

					selected->set_transform(t);
					selected->scale(original_transform.basis.get_scale());
				}
				else {
					Transform original = original_transform;
					Transform r = Transform();
					Transform base = Transform(Basis(), _edit.center);

					r.basis = r.basis.rotated(plane.normal, angle);
					t = base * r * base.inverse() * original;

					selected->set_transform(t);
				}
				update_all_gizmos(selected);
				update_transform_gizmo();
			} break;
			}
		}
	}
}

void ViewportGizmoController::_init_indicators() {
	_init_origin();
	_init_transform();
}

void ViewportGizmoController::_init_gizmo_instance() {
	Ref<World> current_world = controller->get_tree()->get_root()->get_world();
	for (int i = 0; i < 3; ++i) {
		move_gizmo_instance[i] = VS::get_singleton()->instance_create();
		VS::get_singleton()->instance_set_base(move_gizmo_instance[i], move_gizmo[i]->get_rid());
		VS::get_singleton()->instance_set_scenario(move_gizmo_instance[i], current_world->get_scenario());
		VS::get_singleton()->instance_set_visible(move_gizmo_instance[i], false);
		VS::get_singleton()->instance_geometry_set_cast_shadows_setting(move_gizmo_instance[i], VS::SHADOW_CASTING_SETTING_OFF);

		rotate_gizmo_instance[i] = VS::get_singleton()->instance_create();
		VS::get_singleton()->instance_set_base(rotate_gizmo_instance[i], rotate_gizmo[i]->get_rid());
		VS::get_singleton()->instance_set_scenario(rotate_gizmo_instance[i], current_world->get_scenario());
		VS::get_singleton()->instance_set_visible(rotate_gizmo_instance[i], false);
		VS::get_singleton()->instance_geometry_set_cast_shadows_setting(rotate_gizmo_instance[i], VS::SHADOW_CASTING_SETTING_OFF);
	}

	rotate_gizmo_instance[3] = VS::get_singleton()->instance_create();
	VS::get_singleton()->instance_set_base(rotate_gizmo_instance[3], rotate_gizmo[3]->get_rid());
	VS::get_singleton()->instance_set_scenario(rotate_gizmo_instance[3], current_world->get_scenario());
	VS::get_singleton()->instance_set_visible(rotate_gizmo_instance[3], false);
	VS::get_singleton()->instance_geometry_set_cast_shadows_setting(rotate_gizmo_instance[3], VS::SHADOW_CASTING_SETTING_OFF);
}

void ViewportGizmoController::set_viewport_controller(const Object* p_controller) {
	controller = const_cast<Node*>(cast_to<Node>(p_controller));
	if (controller) {
		if (get_script_instance() && get_script_instance()->has_method("register_all_gizmos")) {
			get_script_instance()->call("register_all_gizmos");
		}
		_init_indicators();
		_init_gizmo_instance();
	}
}

struct _GizmoPluginPriorityComparator {
	bool operator()(const Ref<PahdoSpatialGizmoPlugin>& A, const Ref<PahdoSpatialGizmoPlugin>& B) const {
		if (A->get_priority() == B->get_priority()) {
			return A->get_name() < B->get_name();
		}
		return A->get_priority() < B->get_priority();
	}
};

void ViewportGizmoController::add_gizmo_plugin(const Ref<PahdoSpatialGizmoPlugin> p_plugin) {
	gizmos_by_priority.push_back(p_plugin);
	gizmos_by_priority.sort_custom<_GizmoPluginPriorityComparator>();

	update_all_gizmos();
}

void ViewportGizmoController::_finish_indicators() {
	if (origin_instance.is_valid()) {
		VS::get_singleton()->free(origin_instance);
		origin_instance = RID();
	}
	if (origin.is_valid()) {
		VS::get_singleton()->free(origin);
		origin = RID();
	}
}

void ViewportGizmoController::_finish_gizmo_instances() {
	for (int i = 0; i < 3; ++i) {
		VS::get_singleton()->free(move_gizmo_instance[i]);
		VS::get_singleton()->free(rotate_gizmo_instance[i]);
	}
	VS::get_singleton()->free(rotate_gizmo_instance[3]);
}

int ViewportGizmoController::get_selected_count() {
	Node* selected = cast_to<Node>(controller->get("selected"));
	return selected != nullptr ? 1 : 0;
}

bool ViewportGizmoController::is_gizmo_visible() {
	return gizmo.visible;
}

Vector3 ViewportGizmoController::_get_ray_pos(const Vector2& p_screenpos) {
	return cast_to<Camera>(controller->get("camera"))->project_ray_origin(p_screenpos / cast_to<ViewportContainer>(controller)->get_stretch_shrink());
}

Vector3 ViewportGizmoController::_get_ray(const Vector2& p_screenpos) {
	return cast_to<Camera>(controller->get("camera"))->project_ray_normal(p_screenpos) / static_cast<float>(controller->get("stretch_shrink"));
}

Transform ViewportGizmoController::get_gizmo_transform() {
	return controller->get("selected") ? cast_to<Spatial>(controller->get("selected"))->get_transform() : Transform();
}

Transform ViewportGizmoController::get_global_gizmo_transform() {
	return controller->get("selected") ? cast_to<Spatial>(controller->get("selected"))->get_global_transform() : Transform();
}

void ViewportGizmoController::_compute_edit(const Vector2& p_screenpos) {
	_edit.click_ray = _get_ray(p_screenpos);
	_edit.click_ray_pos = _get_ray_pos(p_screenpos);
	_edit.plane = TRANSFORM_VIEW;
	update_transform_gizmo();
	_edit.center = get_gizmo_transform().origin;

	if (controller->get("selected")) {
		original_transform = get_global_gizmo_transform();
	}
}

Vector3 ViewportGizmoController::_get_camera_normal() {
	return -cast_to<Camera>(controller->get("camera"))->get_camera_transform().basis.get_axis(2);
}

void ViewportGizmoController::select_gizmo_highlight_axis(int axis) {
	for (int i = 0; i < 3; ++i) {
		if (move_gizmo[i].is_valid()) {
			move_gizmo[i]->surface_set_material(0, i == axis ? move_gizmo_color_hl[i] : move_gizmo_color[i]);
		}
		if (rotate_gizmo[i].is_valid()) {
			rotate_gizmo[i]->surface_set_material(0, (i + 3) == axis ? rotate_gizmo_color_hl[i] : rotate_gizmo_color[i]);
		}
	}
}

void ViewportGizmoController::remove_gizmos_for(const Object* p_node) {
	Spatial* spatial = const_cast<Spatial*>(cast_to<Spatial>(p_node));
	if (spatial && gizmos_by_node.has(spatial)) {
		gizmos_by_node.erase(spatial);
	}
}

void ViewportGizmoController::edit(const Object* p_node) {
	Spatial* spatial = const_cast<Spatial*>(cast_to<Spatial>(p_node));
	if (spatial != controller->get("selected")) {
		if (controller->get("selected")) {
			Ref<PahdoSpatialGizmo> spg;
			Spatial* selected = cast_to<Spatial>(controller->get("selected"));
			if (gizmos_by_node.has(selected)) {
				spg = gizmos_by_node[selected].gizmo;
			}
			if (spg.is_valid()) {
				spg->set_selected(false);
				update_gizmo(selected);
			}
		}
		controller->set("selected", spatial);
		over_gizmo_handle = -1;

		if (controller->get("selected")) {
			Ref<PahdoSpatialGizmo> spg;
			Spatial* selected = cast_to<Spatial>(controller->get("selected"));
			if (gizmos_by_node.has(selected)) {
				spg = gizmos_by_node[selected].gizmo;
			}
			if (!spg.is_valid()) {
				_request_gizmo(selected);
				spg = gizmos_by_node[selected].gizmo;
			}
			if (spg.is_valid()) {
				spg->set_selected(true);
				update_gizmo(selected);
			}
		}
		update_transform_gizmo();
	}
}

void ViewportGizmoController::_bind_methods() {
	BIND_VMETHOD(MethodInfo("register_all_gizmos"));

	ClassDB::bind_method(D_METHOD("add_gizmo_plugin", "plugin"), &ViewportGizmoController::add_gizmo_plugin);
	ClassDB::bind_method(D_METHOD("gui_input", "event"), &ViewportGizmoController::gui_input);
	ClassDB::bind_method(D_METHOD("update_all_gizmos", "spatial"), &ViewportGizmoController::update_all_gizmos);
	ClassDB::bind_method("update_transform_gizmo", &ViewportGizmoController::update_transform_gizmo);
	ClassDB::bind_method(D_METHOD("edit", "spatial"), &ViewportGizmoController::edit);
	ClassDB::bind_method(D_METHOD("remove_gizmos_for", "spatial"), &ViewportGizmoController::remove_gizmos_for);
	ClassDB::bind_method(D_METHOD("set_viewport_controller", "controller"), &ViewportGizmoController::set_viewport_controller);
	ClassDB::bind_method("_on_other_transform_changed", &ViewportGizmoController::_on_other_transform_changed);

	ClassDB::bind_method(D_METHOD("_update_gizmo", "spatial"), &ViewportGizmoController::_update_gizmo);

	ADD_SIGNAL(MethodInfo("transform_changed"));
}

ViewportGizmoController::ViewportGizmoController() {
	connect(TTR("transform_changed"), this, "_on_other_transform_changed");
}

ViewportGizmoController::~ViewportGizmoController() {
	_finish_indicators();
	_finish_gizmo_instances();
}
