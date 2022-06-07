#ifndef PAHDO_SPATIAL_GIZMO_H
#define PAHDO_SPATIAL_GIZMO_H

#include "scene/3d/skeleton.h"
#include "scene/3d/spatial.h"

class PahdoSpatialGizmoPlugin;

class PahdoSpatialGizmo : public Reference {
	GDCLASS(PahdoSpatialGizmo, Reference);

	bool selected;
	bool instanced;

public:
	void set_selected(bool p_selected) { selected = p_selected; }
	bool is_selected() const { return selected; }

	struct Instance {
		RID instance;
		Ref<Mesh> mesh;
		Ref<Material> material;
		Ref<SkinReference> skin_reference;
		RID skeleton;
		bool billboard;
		bool unscaled;
		bool can_intersect;
		bool extra_margin;
		Instance() {
			billboard = false;
			unscaled = false;
			can_intersect = false;
			extra_margin = false;
		}

		void create_instance(Spatial* p_base, bool p_hidden = false);
	};

	Vector<Vector3> collision_segments;
	Ref<TriangleMesh> collision_mesh;

	struct Handle {
		Vector3 pos;
		bool billboard;
	};

	Vector<Vector3> handles;
	Vector<Vector3> secondary_handles;
	float selectable_icon_size;
	bool billboard_handle;

	bool valid;
	bool hidden;
	Spatial* base;
	Vector<Instance> instances;
	Spatial* spatial_node;
	PahdoSpatialGizmoPlugin* gizmo_plugin;

	void _set_spatial_node(Node* p_node) { set_spatial_node(Object::cast_to<Spatial>(p_node)); }

protected:
	static void _bind_methods();

public:
	void add_lines(const Vector<Vector3>& p_lines, const Ref<Material>& p_material, bool p_billboard = false, const Color& p_modulate = Color(1, 1, 1));
	void add_vertices(const Vector<Vector3>& p_vertices, const Ref<Material>& p_material, Mesh::PrimitiveType p_primitive_type, bool p_billboard = false, const Color& p_modulate = Color(1, 1, 1));
	void add_mesh(const Ref<Mesh>& p_mesh, bool p_billboard = false, const Ref<SkinReference>& p_skin_reference = Ref<SkinReference>(), const Ref<Material>& p_material = Ref<Material>());
	void add_collision_segments(const Vector<Vector3>& p_lines);
	void add_collision_triangles(const Ref<TriangleMesh>& p_tmesh);
	void add_unscaled_billboard(const Ref<Material>& p_material, float p_scale = 1, const Color& p_modulate = Color(1, 1, 1));
	void add_handles(const Vector<Vector3>& p_handles, const Ref<Material>& p_material, bool p_billboard = false, bool p_secondary = false);
	void add_solid_box(Ref<Material>& p_material, Vector3 p_size, Vector3 p_position = Vector3());

	virtual bool is_handle_highlighted(int p_idx) const;
	virtual String get_handle_name(int p_idx) const;
	virtual Variant get_handle_value(int p_idx);
	virtual void set_handle(int p_idx, Camera* p_camera, const Point2& p_point);
	virtual void commit_handle(int p_idx, const Variant& p_restore, bool p_cancel = false);

	void set_spatial_node(Spatial* p_node);
	Spatial* get_spatial_node() const { return spatial_node; }
	Ref<PahdoSpatialGizmoPlugin> get_plugin() const { return gizmo_plugin; }
	Vector3 get_handle_pos(int p_idx) const;
	bool intersect_frustum(const Camera* p_camera, const Vector<Plane>& p_frustum);
	bool intersect_ray(Camera* p_camera, const Point2& p_point, Vector3& r_pos, Vector3& r_normal, int* r_gizmo_handle = nullptr, bool p_sec_first = false);

	virtual void clear();
	virtual void create();
	virtual void transform();
	virtual void redraw();
	virtual void free();

	virtual bool is_editable() const;

	void set_hidden(bool p_hidden);
	void set_plugin(PahdoSpatialGizmoPlugin* p_plugin);

	PahdoSpatialGizmo();
	~PahdoSpatialGizmo();
};

#endif
