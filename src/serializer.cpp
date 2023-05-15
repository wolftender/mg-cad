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

namespace mini {
	inline json s_serialize_data (const glm::vec3 & vec) {
		json v;
		v["x"] = vec.x;
		v["y"] = vec.y;
		v["z"] = vec.z;

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

	bool scene_serializer::add_object (int id, std::shared_ptr<scene_obj_t> object) {
		scene_serializer_node node;

		node.id = id;
		node.object = object;

		if (std::dynamic_pointer_cast<point_object> (object) != nullptr) {
			m_points.push_back (node);
		} else {
			m_geometry.push_back (node);
		}

		m_cache[object->get_id ()] = id;
		return true;
	}

	void scene_serializer::reset () {
		m_cache.clear ();
		m_geometry.clear ();
		m_points.clear ();
	}

	// basic serializer
	json empty_object_serializer::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
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

	template<>
	json generic_object_serializer<scene_obj_t>::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
		return s_serialize_base (id, object);
	}

	template<>
	json generic_object_serializer<cube_object>::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);
		j["objectType"] = "ext.eter.cube";

		return j;
	}

	template<>
	json generic_object_serializer<torus_object>::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
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
			auto it = cache.find (point_id);

			if (it == cache.end ()) {
				throw std::runtime_error ("failed to serialize, incorrect relationship");
			}

			point_data["id"] = it->second;
			serialized.push_back (point_data);
		}

		return serialized;
	}

	template<>
	json generic_object_serializer<bezier_curve_c0>::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<bezier_curve_c0> segment = std::dynamic_pointer_cast<mini::bezier_curve_c0> (object);

		if (segment) {
			j["objectType"] = "bezierC0";
			j["controlPoints"] = s_serialize_control_points (segment.get (), cache);
		}

		return j;
	}

	template<>
	json generic_object_serializer<bspline_curve>::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<bspline_curve> segment = std::dynamic_pointer_cast<mini::bspline_curve> (object);

		if (segment) {
			j["objectType"] = "bezierC2";
			j["controlPoints"] = s_serialize_control_points (segment.get (), cache);
		}

		return j;
	}

	template<>
	json generic_object_serializer<interpolating_curve>::serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const {
		json j = s_serialize_base (id, object);

		std::shared_ptr<interpolating_curve> segment = std::dynamic_pointer_cast<mini::interpolating_curve> (object);

		if (segment) {
			j["objectType"] = "interpolatingC2";
			j["controlPoints"] = s_serialize_control_points (segment.get (), cache);
		}

		return j;
	}
}