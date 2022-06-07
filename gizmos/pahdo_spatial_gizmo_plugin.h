#ifndef PAHDO_SPATIAL_GIZMO_PLUGIN_H
#define PAHDO_SPATIAL_GIZMO_PLUGIN_H

#include "core/resource.h"

class Camera;
class Texture;
class Spatial;
class SpatialMaterial;
class PahdoSpatialGizmo;

class PahdoSpatialGizmoPlugin : public Resource {
	GDCLASS(PahdoSpatialGizmoPlugin, Resource);

public:
	static const int VISIBLE = 0;
	static const int HIDDEN = 1;
	static const int ON_TOP = 2;

protected:
	int current_state;
	List<PahdoSpatialGizmo*> current_gizmos;
	HashMap<String, Vector<Ref<SpatialMaterial>>> materials;

	static void _bind_methods();
	virtual bool has_gizmo(Spatial* p_spatial);
	virtual Ref<PahdoSpatialGizmo> create_gizmo(Spatial* p_spatial);

public:
	void create_material(const String& p_name, const Color& p_color, bool p_billboard = false, bool p_on_top = false, bool p_use_vertex_color = false);
	void create_icon_material(const String& p_name, const Ref<Texture>& p_texture, bool p_on_top = false, const Color& p_albedo = Color(1, 1, 1, 1));
	void create_handle_material(const String& p_name, bool p_billboard = false, const Ref<Texture>& p_icon = nullptr);
	void add_material(const String& p_name, Ref<SpatialMaterial> p_material);

	Ref<SpatialMaterial> get_material(const String& p_name, const Ref<PahdoSpatialGizmo>& p_gizmo = Ref<PahdoSpatialGizmo>());

	virtual String get_name() const;
	virtual int get_priority() const;
	virtual bool can_be_hidden() const;
	virtual bool is_selectable_when_hidden() const;

	virtual void redraw(PahdoSpatialGizmo* p_gizmo);
	virtual String get_handle_name(const PahdoSpatialGizmo* p_gizmo, int p_idx) const;
	virtual Variant get_handle_value(PahdoSpatialGizmo* p_gizmo, int p_idx) const;
	virtual void set_handle(PahdoSpatialGizmo* p_gizmo, int p_idx, Camera* p_camera, const Point2& p_point);
	virtual void commit_handle(PahdoSpatialGizmo* p_gizmo, int p_idx, const Variant& p_restore, bool p_cancel = false);
	virtual bool is_handle_highlighted(const PahdoSpatialGizmo* p_gizmo, int p_idx) const;

	Ref<PahdoSpatialGizmo> get_gizmo(const Object* p_spatial);
	void set_state(int p_state);
	int get_state() const;
	void unregister_gizmo(PahdoSpatialGizmo* p_gizmo);

	PahdoSpatialGizmoPlugin();
	virtual ~PahdoSpatialGizmoPlugin();
};

#endif
