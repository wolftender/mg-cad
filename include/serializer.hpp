#pragma once
#include <string>
#include <list>
#include <unordered_map>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

using namespace nlohmann;

namespace mini {
	class scene_obj_t;

	class object_serializer_base {
		public:
			virtual ~object_serializer_base () = default;
			virtual json serialize (int id, std::shared_ptr<scene_obj_t> object) const = 0;
	};

	class object_deserializer_base {
		public:
			virtual ~object_deserializer_base () = default;
			virtual std::shared_ptr<scene_obj_t> deserialize (const json & data) const = 0;
	};
	
	class scene_serializer {
		private:
			struct scene_serializer_node {
				std::shared_ptr<scene_obj_t> object;
				int id;
			};

			std::list<scene_serializer_node> m_geometry;
			std::list<scene_serializer_node> m_points;

			std::unordered_map<scene_obj_t*, int> m_cache;

		public:
			scene_serializer ();
			~scene_serializer ();

			std::string get_data ();
			bool add_object (int id, std::shared_ptr<scene_obj_t> object);

			void reset ();
	};

	class scene_deserializer {
		private:
			std::map<int, std::shared_ptr<scene_obj_t>> m_id_mapping;
			bool m_ready;

		public:
			scene_deserializer ();
			scene_deserializer (const std::string & data);
			~scene_deserializer ();

			bool load (const std::string & data);

			void reset ();
			bool has_next ();
			std::shared_ptr<scene_obj_t> get_next ();
	};

	class empty_object_serializer : public object_serializer_base {
		public:
			static const empty_object_serializer & get_instance () {
				static empty_object_serializer serializer;
				return serializer;
			}

			virtual json serialize (int id, std::shared_ptr<scene_obj_t> object) const override;
	};

	class generic_object_serializer : public object_serializer_base {
		public:
			static const generic_object_serializer & get_instance () {
				static generic_object_serializer serializer;
				return serializer;
			}

			virtual json serialize (int id, std::shared_ptr<scene_obj_t> object) const override;
	};
}