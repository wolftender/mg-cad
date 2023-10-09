#include <sstream>

#include "serializer.hpp"
#include "object.hpp"
#include "cube.hpp"
#include "point.hpp"
#include "torus.hpp"
#include "bezier.hpp"
#include "bspline.hpp"
#include "interpolate.hpp"
#include "beziersurf.hpp"
#include "bsplinesurf.hpp"

namespace mini {
#define DESERIALIZER(T) template<> \
		std::shared_ptr<scene_obj_t> generic_object_deserializer<T>::deserialize

#define SERIALIZER(T) template<> json generic_object_serializer<T>::serialize

	inline json s_serialize_data (const glm::vec3 & vec) {
		json v;
		v["x"] = vec.x;
		v["y"] = vec.y;
		v["z"] = vec.z;

		return v;
	}

	inline json s_serialize_color (const glm::vec4 & vec) {
		json v;
		v["r"] = vec.x;
		v["g"] = vec.y;
		v["b"] = vec.z;

		return v;
	}

	scene_serializer::scene_serializer () { }
	scene_serializer::~scene_serializer () { }

	std::string scene_serializer::get_data () {
		json scene;

		{
			json points;
			for (const auto & point : m_points) {
				json point_s;
				
				point_s["id"] = point.id;
				point_s["name"] = point.object->get_name ();
				point_s["position"] = s_serialize_data (point.object->get_translation ());

				points.push_back (point_s);
			}

			scene["points"] = std::move (points);
		}

		{
			json geometry;
			for (auto & object : m_geometry) {
				const auto & serializer = object.object->get_serializer ();
				geometry.push_back (serializer.serialize (object.id, object.object, m_cache));
			}

			scene["geometry"] = std::move (geometry);
		}

		std::stringstream ss;
		ss << scene;

		return ss.str ();
	}

	bool scene_serializer::add_object (std::shared_ptr<scene_obj_t> object) {
		scene_serializer_node node;

		node.id = m_cache.add (object->get_id ());
		node.object = object; 

		if (std::dynamic_pointer_cast<point_object> (object) != nullptr) {
			m_points.push_back (node);
		} else {
			m_geometry.push_back (node);
		}

		return true;
	}

	void scene_serializer::reset () {
		m_cache.clear ();
		m_geometry.clear ();
		m_points.clear ();
	}

	// basic serializer
	json empty_object_serializer::serialize (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		json j;

		j["id"] = id;
		j["objectType"] = "unknown";
		j["name"] = object->get_name ();

		return j;
	}

	inline json s_serialize_base (int id, std::shared_ptr<scene_obj_t> object) {
		json j;

		j["id"] = id;
		j["objectType"] = "undefined";
		j["name"] = object->get_name ();

		if (object->is_movable ()) {
			j["position"] = s_serialize_data (object->get_translation ());
		}

		if (object->is_rotatable ()) {
			j["rotation"] = s_serialize_data (object->get_euler_angles ());
		}

		if (object->is_scalable ()) {
			j["scale"] = s_serialize_data (object->get_scale ());
		}

		return j;
	}

	SERIALIZER (scene_obj_t) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		return s_serialize_base (id, object);
	}

	SERIALIZER (cube_object) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);
		j["objectType"] = "ext.eter.cube";

		return j;
	}

	SERIALIZER (torus_object) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<mini::torus_object> torus = std::dynamic_pointer_cast<mini::torus_object> (object);

		if (torus) {
			json samples;
			samples["x"] = torus->get_div_u ();
			samples["y"] = torus->get_div_v ();

			j["samples"] = samples;
			j["smallRadius"] = torus->get_inner_radius ();
			j["largeRadius"] = torus->get_outer_radius ();

			j["objectType"] = "torus";
		}

		return j;
	}

	json s_serialize_control_points (curve_base * curve, const cache_object_id_t & cache) {
		auto control_points = curve->serialize_points ();
		json serialized;

		for (const auto point_id : control_points) {
			json point_data;
			auto cached_id = cache.get (point_id);

			if (cached_id < 0) {
				throw std::runtime_error ("failed to serialize, incorrect relationship");
			}

			point_data["id"] = cached_id;
			serialized.push_back (point_data);
		}

		return serialized;
	}

	SERIALIZER (bezier_curve_c0) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<bezier_curve_c0> segment = std::dynamic_pointer_cast<mini::bezier_curve_c0> (object);

		if (segment) {
			j["objectType"] = "bezierC0";
			j["color"] = s_serialize_color (segment->get_color ());
			j["controlPoints"] = s_serialize_control_points (segment.get (), cache);
		}

		return j;
	}

	SERIALIZER (bspline_curve) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<bspline_curve> segment = std::dynamic_pointer_cast<mini::bspline_curve> (object);

		if (segment) {
			j["objectType"] = "bezierC2";
			j["color"] = s_serialize_color (segment->get_color ());
			j["controlPoints"] = s_serialize_control_points (segment.get (), cache);
		}

		return j;
	}

	SERIALIZER (interpolating_curve) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<interpolating_curve> segment = std::dynamic_pointer_cast<mini::interpolating_curve> (object);

		if (segment) {
			j["objectType"] = "interpolatingC2";
			j["color"] = s_serialize_color (segment->get_color ());
			j["controlPoints"] = s_serialize_control_points (segment.get (), cache);
		}

		return j;
	}

	static json s_serialize_bicubic (int id, std::shared_ptr<bicubic_surface> surface, cache_object_id_t & cache, 
		const std::string & patch_type, const std::string & surface_type) {

		json j = s_serialize_base (id, surface);

		if (surface) {
			auto patches = surface->serialize_patches ();
			json serialized_patches;

			for (const auto & patch : patches) {
				json serialized_patch, control_points;

				for (uint64_t point_id : patch) {
					json point_data;
					auto cached_id = cache.get (point_id);

					if (cached_id < 0) {
						throw std::runtime_error ("failed to serialize, incorrect relationship");
					}

					point_data["id"] = cached_id;
					control_points.push_back (point_data);
				}

				serialized_patch["controlPoints"] = control_points;
				serialized_patch["id"] = cache.reserve_id ();
				serialized_patch["objectType"] = patch_type;
				serialized_patch["name"] = "patch_c2";
				serialized_patch["samples"] = { { "x", 4 }, { "y", 4 } };

				serialized_patches.push_back (serialized_patch);
			}

			j["patches"] = serialized_patches;
			j["objectType"] = surface_type;
			j["size"] = { { "x", surface->get_patches_x () }, { "y", surface->get_patches_y () } };
		}

		return j;
	}

	SERIALIZER (bezier_surface_c0) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		std::shared_ptr<bezier_surface_c0> surface = std::dynamic_pointer_cast<bezier_surface_c0> (object);
		auto j = s_serialize_bicubic (id, surface, cache, "bezierPatchC0", "bezierSurfaceC0");

		j["parameterWrapped"] = {
			{ "u", surface->is_u_wrapped() },
			{ "v", surface->is_v_wrapped() }
		};

		return j;
	}

	SERIALIZER (bspline_surface) (int id, std::shared_ptr<scene_obj_t> object, cache_object_id_t & cache) const {
		std::shared_ptr<bspline_surface> surface = std::dynamic_pointer_cast<bspline_surface> (object);
		auto j = s_serialize_bicubic(id, surface, cache, "bezierPatchC2", "bezierSurfaceC2");

		j["parameterWrapped"] = {
			{ "u", surface->is_u_wrapped() },
			{ "v", surface->is_v_wrapped() }
		};

		return j;
	}


	///////////////////////////////////////////


	inline glm::vec3 s_deserialize_vector (const json & data) {
		glm::vec3 v;

		v.x = data["x"];
		v.y = data["y"];
		v.z = data["z"];

		return v;
	}

	inline glm::vec3 s_deserialize_color (const json & data) {
		glm::vec3 c;

		c.r = data["r"];
		c.g = data["g"];
		c.b = data["b"];

		return c;
	}

	scene_deserializer::scene_deserializer (scene_controller_base & scene, std::shared_ptr<resource_store> store) : 
		m_scene (scene) {

		m_store = store;
		m_ready = false;
		m_init_deserializers ();
	}

	scene_deserializer::scene_deserializer (scene_controller_base & scene, std::shared_ptr<resource_store> store, const std::string & data) :
		m_scene (scene) {

		m_store = store;
		m_ready = false;
		m_init_deserializers ();
		load (data);
	}

	scene_deserializer::~scene_deserializer () { }

	bool scene_deserializer::load_safe (const std::string & data) {
		try {
			load (data);
		} catch (const std::exception &) {
			return false;
		}

		return true;
	}

	void scene_deserializer::load (const std::string & data) {
		json j = json::parse (data);

		for (const auto & point : j["points"]) {
			m_deserialize_point (point);
		}

		for (const auto & geometry : j["geometry"]) {
			m_deserialize_object (geometry);
		}

		std::sort (m_objects.begin (), m_objects.end (), [](const object_deque_item & a, const object_deque_item & b) {
			return a.id < b.id;
		});
	}

	void scene_deserializer::reset () {
		m_objects.clear ();
		m_cache.clear ();
	}

	bool scene_deserializer::has_next () {
		return !m_objects.empty ();
	}

	std::shared_ptr<scene_obj_t> scene_deserializer::get_next () {
		auto next = m_objects.front ();
		m_objects.pop_front ();

		return next.object;
	}

	void scene_deserializer::m_init_deserializers () {
		m_deserializers.insert ({ "torus", &generic_object_deserializer<torus_object>::get_instance () });
		m_deserializers.insert ({ "ext.eter.cube", &generic_object_deserializer<cube_object>::get_instance () });
		m_deserializers.insert ({ "bezierC0", &generic_object_deserializer<bezier_curve_c0>::get_instance () });
		m_deserializers.insert ({ "bezierC2", &generic_object_deserializer<bspline_curve>::get_instance () });
		m_deserializers.insert ({ "interpolatingC2", &generic_object_deserializer<interpolating_curve>::get_instance () });
		m_deserializers.insert ({ "bezierSurfaceC0", &generic_object_deserializer<bezier_surface_c0>::get_instance () });
		m_deserializers.insert ({ "bezierSurfaceC2", &generic_object_deserializer<bspline_surface>::get_instance () });
	}

	void scene_deserializer::m_deserialize_point (const json & data) {
		int id = data["id"].get<int> ();

		if (m_cache.find (id) != m_cache.end ()) {
			throw std::runtime_error ("duplicate object id " + id);
		}

		auto point = std::make_shared<point_object> (
			m_scene,
			m_store->get_billboard_s_shader (),
			m_store->get_point_texture ()
		);

		if (data.find ("name") != data.end ()) {
			point->set_name (data["name"].get<std::string> ());
		} else {
			point->set_name ("point");
		}
		
		point->set_translation (s_deserialize_vector (data["position"]));

		// if id < 0 then it will not be ever referenced
		if (id >= 0) {
			m_cache.insert ({ id, point });
		}

		m_objects.push_back ({ id, point });
	}

	void scene_deserializer::m_deserialize_object (const json & data) {
		std::string type = data["objectType"].get<std::string> ();
		int id = data["id"].get<int> ();

		if (m_cache.find (id) != m_cache.end ()) {
			throw std::runtime_error ("duplicate object id " + id);
		}

		auto deserializer = m_deserializers.find (type);
		if (deserializer != m_deserializers.end ()) {
			auto object = deserializer->second->deserialize (m_scene, m_store, data, m_cache);

			if (object) {
				m_cache.insert ({ id, object });
				m_objects.push_back ({ id, object });
			}
		}
	}

	inline void s_deserialize_transform (scene_obj_t & obj, const json & data) {
		if (obj.is_movable ()) {
			obj.set_translation (s_deserialize_vector (data["position"]));
		}

		if (obj.is_scalable ()) {
			obj.set_scale (s_deserialize_vector (data["scale"]));
		}

		if (obj.is_rotatable ()) {
			obj.set_euler_angles (s_deserialize_vector (data["rotation"]));
		}
	}

	inline void s_deserialize_point_list (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, const cache_id_object_t & cache, point_list & points) {
		
		if (!data.is_array ()) {
			throw std::runtime_error ("cannot deserialize point list from non-array json data");
		}

		points.reserve (data.size ());

		for (const auto & el : data) {
			int id = el["id"].get<int> ();
			auto it = cache.find (id);

			if (it == cache.end ()) {
				throw std::runtime_error ("failed to deserialize point " + id);
			}

			auto object_ptr = it->second;
			auto point_ptr = std::dynamic_pointer_cast<point_object> (object_ptr);

			if (!point_ptr) {
				throw std::runtime_error ("object id " + std::to_string (id) + " does not refer to a point");
			}

			points.push_back (point_ptr);
		}
	}

	DESERIALIZER (point_object) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		auto point = std::make_shared<point_object> (
			scene,
			store->get_billboard_s_shader (),
			store->get_point_texture ()
		);

		point->set_name (data["name"].get<std::string> ());
		s_deserialize_transform (*point, data);
		return point;
	}

	DESERIALIZER (torus_object) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		unsigned int div_u = data["samples"]["x"].get<unsigned int> ();
		unsigned int div_v = data["samples"]["y"].get<unsigned int> ();

		float small_r = data["smallRadius"].get<float> ();
		float large_r = data["largeRadius"].get<float> ();
		
		auto torus = std::make_shared<torus_object> (
			scene,
			store->get_mesh_shader (),
			store->get_alt_mesh_shader (),
			small_r, large_r
		);

		torus->set_name (data["name"].get<std::string>());
		torus->set_div_u (div_u);
		torus->set_div_v (div_v);

		s_deserialize_transform (*torus, data);
		return torus;
	}

	DESERIALIZER (cube_object) (scene_controller_base & scene, std::shared_ptr<resource_store> store, 
		const json & data, cache_id_object_t & cache) const {

		auto cube = std::make_shared<cube_object> (
			scene,
			store->get_basic_shader ()
		);

		cube->set_name (data["name"].get<std::string> ());
		s_deserialize_transform (*cube, data);
		return cube;
	}

	DESERIALIZER (bezier_curve_c0) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		point_list control_points;
		s_deserialize_point_list (scene, store, data["controlPoints"], cache, control_points);

		auto curve = std::make_shared<bezier_curve_c0> (
			scene,
			store->get_bezier_shader (),
			store->get_line_shader (),
			control_points
		);

		curve->set_name (data["name"].get<std::string> ());
		return curve;
	}

	DESERIALIZER (bspline_curve) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		point_list control_points;
		s_deserialize_point_list (scene, store, data["controlPoints"], cache, control_points);

		auto curve = std::make_shared<bspline_curve> (
			scene,
			store->get_bezier_shader (),
			store->get_line_shader (),
			store->get_billboard_s_shader (),
			store->get_point_texture (),
			control_points
		);

		curve->set_name (data["name"].get<std::string> ());
		return curve;
	}

	DESERIALIZER (interpolating_curve) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		point_list control_points;
		s_deserialize_point_list (scene, store, data["controlPoints"], cache, control_points);

		auto curve = std::make_shared<interpolating_curve> (
			scene,
			store->get_bezier_shader (),
			store->get_line_shader (),
			control_points
		);

		curve->set_name (data["name"].get<std::string> ());
		return curve;
	}

	DESERIALIZER (bezier_surface_c0) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		// this is going to be complicated :)
		point_list control_points;

		std::map<int, GLuint> id_map;
		std::vector<GLuint> topology;

		unsigned int patches_x = data["size"]["x"];
		unsigned int patches_y = data["size"]["y"];

		bool u_wrapped = false;
		bool v_wrapped = false;

		if (data.contains("parameterWrapped")) {
			auto parameterWrapped = data["parameterWrapped"];

			if (parameterWrapped.contains("u")) {
				u_wrapped = parameterWrapped["u"].get<bool>();
			}

			if (parameterWrapped.contains("v")) {
				v_wrapped = parameterWrapped["v"].get<bool>();
			}
		}

		control_points.reserve (patches_x * patches_y * 3 + patches_y * 3 + patches_x * 3 + 1);
		for (const auto & patch : data["patches"]) {
			std::array<GLuint, 16> patch_topology;
			auto type = patch["objectType"].get<std::string> ();

			if (type != "bezierPatchC0") {
				throw std::runtime_error ("invalid patch type " + type + " for surface");
			}

			int patch_index = 0;
			for (const auto & point_s : patch["controlPoints"]) {
				int id = point_s["id"].get<int> ();

				auto iter_id_map = id_map.find (id);
				if (iter_id_map != id_map.end ()) {
					patch_topology[patch_index] = iter_id_map->second;
					patch_index++;
					continue;
				}

				// point was not yet indexed, add it to the index
				auto iter_cache = cache.find (id);
				if (iter_cache == cache.end ()) {
					throw std::runtime_error ("failed to deserialize point " + id);
				}

				auto point = std::dynamic_pointer_cast<point_object> (iter_cache->second);
				if (!point) {
					throw std::runtime_error ("object id " + std::to_string (id) + " does not refer to a point");
				}

				control_points.push_back (point);
				GLuint point_index = static_cast<GLuint> (control_points.size () - 1);

				patch_topology[patch_index] = point_index;
				patch_index++;

				id_map.insert ({ id, point_index });
			}

			if (patch_index != 16) {
				throw std::runtime_error ("incorrect number of control points for c0 surface patch: " + patch_index);
			}

			topology.insert (topology.end (), patch_topology.begin (), patch_topology.end ());
		}

		auto patch = std::make_shared<bezier_surface_c0> (
			scene,
			store->get_bezier_surf_shader (),
			store->get_bezier_surf_solid_shader (),
			store->get_line_shader (),
			patches_x, patches_y, control_points, topology, u_wrapped, v_wrapped);

		patch->set_name (data["name"].get<std::string> ());
		return patch;
	}

	DESERIALIZER (bspline_surface) (scene_controller_base & scene, std::shared_ptr<resource_store> store,
		const json & data, cache_id_object_t & cache) const {

		point_list control_points;

		std::map<int, GLuint> id_map;
		std::vector<GLuint> topology;

		unsigned int patches_x = data["size"]["x"];
		unsigned int patches_y = data["size"]["y"];

		bool u_wrapped = false;
		bool v_wrapped = false;

		if (data.contains("parameterWrapped")) {
			auto parameterWrapped = data["parameterWrapped"];

			if (parameterWrapped.contains("u")) {
				u_wrapped = parameterWrapped["u"].get<bool>();
			}

			if (parameterWrapped.contains("v")) {
				v_wrapped = parameterWrapped["v"].get<bool>();
			}
		}

		control_points.reserve ((3 + patches_x) * (3 + patches_y));
		for (const auto & patch : data["patches"]) {
			std::array<GLuint, 16> patch_topology;
			auto type = patch["objectType"].get<std::string> ();

			if (type != "bezierPatchC2") {
				throw std::runtime_error ("invalid patch type " + type + " for surface");
			}

			int patch_index = 0;
			for (const auto & point_s : patch["controlPoints"]) {
				int id = point_s["id"].get<int> ();

				auto iter_id_map = id_map.find (id);
				if (iter_id_map != id_map.end ()) {
					patch_topology[patch_index] = iter_id_map->second;
					patch_index++;
					continue;
				}

				// point was not yet indexed, add it to the index
				auto iter_cache = cache.find (id);
				if (iter_cache == cache.end ()) {
					throw std::runtime_error ("failed to deserialize point " + id);
				}

				auto point = std::dynamic_pointer_cast<point_object> (iter_cache->second);
				if (!point) {
					throw std::runtime_error ("object id " + std::to_string (id) + " does not refer to a point");
				}

				control_points.push_back (point);
				GLuint point_index = static_cast<GLuint> (control_points.size () - 1);

				patch_topology[patch_index] = point_index;
				patch_index++;

				id_map.insert ({ id, point_index });
			}

			if (patch_index != 16) {
				throw std::runtime_error ("incorrect number of control points for c2 surface patch: " + patch_index);
			}

			topology.insert (topology.end (), patch_topology.begin (), patch_topology.end ());
		}

		auto patch = std::make_shared<bspline_surface> (
			scene,
			store->get_bspline_surf_shader (),
			store->get_bspline_surf_solid_shader (),
			store->get_line_shader (),
			patches_x, patches_y, control_points, topology, u_wrapped, v_wrapped);

		patch->set_name (data["name"].get<std::string> ());
		return patch;
	}

	//////////////////////////////////////////////////

	cache_object_id_t::cache_object_id_t () {
		m_last_object_id = 0;
	}

	cache_object_id_t::~cache_object_id_t () { }

	int cache_object_id_t::add (uint64_t object) {
		int id = reserve_id ();
		m_cache.insert ({object, id});

		return id;
	}

	void cache_object_id_t::clear () {
		m_last_object_id = 0;
		m_cache.clear ();
	}

	bool cache_object_id_t::has (uint64_t object) const {
		auto iter = m_cache.find (object);
		return (iter != m_cache.end ());
	}

	int cache_object_id_t::get (uint64_t object) const {
		auto iter = m_cache.find (object);
		if (iter == m_cache.end ()) {
			return -1;
		}

		return iter->second;
	}

	int cache_object_id_t::reserve_id () {
		return m_last_object_id++;
	}
}