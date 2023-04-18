#include <sstream>

#include "serializer.hpp"
#include "object.hpp"
#include "point.hpp"

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
				geometry.push_back (serializer.serialize (object.id, object.object));
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

		m_cache[object.get()] = id;
		return true;
	}

	void scene_serializer::reset () {
		m_cache.clear ();
		m_geometry.clear ();
		m_points.clear ();
	}

	// basic serializer
	json empty_object_serializer::serialize (int id, std::shared_ptr<scene_obj_t> object) const {
		json j;

		j["id"] = id;
		j["objectType"] = "unknown";
		j["name"] = object->get_name ();

		return j;
	}

	json generic_object_serializer::serialize (int id, std::shared_ptr<scene_obj_t> object) const {
		json j;

		j[id] = id;
		j["objectType"] = "unknown";
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
}