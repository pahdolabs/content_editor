#include "pahdo_spatial_gizmo.h"

#include "pahdo_spatial_gizmo_plugin.h"
#include "scene/3d/camera.h"
#include "scene/3d/skeleton.h"
#include "scene/resources/primitive_meshes.h"

#define HANDLE_HALF_SIZE 9.5

bool PahdoSpatialGizmo::is_editable() const {
	ERR_FAIL_COND_V(!spatial_node, false);
	Node* scene_root = spatial_node->get_tree()->get_current_scene();
	if (spatial_node == scene_root) {
		return true;
	}
	if (spatial_node->get_owner() == scene_root) {
		return true;
	}
	
	if (scene_root->is_a_parent_of(spatial_node)) {
		return true;
	}

	return false;
}

void PahdoSpatialGizmo::clear() {
	for (int i = 0; i < instances.size(); i++) {
		if (instances[i].instance.is_valid()) {
			VS::get_singleton()->free(instances[i].instance);
			instances.write[i].instance = RID();
		}
	}

	billboard_handle = false;
	collision_segments.clear();
	collision_mesh = Ref<TriangleMesh>();
	instances.clear();
	handles.clear();
	secondary_handles.clear();
}

void PahdoSpatialGizmo::redraw() {
	if (get_script_instance() && get_script_instance()->has_method("redraw")) {
		get_script_instance()->call("redraw");
		return;
	}

	ERR_FAIL_COND(!gizmo_plugin);
	gizmo_plugin->redraw(this);
}

String PahdoSpatialGizmo::get_handle_name(int p_idx) const {
	if (get_script_instance() && get_script_instance()->has_method("get_handle_name")) {
		return get_script_instance()->call("get_handle_name", p_idx);
	}

	ERR_FAIL_COND_V(!gizmo_plugin, "");
	return gizmo_plugin->get_handle_name(this, p_idx);
}

bool PahdoSpatialGizmo::is_handle_highlighted(int p_idx) const {
	if (get_script_instance() && get_script_instance()->has_method("is_handle_highlighted")) {
		return get_script_instance()->call("is_handle_highlighted", p_idx);
	}

	ERR_FAIL_COND_V(!gizmo_plugin, false);
	return gizmo_plugin->is_handle_highlighted(this, p_idx);
}

Variant PahdoSpatialGizmo::get_handle_value(int p_idx) {
	if (get_script_instance() && get_script_instance()->has_method("get_handle_value")) {
		return get_script_instance()->call("get_handle_value", p_idx);
	}

	ERR_FAIL_COND_V(!gizmo_plugin, Variant());
	return gizmo_plugin->get_handle_value(this, p_idx);
}

void PahdoSpatialGizmo::set_handle(int p_idx, const Object* p_camera, const Point2& p_point) {
	const Camera* camera = cast_to<Camera>(p_camera);
	ERR_FAIL_COND(!camera);

	if (get_script_instance() && get_script_instance()->has_method("set_handle")) {
		get_script_instance()->call("set_handle", p_idx, const_cast<Camera*>(camera), p_point);
		return;
	}

	ERR_FAIL_COND(!gizmo_plugin);
	gizmo_plugin->set_handle(this, p_idx, const_cast<Camera*>(camera), p_point);
}

void PahdoSpatialGizmo::commit_handle(int p_idx, const Variant& p_restore, bool p_cancel) {
	if (get_script_instance() && get_script_instance()->has_method("commit_handle")) {
		get_script_instance()->call("commit_handle", p_idx, p_restore, p_cancel);
		return;
	}

	ERR_FAIL_COND(!gizmo_plugin);
	gizmo_plugin->commit_handle(this, p_idx, p_restore, p_cancel);
}

void PahdoSpatialGizmo::set_spatial_node(Spatial* p_node) {
	ERR_FAIL_NULL(p_node);
	spatial_node = p_node;
}

void PahdoSpatialGizmo::Instance::create_instance(Spatial* p_base, bool p_hidden) {
	instance = VS::get_singleton()->instance_create2(mesh->get_rid(), p_base->get_world()->get_scenario());
	VS::get_singleton()->instance_set_portal_mode(instance, VisualServer::INSTANCE_PORTAL_MODE_GLOBAL);
	VS::get_singleton()->instance_attach_object_instance_id(instance, p_base->get_instance_id());
	if (skin_reference.is_valid()) {
		VS::get_singleton()->instance_attach_skeleton(instance, skin_reference->get_skeleton());
	}
	if (extra_margin) {
		VS::get_singleton()->instance_set_extra_visibility_margin(instance, 1);
	}
	VS::get_singleton()->instance_geometry_set_cast_shadows_setting(instance, VS::SHADOW_CASTING_SETTING_OFF);
	int layer = p_hidden ? 0 : 1;// << 26; // SpatialEditorViewport::GIZMO_EDIT_LAYER;
	VS::get_singleton()->instance_set_layer_mask(instance, layer); //gizmos are 26
}

void PahdoSpatialGizmo::add_mesh(const Ref<Mesh>& p_mesh, bool p_billboard, const Ref<SkinReference>& p_skin_reference, const Ref<Material>& p_material) {
	ERR_FAIL_COND(!spatial_node);
	ERR_FAIL_COND_MSG(!p_mesh.is_valid(), "EditorSpatialGizmo.add_mesh() requires a valid Mesh resource.");

	Instance ins;

	ins.billboard = p_billboard;
	ins.mesh = p_mesh;
	ins.skin_reference = p_skin_reference;
	ins.material = p_material;
	if (valid) {
		ins.create_instance(spatial_node, hidden);
		VS::get_singleton()->instance_set_transform(ins.instance, spatial_node->get_global_transform());
		if (ins.material.is_valid()) {
			VS::get_singleton()->instance_geometry_set_material_override(ins.instance, p_material->get_rid());
		}
	}

	instances.push_back(ins);
}

void PahdoSpatialGizmo::add_lines(const Vector<Vector3>& p_lines, const Ref<Material>& p_material, bool p_billboard, const Color& p_modulate) {
	if (p_lines.empty()) {
		return;
	}

	ERR_FAIL_COND(!spatial_node);
	Instance ins;

	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);

	a[Mesh::ARRAY_VERTEX] = p_lines;

	PoolVector<Color> color;
	color.resize(p_lines.size());
	{
		PoolVector<Color>::Write w = color.write();
		for (int i = 0; i < p_lines.size(); i++) {
			if (is_selected()) {
				w[i] = Color(1, 1, 1, 0.8) * p_modulate;
			}
			else {
				w[i] = Color(1, 1, 1, 0.2) * p_modulate;
			}
		}
	}

	a[Mesh::ARRAY_COLOR] = color;

	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_LINES, a);
	mesh->surface_set_material(0, p_material);

	if (p_billboard) {
		float md = 0;
		for (int i = 0; i < p_lines.size(); i++) {
			md = MAX(0, p_lines[i].length());
		}
		if (md) {
			mesh->set_custom_aabb(AABB(Vector3(-md, -md, -md), Vector3(md, md, md) * 2.0));
		}
	}

	ins.billboard = p_billboard;
	ins.mesh = mesh;
	if (valid) {
		ins.create_instance(spatial_node, hidden);
		VS::get_singleton()->instance_set_transform(ins.instance, spatial_node->get_global_transform());
	}

	instances.push_back(ins);
}

void PahdoSpatialGizmo::add_vertices(const Vector<Vector3>& p_vertices, const Ref<Material>& p_material, Mesh::PrimitiveType p_primitive_type, bool p_billboard, const Color& p_modulate) {
	if (p_vertices.empty()) {
		return;
	}

	ERR_FAIL_COND(!spatial_node);
	Instance ins;

	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);

	a[Mesh::ARRAY_VERTEX] = p_vertices;

	PoolVector<Color> color;
	color.resize(p_vertices.size());
	{
		PoolVector<Color>::Write w = color.write();
		for (int i = 0; i < p_vertices.size(); i++) {
			if (is_selected()) {
				w[i] = Color(1, 1, 1, 0.8) * p_modulate;
			}
			else {
				w[i] = Color(1, 1, 1, 0.2) * p_modulate;
			}
		}
	}

	a[Mesh::ARRAY_COLOR] = color;

	mesh->add_surface_from_arrays(p_primitive_type, a);
	mesh->surface_set_material(0, p_material);

	if (p_billboard) {
		float md = 0;
		for (int i = 0; i < p_vertices.size(); i++) {
			md = MAX(0, p_vertices[i].length());
		}
		if (md) {
			mesh->set_custom_aabb(AABB(Vector3(-md, -md, -md), Vector3(md, md, md) * 2.0));
		}
	}

	ins.billboard = p_billboard;
	ins.mesh = mesh;
	if (valid) {
		ins.create_instance(spatial_node, hidden);
		VS::get_singleton()->instance_set_transform(ins.instance, spatial_node->get_global_transform());
	}

	instances.push_back(ins);
}

void PahdoSpatialGizmo::add_unscaled_billboard(const Ref<Material>& p_material, float p_scale, const Color& p_modulate) {
	ERR_FAIL_COND(!spatial_node);
	Instance ins;

	Vector<Vector3> vs;
	Vector<Vector2> uv;
	Vector<Color> colors;

	vs.push_back(Vector3(-p_scale, p_scale, 0));
	vs.push_back(Vector3(p_scale, p_scale, 0));
	vs.push_back(Vector3(p_scale, -p_scale, 0));
	vs.push_back(Vector3(-p_scale, -p_scale, 0));

	uv.push_back(Vector2(0, 0));
	uv.push_back(Vector2(1, 0));
	uv.push_back(Vector2(1, 1));
	uv.push_back(Vector2(0, 1));

	colors.push_back(p_modulate);
	colors.push_back(p_modulate);
	colors.push_back(p_modulate);
	colors.push_back(p_modulate);

	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);
	a[Mesh::ARRAY_VERTEX] = vs;
	a[Mesh::ARRAY_TEX_UV] = uv;
	a[Mesh::ARRAY_COLOR] = colors;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLE_FAN, a);
	mesh->surface_set_material(0, p_material);

	float md = 0;
	for (int i = 0; i < vs.size(); i++) {
		md = MAX(0, vs[i].length());
	}
	if (md) {
		mesh->set_custom_aabb(AABB(Vector3(-md, -md, -md), Vector3(md, md, md) * 2.0));
	}

	selectable_icon_size = p_scale;
	mesh->set_custom_aabb(AABB(Vector3(-selectable_icon_size, -selectable_icon_size, -selectable_icon_size) * 100.0f, Vector3(selectable_icon_size, selectable_icon_size, selectable_icon_size) * 200.0f));

	ins.mesh = mesh;
	ins.unscaled = true;
	ins.billboard = true;
	if (valid) {
		ins.create_instance(spatial_node, hidden);
		VS::get_singleton()->instance_set_transform(ins.instance, spatial_node->get_global_transform());
	}

	selectable_icon_size = p_scale;

	instances.push_back(ins);
}

void PahdoSpatialGizmo::add_collision_triangles(const Ref<TriangleMesh>& p_tmesh) {
	collision_mesh = p_tmesh;
}

void PahdoSpatialGizmo::add_collision_segments(const Vector<Vector3>& p_lines) {
	int from = collision_segments.size();
	collision_segments.resize(from + p_lines.size());
	for (int i = 0; i < p_lines.size(); i++) {
		collision_segments.write[from + i] = p_lines[i];
	}
}

void PahdoSpatialGizmo::add_handles(const Vector<Vector3>& p_handles, const Ref<Material>& p_material, bool p_billboard, bool p_secondary) {
	billboard_handle = p_billboard;

	if (!is_selected() || !is_editable()) {
		return;
	}

	ERR_FAIL_COND(!spatial_node);

	Instance ins;

	Ref<ArrayMesh> mesh = memnew(ArrayMesh);

	Array a;
	a.resize(VS::ARRAY_MAX);
	a[VS::ARRAY_VERTEX] = p_handles;
	PoolVector<Color> colors;
	{
		colors.resize(p_handles.size());
		PoolVector<Color>::Write w = colors.write();
		for (int i = 0; i < p_handles.size(); i++) {
			Color col(1, 1, 1, 1);
			if (is_handle_highlighted(i)) {
				col = Color(0, 0, 1, 0.9);
			}

			/*if (SpatialEditor::get_singleton()->get_over_gizmo_handle() != i) {
				col.a = 0.8;
			}*/

			w[i] = col;
		}
	}
	a[VS::ARRAY_COLOR] = colors;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_POINTS, a);
	mesh->surface_set_material(0, p_material);

	if (p_billboard) {
		float md = 0;
		for (int i = 0; i < p_handles.size(); i++) {
			md = MAX(0, p_handles[i].length());
		}
		if (md) {
			mesh->set_custom_aabb(AABB(Vector3(-md, -md, -md), Vector3(md, md, md) * 2.0));
		}
	}

	ins.mesh = mesh;
	ins.billboard = p_billboard;
	ins.extra_margin = true;
	if (valid) {
		ins.create_instance(spatial_node, hidden);
		VS::get_singleton()->instance_set_transform(ins.instance, spatial_node->get_global_transform());
	}
	instances.push_back(ins);
	if (!p_secondary) {
		int chs = handles.size();
		handles.resize(chs + p_handles.size());
		for (int i = 0; i < p_handles.size(); i++) {
			handles.write[i + chs] = p_handles[i];
		}
	}
	else {
		int chs = secondary_handles.size();
		secondary_handles.resize(chs + p_handles.size());
		for (int i = 0; i < p_handles.size(); i++) {
			secondary_handles.write[i + chs] = p_handles[i];
		}
	}
}

void PahdoSpatialGizmo::add_solid_box(Ref<Material>& p_material, Vector3 p_size, Vector3 p_position) {
	ERR_FAIL_COND(!spatial_node);

	CubeMesh cubem;
	cubem.set_size(p_size);

	Array arrays = cubem.surface_get_arrays(0);
	PoolVector3Array vertex = arrays[VS::ARRAY_VERTEX];
	PoolVector3Array::Write w = vertex.write();

	for (int i = 0; i < vertex.size(); ++i) {
		w[i] += p_position;
	}

	arrays[VS::ARRAY_VERTEX] = vertex;

	Ref<ArrayMesh> m = memnew(ArrayMesh);
	m->add_surface_from_arrays(cubem.surface_get_primitive_type(0), arrays);
	m->surface_set_material(0, p_material);
	add_mesh(m);
}

bool PahdoSpatialGizmo::intersect_frustum(const Camera* p_camera, const Vector<Plane>& p_frustum) {
	ERR_FAIL_COND_V(!spatial_node, false);
	ERR_FAIL_COND_V(!valid, false);

	if (hidden && !gizmo_plugin->is_selectable_when_hidden()) {
		return false;
	}

	if (selectable_icon_size > 0.0f) {
		Vector3 origin = spatial_node->get_global_transform().get_origin();

		const Plane* p = p_frustum.ptr();
		int fc = p_frustum.size();

		bool any_out = false;

		for (int j = 0; j < fc; j++) {
			if (p[j].is_point_over(origin)) {
				any_out = true;
				break;
			}
		}

		return !any_out;
	}

	if (collision_segments.size()) {
		const Plane* p = p_frustum.ptr();
		int fc = p_frustum.size();

		int vc = collision_segments.size();
		const Vector3* vptr = collision_segments.ptr();
		Transform t = spatial_node->get_global_transform();

		bool any_out = false;
		for (int j = 0; j < fc; j++) {
			for (int i = 0; i < vc; i++) {
				Vector3 v = t.xform(vptr[i]);
				if (p[j].is_point_over(v)) {
					any_out = true;
					break;
				}
			}
			if (any_out) {
				break;
			}
		}

		if (!any_out) {
			return true;
		}
	}

	if (collision_mesh.is_valid()) {
		Transform t = spatial_node->get_global_transform();

		Vector3 mesh_scale = t.get_basis().get_scale();
		t.orthonormalize();

		Transform it = t.affine_inverse();

		Vector<Plane> transformed_frustum;

		for (int i = 0; i < p_frustum.size(); i++) {
			transformed_frustum.push_back(it.xform(p_frustum[i]));
		}

		Vector<Vector3> convex_points = Geometry::compute_convex_mesh_points(p_frustum.ptr(), p_frustum.size());

		if (collision_mesh->inside_convex_shape(transformed_frustum.ptr(), transformed_frustum.size(), convex_points.ptr(), convex_points.size(), mesh_scale)) {
			return true;
		}
	}

	return false;
}

Dictionary PahdoSpatialGizmo::intersect_ray(const Object* p_camera, const Vector2& p_point, bool p_sec_first) const {
	ERR_FAIL_COND_V(!spatial_node, {});
	ERR_FAIL_COND_V(!valid, {});

	if (hidden && !gizmo_plugin->is_selectable_when_hidden()) {
		return {};
	}

	const Camera* camera_obj = cast_to<Camera>(p_camera);

	ERR_FAIL_COND_V(!camera_obj, {});

	Camera* camera = const_cast<Camera*>(camera_obj);

	int r_gizmo_handle = -1;
	Vector3 r_pos;
	Vector3 r_normal;

	if (!hidden) {
		Transform t = spatial_node->get_global_transform();
		if (billboard_handle) {
			t.set_look_at(t.origin, t.origin - camera->get_transform().basis.get_axis(2), camera->get_transform().basis.get_axis(1));
		}

		float min_d = 1e20;
		int idx = -1;

		for (int i = 0; i < secondary_handles.size(); i++) {
			Vector3 hpos = t.xform(secondary_handles[i]);
			Vector2 p = camera->unproject_position(hpos);

			if (p.distance_to(p_point) < HANDLE_HALF_SIZE) {
				real_t dp = camera->get_transform().origin.distance_to(hpos);
				if (dp < min_d) {
					r_pos = t.xform(hpos);
					r_normal = camera->get_transform().basis.get_axis(2);
					min_d = dp;
					idx = i + handles.size();
				}
			}
		}

		if (p_sec_first && idx != -1) {
			r_gizmo_handle = idx;

			Dictionary output;
			output["pos"] = r_pos;
			output["normal"] = r_normal;
			output["handle"] = r_gizmo_handle;

			return output;
		}

		min_d = 1e20;

		for (int i = 0; i < handles.size(); i++) {
			Vector3 hpos = t.xform(handles[i]);
			Vector2 p = camera->unproject_position(hpos);

			if (p.distance_to(p_point) < HANDLE_HALF_SIZE) {
				real_t dp = camera->get_transform().origin.distance_to(hpos);
				if (dp < min_d) {
					r_pos = t.xform(hpos);
					r_normal = camera->get_transform().basis.get_axis(2);
					min_d = dp;
					idx = i;
				}
			}
		}

		if (idx >= 0) {
			r_gizmo_handle = idx;

			Dictionary output;
			output["pos"] = r_pos;
			output["normal"] = r_normal;
			output["handle"] = r_gizmo_handle;

			return output;
		}
	}

	if (selectable_icon_size > 0.0f) {
		Transform t = spatial_node->get_global_transform();
		Vector3 camera_position = camera->get_camera_transform().origin;
		if (camera_position.distance_squared_to(t.origin) > 0.01) {
			t.set_look_at(t.origin, camera_position, Vector3(0, 1, 0));
		}

		float scale = t.origin.distance_to(camera->get_camera_transform().origin);

		if (camera->get_projection() == Camera::PROJECTION_ORTHOGONAL) {
			float aspect = camera->get_viewport()->get_visible_rect().size.aspect();
			float size = camera->get_size();
			scale = size / aspect;
		}

		Point2 center = camera->unproject_position(t.origin);

		Transform orig_camera_transform = camera->get_camera_transform();

		if (orig_camera_transform.origin.distance_squared_to(t.origin) > 0.01 &&
			ABS(orig_camera_transform.basis.get_axis(Vector3::AXIS_Z).dot(Vector3(0, 1, 0))) < 0.99) {
			camera->look_at(t.origin, Vector3(0, 1, 0));
		}

		Vector3 c0 = t.xform(Vector3(selectable_icon_size, selectable_icon_size, 0) * scale);
		Vector3 c1 = t.xform(Vector3(-selectable_icon_size, -selectable_icon_size, 0) * scale);

		Point2 p0 = camera->unproject_position(c0);
		Point2 p1 = camera->unproject_position(c1);

		camera->set_global_transform(orig_camera_transform);

		Rect2 rect(p0, (p1 - p0).abs());

		rect.set_position(center - rect.get_size() / 2.0);

		if (rect.has_point(p_point)) {
			r_pos = t.origin;
			r_normal = -camera->project_ray_normal(p_point);

			Dictionary output;
			output["pos"] = r_pos;
			output["normal"] = r_normal;
			output["handle"] = r_gizmo_handle;

			return output;
		}
	}

	if (!collision_segments.empty()) {
		Plane camp(camera->get_transform().origin, (-camera->get_transform().basis.get_axis(2)).normalized());

		int vc = collision_segments.size();
		const Vector3* vptr = collision_segments.ptr();
		Transform t = spatial_node->get_global_transform();
		if (billboard_handle) {
			t.set_look_at(t.origin, t.origin - camera->get_transform().basis.get_axis(2), camera->get_transform().basis.get_axis(1));
		}

		Vector3 cp;
		float cpd = 1e20;

		for (int i = 0; i < vc / 2; i++) {
			Vector3 a = t.xform(vptr[i * 2 + 0]);
			Vector3 b = t.xform(vptr[i * 2 + 1]);
			Vector2 s[2];
			s[0] = camera->unproject_position(a);
			s[1] = camera->unproject_position(b);

			Vector2 p = Geometry::get_closest_point_to_segment_2d(p_point, s);

			float pd = p.distance_to(p_point);

			if (pd < cpd) {
				float d = s[0].distance_to(s[1]);
				Vector3 tcp;
				if (d > 0) {
					float d2 = s[0].distance_to(p) / d;
					tcp = a + (b - a) * d2;

				}
				else {
					tcp = a;
				}

				if (camp.distance_to(tcp) < camera->get_znear()) {
					continue;
				}
				cp = tcp;
				cpd = pd;
			}
		}

		if (cpd < 8) {
			r_pos = cp;
			r_normal = -camera->project_ray_normal(p_point);

			Dictionary output;
			output["pos"] = r_pos;
			output["normal"] = r_normal;
			output["handle"] = r_gizmo_handle;

			return output;
		}
	}

	if (collision_mesh.is_valid()) {
		Transform gt = spatial_node->get_global_transform();

		if (billboard_handle) {
			gt.set_look_at(gt.origin, gt.origin - camera->get_transform().basis.get_axis(2), camera->get_transform().basis.get_axis(1));
		}

		Transform ai = gt.affine_inverse();
		Vector3 ray_from = ai.xform(camera->project_ray_origin(p_point));
		Vector3 ray_dir = ai.basis.xform(camera->project_ray_normal(p_point)).normalized();
		Vector3 rpos, rnorm;

		if (collision_mesh->intersect_ray(ray_from, ray_dir, rpos, rnorm)) {
			r_pos = gt.xform(rpos);
			r_normal = gt.basis.xform(rnorm).normalized();

			Dictionary output;
			output["pos"] = r_pos;
			output["normal"] = r_normal;
			output["handle"] = r_gizmo_handle;

			return output;
		}
	}

	return {};
}

void PahdoSpatialGizmo::create() {
	ERR_FAIL_COND(!spatial_node);
	ERR_FAIL_COND(valid);
	valid = true;

	for (int i = 0; i < instances.size(); i++) {
		instances.write[i].create_instance(spatial_node, hidden);
	}

	transform();
}

void PahdoSpatialGizmo::transform() {
	ERR_FAIL_COND(!spatial_node);
	ERR_FAIL_COND(!valid);
	for (int i = 0; i < instances.size(); i++) {
		VS::get_singleton()->instance_set_transform(instances[i].instance, spatial_node->get_global_transform());
	}
}

void PahdoSpatialGizmo::free() {
	ERR_FAIL_COND(!spatial_node);
	ERR_FAIL_COND(!valid);

	for (int i = 0; i < instances.size(); i++) {
		if (instances[i].instance.is_valid()) {
			VS::get_singleton()->free(instances[i].instance);
			instances.write[i].instance = RID();
		}
	}

	clear();

	valid = false;
}

void PahdoSpatialGizmo::set_hidden(bool p_hidden) {
	hidden = p_hidden;
	int layer = hidden ? 0 : 1;// << 26; // SpatialEditorViewport::GIZMO_EDIT_LAYER;
	for (int i = 0; i < instances.size(); ++i) {
		VS::get_singleton()->instance_set_layer_mask(instances[i].instance, layer);
	}
}

void PahdoSpatialGizmo::set_plugin(PahdoSpatialGizmoPlugin* p_plugin) {
	gizmo_plugin = p_plugin;
}

void PahdoSpatialGizmo::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_selected", "is_selected"), &PahdoSpatialGizmo::set_selected);
	ClassDB::bind_method("is_selected", &PahdoSpatialGizmo::is_selected);
	ClassDB::bind_method(D_METHOD("add_lines", "lines", "material", "billboard", "modulate"), &PahdoSpatialGizmo::add_lines, DEFVAL(false), DEFVAL(Color(1, 1, 1)));
	ClassDB::bind_method(D_METHOD("add_mesh", "mesh", "billboard", "skeleton", "material"), &PahdoSpatialGizmo::add_mesh, DEFVAL(false), DEFVAL(Ref<SkinReference>()), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("add_collision_segments", "segments"), &PahdoSpatialGizmo::add_collision_segments);
	ClassDB::bind_method(D_METHOD("add_collision_triangles", "triangles"), &PahdoSpatialGizmo::add_collision_triangles);
	ClassDB::bind_method(D_METHOD("add_unscaled_billboard", "material", "default_scale", "modulate"), &PahdoSpatialGizmo::add_unscaled_billboard, DEFVAL(1), DEFVAL(Color(1, 1, 1)));
	ClassDB::bind_method(D_METHOD("add_handles", "handles", "material", "billboard", "secondary"), &PahdoSpatialGizmo::add_handles, DEFVAL(false), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("set_spatial_node", "node"), &PahdoSpatialGizmo::_set_spatial_node);
	ClassDB::bind_method(D_METHOD("get_spatial_node"), &PahdoSpatialGizmo::get_spatial_node);
	ClassDB::bind_method(D_METHOD("get_plugin"), &PahdoSpatialGizmo::get_plugin);
	ClassDB::bind_method("create", &PahdoSpatialGizmo::create);
	ClassDB::bind_method("transform", &PahdoSpatialGizmo::transform);
	ClassDB::bind_method("clear", &PahdoSpatialGizmo::clear);
	ClassDB::bind_method("redraw", &PahdoSpatialGizmo::redraw);
	ClassDB::bind_method(D_METHOD("set_hidden", "hidden"), &PahdoSpatialGizmo::set_hidden);
	ClassDB::bind_method(D_METHOD("intersect_ray", "camera", "point", "sec_first"), &PahdoSpatialGizmo::intersect_ray, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_handle_name", "index"), &PahdoSpatialGizmo::get_handle_name);
	ClassDB::bind_method(D_METHOD("is_handle_highlighted", "index"), &PahdoSpatialGizmo::is_handle_highlighted);
	ClassDB::bind_method(D_METHOD("get_handle_value", "index"), &PahdoSpatialGizmo::get_handle_value);
	ClassDB::bind_method(D_METHOD("set_handle", "index", "camera", "point"), &PahdoSpatialGizmo::set_handle);
	ClassDB::bind_method(D_METHOD("commit_handle", "index", "restore", "cancel"), &PahdoSpatialGizmo::commit_handle, DEFVAL(false));
}

PahdoSpatialGizmo::PahdoSpatialGizmo() {
	valid = false;
	billboard_handle = false;
	hidden = false;
	base = nullptr;
	selected = false;
	instanced = false;
	spatial_node = nullptr;
	gizmo_plugin = nullptr;
	selectable_icon_size = -1.0f;
}

PahdoSpatialGizmo::~PahdoSpatialGizmo() {
	if (gizmo_plugin != nullptr) {
		gizmo_plugin->unregister_gizmo(this);
	}
	clear();
}

Vector3 PahdoSpatialGizmo::get_handle_pos(int p_idx) const {
	ERR_FAIL_INDEX_V(p_idx, handles.size(), Vector3());

	return handles[p_idx];
}
