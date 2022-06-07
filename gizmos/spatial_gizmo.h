#ifndef SPATIAL_GIZMO_H
#define SPATIAL_GIZMO_H

#include "core/reference.h"
#include "core/rid.h"
#include "core/templates/local_vector.h"
#include "core/templates/ordered_hash_map.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/spatial.h"
#include "thirdparty/embree/kernels/common/scene_triangle_mesh.h"

class SkinReference;
class Material;
class Mesh;

class EditorNode3DGizmo : public SpatialGizmo {
	GDCLASS(EditorNode3DGizmo, SpatialGizmo);

	struct Instance {
		RID instance;
		Ref<Mesh> mesh;
		Ref<Material> material;
		Ref<SkinReference> skin_reference;
		bool extra_margin = false;
		Transform xform;

		void create_instance(Spatial* p_base, bool p_hidden = false);
	};

	bool selected;

	Vector<Vector3> collision_segments;
	Ref<embree::TriangleMesh> collision_mesh;

	Vector<Vector3> handles;
	Vector<int> handle_ids;
	Vector<Vector3> secondary_handles;
	Vector<int> secondary_handle_ids;

	real_t selectable_icon_size;
	bool billboard_handle;

	bool valid;
	bool hidden;
	Vector<Instance> instances;
	Spatial* spatial_node = nullptr;

	void _set_spatial_node(Node* p_node) { set_spatial_node(Object::cast_to<Spatial>(p_node)); }

protected:
	static void _bind_methods();

	EditorNode3DGizmoPlugin* gizmo_plugin = nullptr;

	GDVIRTUAL0(_redraw)
		GDVIRTUAL2RC(String, _get_handle_name, int, bool)
		GDVIRTUAL2RC(bool, _is_handle_highlighted, int, bool)
		GDVIRTUAL2RC(Variant, _get_handle_value, int, bool)
		GDVIRTUAL4(_set_handle, int, bool, const Camera3D*, Vector2)
		GDVIRTUAL4(_commit_handle, int, bool, Variant, bool)

		GDVIRTUAL2RC(int, _subgizmos_intersect_ray, const Camera3D*, Vector2)
		GDVIRTUAL2RC(Vector<int>, _subgizmos_intersect_frustum, const Camera3D*, TypedArray<Plane>)
		GDVIRTUAL1RC(Transform3D, _get_subgizmo_transform, int)
		GDVIRTUAL2(_set_subgizmo_transform, int, Transform3D)
		GDVIRTUAL3(_commit_subgizmos, Vector<int>, TypedArray<Transform3D>, bool)
public:
	void add_lines(const Vector<Vector3>& p_lines, const Ref<Material>& p_material, bool p_billboard = false, const Color& p_modulate = Color(1, 1, 1));
	void add_vertices(const Vector<Vector3>& p_vertices, const Ref<Material>& p_material, Mesh::PrimitiveType p_primitive_type, bool p_billboard = false, const Color& p_modulate = Color(1, 1, 1));
	void add_mesh(const Ref<Mesh>& p_mesh, const Ref<Material>& p_material = Ref<Material>(), const Transform3D& p_xform = Transform3D(), const Ref<SkinReference>& p_skin_reference = Ref<SkinReference>());
	void add_collision_segments(const Vector<Vector3>& p_lines);
	void add_collision_triangles(const Ref<TriangleMesh>& p_tmesh);
	void add_unscaled_billboard(const Ref<Material>& p_material, real_t p_scale = 1, const Color& p_modulate = Color(1, 1, 1));
	void add_handles(const Vector<Vector3>& p_handles, const Ref<Material>& p_material, const Vector<int>& p_ids = Vector<int>(), bool p_billboard = false, bool p_secondary = false);
	void add_solid_box(Ref<Material>& p_material, Vector3 p_size, Vector3 p_position = Vector3(), const Transform3D& p_xform = Transform3D());

	virtual bool is_handle_highlighted(int p_id, bool p_secondary) const;
	virtual String get_handle_name(int p_id, bool p_secondary) const;
	virtual Variant get_handle_value(int p_id, bool p_secondary) const;
	virtual void set_handle(int p_id, bool p_secondary, Camera3D* p_camera, const Point2& p_point);
	virtual void commit_handle(int p_id, bool p_secondary, const Variant& p_restore, bool p_cancel = false);

	virtual int subgizmos_intersect_ray(Camera3D* p_camera, const Vector2& p_point) const;
	virtual Vector<int> subgizmos_intersect_frustum(const Camera3D* p_camera, const Vector<Plane>& p_frustum) const;
	virtual Transform3D get_subgizmo_transform(int p_id) const;
	virtual void set_subgizmo_transform(int p_id, Transform3D p_transform);
	virtual void commit_subgizmos(const Vector<int>& p_ids, const Vector<Transform3D>& p_restore, bool p_cancel = false);

	void set_selected(bool p_selected) { selected = p_selected; }
	bool is_selected() const { return selected; }

	void set_spatial_node(Node3D* p_node);
	Node3D* get_spatial_node() const { return spatial_node; }
	Ref<EditorNode3DGizmoPlugin> get_plugin() const { return gizmo_plugin; }
	bool intersect_frustum(const Camera3D* p_camera, const Vector<Plane>& p_frustum);
	void handles_intersect_ray(Camera3D* p_camera, const Vector2& p_point, bool p_shift_pressed, int& r_id, bool& r_secondary);
	bool intersect_ray(Camera3D* p_camera, const Point2& p_point, Vector3& r_pos, Vector3& r_normal);
	bool is_subgizmo_selected(int p_id) const;
	Vector<int> get_subgizmo_selection() const;

	virtual void clear() override;
	virtual void create() override;
	virtual void transform() override;
	virtual void redraw() override;
	virtual void free() override;

	virtual bool is_editable() const;

	void set_hidden(bool p_hidden);
	void set_plugin(EditorNode3DGizmoPlugin* p_plugin);

	EditorNode3DGizmo();
	~EditorNode3DGizmo();
};

#endif