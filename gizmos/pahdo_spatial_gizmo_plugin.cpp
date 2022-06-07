#include "pahdo_spatial_gizmo_plugin.h"

#include "pahdo_spatial_gizmo.h"
#include "content_editor/EditorConsts.h"
#include "content_editor/icons_cache.h"
#include "scene/resources/material.h"

void PahdoSpatialGizmoPlugin::create_material(const String& p_name, const Color& p_color, bool p_billboard, bool p_on_top, bool p_use_vertex_color) {
	Color instanced_color = EditorConsts::get_singleton()->named_color("instanced", Color(0.7, 0.7, 0.7, 0.6));

	Vector<Ref<SpatialMaterial>> mats;

	for (int i = 0; i < 4; i++) {
		bool selected = i % 2 == 1;
		bool instanced = i < 2;

		Ref<SpatialMaterial> material = Ref<SpatialMaterial>(memnew(SpatialMaterial));

		Color color = instanced ? instanced_color : p_color;

		if (!selected) {
			color.a *= 0.3;
		}

		material->set_albedo(color);
		material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
		material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
		material->set_render_priority(SpatialMaterial::RENDER_PRIORITY_MIN + 1);

		if (p_use_vertex_color) {
			material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
		}

		if (p_billboard) {
			material->set_billboard_mode(SpatialMaterial::BILLBOARD_ENABLED);
		}

		if (p_on_top && selected) {
			material->set_on_top_of_alpha();
		}

		mats.push_back(material);
	}

	materials[p_name] = mats;
}

void PahdoSpatialGizmoPlugin::create_icon_material(const String& p_name, const Ref<Texture>& p_texture, bool p_on_top, const Color& p_albedo) {
	Color instanced_color = EditorConsts::get_singleton()->named_color("instanced", Color(0.7, 0.7, 0.7, 0.6));

	Vector<Ref<SpatialMaterial>> icons;

	for (int i = 0; i < 4; i++) {
		bool selected = i % 2 == 1;
		bool instanced = i < 2;

		Ref<SpatialMaterial> icon = Ref<SpatialMaterial>(memnew(SpatialMaterial));

		Color color = instanced ? instanced_color : p_albedo;

		if (!selected) {
			color.a *= 0.85;
		}

		icon->set_albedo(color);

		icon->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
		icon->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		icon->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
		icon->set_cull_mode(SpatialMaterial::CULL_DISABLED);
		icon->set_depth_draw_mode(SpatialMaterial::DEPTH_DRAW_DISABLED);
		icon->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
		icon->set_texture(SpatialMaterial::TEXTURE_ALBEDO, p_texture);
		icon->set_flag(SpatialMaterial::FLAG_FIXED_SIZE, true);
		icon->set_billboard_mode(SpatialMaterial::BILLBOARD_ENABLED);
		icon->set_render_priority(SpatialMaterial::RENDER_PRIORITY_MIN);

		if (p_on_top && selected) {
			icon->set_on_top_of_alpha();
		}

		icons.push_back(icon);
	}

	materials[p_name] = icons;
}

void PahdoSpatialGizmoPlugin::create_handle_material(const String& p_name, bool p_billboard, const Ref<Texture>& p_icon) {
	Ref<SpatialMaterial> handle_material = Ref<SpatialMaterial>(memnew(SpatialMaterial));

	handle_material->set_flag(SpatialMaterial::FLAG_UNSHADED, true);
	handle_material->set_flag(SpatialMaterial::FLAG_USE_POINT_SIZE, true);

	Ref<Texture> handle_t = p_icon != nullptr ? p_icon : IconsCache::get_singleton()->get_icon("Editor3DHandle");
	handle_material->set_point_size(handle_t->get_width());
	handle_material->set_texture(SpatialMaterial::TEXTURE_ALBEDO, handle_t);
	handle_material->set_albedo(Color(1, 1, 1));
	handle_material->set_feature(SpatialMaterial::FEATURE_TRANSPARENT, true);
	handle_material->set_flag(SpatialMaterial::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	handle_material->set_flag(SpatialMaterial::FLAG_SRGB_VERTEX_COLOR, true);
	handle_material->set_on_top_of_alpha();
	if (p_billboard) {
		handle_material->set_billboard_mode(SpatialMaterial::BILLBOARD_ENABLED);
		handle_material->set_on_top_of_alpha();
	}

	materials[p_name] = Vector<Ref<SpatialMaterial>>();
	materials[p_name].push_back(handle_material);
}

void PahdoSpatialGizmoPlugin::add_material(const String& p_name, Ref<SpatialMaterial> p_material) {
	materials[p_name] = Vector<Ref<SpatialMaterial>>();
	materials[p_name].push_back(p_material);
}

Ref<SpatialMaterial> PahdoSpatialGizmoPlugin::get_material(const String& p_name, const Ref<PahdoSpatialGizmo>& p_gizmo) {
	ERR_FAIL_COND_V(!materials.has(p_name), Ref<SpatialMaterial>());
	ERR_FAIL_COND_V(materials[p_name].size() == 0, Ref<SpatialMaterial>());

	if (p_gizmo.is_null() || materials[p_name].size() == 1) {
		return materials[p_name][0];
	}

	int index = (p_gizmo->is_selected() ? 1 : 0) + (p_gizmo->is_editable() ? 2 : 0);

	Ref<SpatialMaterial> mat = materials[p_name][index];

	if (current_state == ON_TOP && p_gizmo->is_selected()) {
		mat->set_flag(SpatialMaterial::FLAG_DISABLE_DEPTH_TEST, true);
	}
	else {
		mat->set_flag(SpatialMaterial::FLAG_DISABLE_DEPTH_TEST, false);
	}

	return mat;
}

String PahdoSpatialGizmoPlugin::get_name() const {
	if (get_script_instance() && get_script_instance()->has_method("get_name")) {
		return get_script_instance()->call("get_name");
	}
	WARN_PRINT_ONCE("A 3D editor gizmo has no name defined (it will appear as \"Unnamed Gizmo\" in the \"View > Gizmos\" menu). To resolve this, override the `get_name()` function to return a String in the script that extends PahdoSpatialGizmoPlugin.");
	return TTR("Unnamed Gizmo");
}

int PahdoSpatialGizmoPlugin::get_priority() const {
	if (get_script_instance() && get_script_instance()->has_method("get_priority")) {
		return get_script_instance()->call("get_priority");
	}
	return 0;
}

Ref<PahdoSpatialGizmo> PahdoSpatialGizmoPlugin::get_gizmo(const Object* p_spatial) {
	Spatial* spatial = const_cast<Spatial*>(cast_to<Spatial>(p_spatial));
	if(spatial == nullptr) {
		return nullptr;
	}
	Ref<PahdoSpatialGizmo> ref = create_gizmo(spatial);

	if (ref.is_null()) {
		return ref;
	}

	ref->set_plugin(this);
	ref->set_spatial_node(spatial);
	ref->set_hidden(current_state == HIDDEN);

	current_gizmos.push_back(ref.ptr());
	return ref;
}

void PahdoSpatialGizmoPlugin::_bind_methods() {
#define GIZMO_REF PropertyInfo(Variant::OBJECT, "gizmo", PROPERTY_HINT_RESOURCE_TYPE, "PahdoSpatialGizmo")

	BIND_VMETHOD(MethodInfo(Variant::BOOL, "has_gizmo", PropertyInfo(Variant::OBJECT, "spatial", PROPERTY_HINT_RESOURCE_TYPE, "Spatial")));
	BIND_VMETHOD(MethodInfo(GIZMO_REF, "create_gizmo", PropertyInfo(Variant::OBJECT, "spatial", PROPERTY_HINT_RESOURCE_TYPE, "Spatial")));

	ClassDB::bind_method(D_METHOD("get_gizmo", "spatial"), &PahdoSpatialGizmoPlugin::get_gizmo);
	ClassDB::bind_method(D_METHOD("create_material", "name", "color", "billboard", "on_top", "use_vertex_color"), &PahdoSpatialGizmoPlugin::create_material, DEFVAL(false), DEFVAL(false), DEFVAL(false));
	ClassDB::bind_method(D_METHOD("create_icon_material", "name", "texture", "on_top", "color"), &PahdoSpatialGizmoPlugin::create_icon_material, DEFVAL(false), DEFVAL(Color(1, 1, 1, 1)));
	ClassDB::bind_method(D_METHOD("create_handle_material", "name", "billboard", "texture"), &PahdoSpatialGizmoPlugin::create_handle_material, DEFVAL(false), DEFVAL(Variant()));
	ClassDB::bind_method(D_METHOD("add_material", "name", "material"), &PahdoSpatialGizmoPlugin::add_material);

	ClassDB::bind_method(D_METHOD("get_material", "name", "gizmo"), &PahdoSpatialGizmoPlugin::get_material, DEFVAL(Ref<PahdoSpatialGizmo>()));

	BIND_VMETHOD(MethodInfo(Variant::STRING, "get_name"));
	BIND_VMETHOD(MethodInfo(Variant::INT, "get_priority"));
	BIND_VMETHOD(MethodInfo(Variant::BOOL, "can_be_hidden"));
	BIND_VMETHOD(MethodInfo(Variant::BOOL, "is_selectable_when_hidden"));

	BIND_VMETHOD(MethodInfo("redraw", GIZMO_REF));
	BIND_VMETHOD(MethodInfo(Variant::STRING, "get_handle_name", GIZMO_REF, PropertyInfo(Variant::INT, "index")));

	MethodInfo hvget(Variant::NIL, "get_handle_value", GIZMO_REF, PropertyInfo(Variant::INT, "index"));
	hvget.return_val.usage |= PROPERTY_USAGE_NIL_IS_VARIANT;
	BIND_VMETHOD(hvget);

	BIND_VMETHOD(MethodInfo("set_handle", GIZMO_REF, PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::OBJECT, "camera", PROPERTY_HINT_RESOURCE_TYPE, "Camera"), PropertyInfo(Variant::VECTOR2, "point")));
	MethodInfo cm = MethodInfo("commit_handle", GIZMO_REF, PropertyInfo(Variant::INT, "index"), PropertyInfo(Variant::NIL, "restore"), PropertyInfo(Variant::BOOL, "cancel"));
	cm.default_arguments.push_back(false);
	BIND_VMETHOD(cm);

	BIND_VMETHOD(MethodInfo(Variant::BOOL, "is_handle_highlighted", GIZMO_REF, PropertyInfo(Variant::INT, "index")));

#undef GIZMO_REF
}

bool PahdoSpatialGizmoPlugin::has_gizmo(Spatial* p_spatial) {
	if (get_script_instance() && get_script_instance()->has_method("has_gizmo")) {
		return get_script_instance()->call("has_gizmo", p_spatial);
	}
	return false;
}

Ref<PahdoSpatialGizmo> PahdoSpatialGizmoPlugin::create_gizmo(Spatial* p_spatial) {
	if (get_script_instance() && get_script_instance()->has_method("create_gizmo")) {
		return get_script_instance()->call("create_gizmo", p_spatial);
	}

	Ref<PahdoSpatialGizmo> ref;
	if (has_gizmo(p_spatial)) {
		ref.instance();
	}
	return ref;
}

bool PahdoSpatialGizmoPlugin::can_be_hidden() const {
	if (get_script_instance() && get_script_instance()->has_method("can_be_hidden")) {
		return get_script_instance()->call("can_be_hidden");
	}
	return true;
}

bool PahdoSpatialGizmoPlugin::is_selectable_when_hidden() const {
	if (get_script_instance() && get_script_instance()->has_method("is_selectable_when_hidden")) {
		return get_script_instance()->call("is_selectable_when_hidden");
	}
	return false;
}

void PahdoSpatialGizmoPlugin::redraw(PahdoSpatialGizmo* p_gizmo) {
	if (get_script_instance() && get_script_instance()->has_method("redraw")) {
		Ref<PahdoSpatialGizmo> ref(p_gizmo);
		get_script_instance()->call("redraw", ref);
	}
}

String PahdoSpatialGizmoPlugin::get_handle_name(const PahdoSpatialGizmo* p_gizmo, int p_idx) const {
	if (get_script_instance() && get_script_instance()->has_method("get_handle_name")) {
		return get_script_instance()->call("get_handle_name", p_gizmo, p_idx);
	}
	return "";
}

Variant PahdoSpatialGizmoPlugin::get_handle_value(PahdoSpatialGizmo* p_gizmo, int p_idx) const {
	if (get_script_instance() && get_script_instance()->has_method("get_handle_value")) {
		return get_script_instance()->call("get_handle_value", p_gizmo, p_idx);
	}
	return Variant();
}

void PahdoSpatialGizmoPlugin::set_handle(PahdoSpatialGizmo* p_gizmo, int p_idx, Camera* p_camera, const Point2& p_point) {
	if (get_script_instance() && get_script_instance()->has_method("set_handle")) {
		get_script_instance()->call("set_handle", p_gizmo, p_idx, p_camera, p_point);
	}
}

void PahdoSpatialGizmoPlugin::commit_handle(PahdoSpatialGizmo* p_gizmo, int p_idx, const Variant& p_restore, bool p_cancel) {
	if (get_script_instance() && get_script_instance()->has_method("commit_handle")) {
		get_script_instance()->call("commit_handle", p_gizmo, p_idx, p_restore, p_cancel);
	}
}

bool PahdoSpatialGizmoPlugin::is_handle_highlighted(const PahdoSpatialGizmo* p_gizmo, int p_idx) const {
	if (get_script_instance() && get_script_instance()->has_method("is_handle_highlighted")) {
		return get_script_instance()->call("is_handle_highlighted", p_gizmo, p_idx);
	}
	return false;
}

void PahdoSpatialGizmoPlugin::set_state(int p_state) {
	current_state = p_state;
	for (int i = 0; i < current_gizmos.size(); ++i) {
		current_gizmos[i]->set_hidden(current_state == HIDDEN);
	}
}

int PahdoSpatialGizmoPlugin::get_state() const {
	return current_state;
}

void PahdoSpatialGizmoPlugin::unregister_gizmo(PahdoSpatialGizmo* p_gizmo) {
	current_gizmos.erase(p_gizmo);
}

PahdoSpatialGizmoPlugin::PahdoSpatialGizmoPlugin() {
	current_state = VISIBLE;
}

PahdoSpatialGizmoPlugin::~PahdoSpatialGizmoPlugin() {
	for (int i = 0; i < current_gizmos.size(); ++i) {
		current_gizmos[i]->set_plugin(nullptr);
		current_gizmos[i]->get_spatial_node()->set_gizmo(nullptr);
	}
	/*if (SpatialEditor::get_singleton()) {
		SpatialEditor::get_singleton()->update_all_gizmos();
	}*/
}
