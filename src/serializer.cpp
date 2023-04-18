#include <sstream>

#include "serializer.hpp"
#include "object.hpp"
#include "point.hpp"

namespace mini {
	scene_serializer::scene_serializer () { }
	scene_serializer::~scene_serializer () { }

	std::string scene_serializer::get_data () {
		json scene;

		{
			json points;
			for (const auto & point : m_points) {
				json point_s;
				
				point_s["id"] = point.id;
				point_s["position"] = m_serialize_data (point.object->get_translation ());

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

	json scene_serializer::m_serialize_data (const glm::vec3 & vec) const {
		json v;
		v["x"] = vec.x;
		v["y"] = vec.y;
		v["z"] = vec.z;

		return v;
	}

	// basic serializer
	json empty_object_serializer::serialize (int id, std::shared_ptr<scene_obj_t> object) const {
		json j;

		j["id"] = id;
		j["objectType"] = "unknown";
		j["name"] = object->get_name ();

		return j;
	}
}