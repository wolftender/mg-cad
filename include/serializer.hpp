#pragma once
#include <string>
#include <list>
#include <unordered_map>
#include <deque>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include "store.hpp"

using namespace nlohmann;

namespace mini {
	class scene_obj_t;
	class scene_controller_base;

	using cache_object_id_t = std::unordered_map<uint64_t, int>;
	using cache_id_object_t = std::unordered_map<int, std::shared_ptr<scene_obj_t>>;

	class object_serializer_base {
		public:
			virtual ~object_serializer_base () = default;
			virtual json serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const = 0;
	};

	class object_deserializer_base {
		public:
			virtual ~object_deserializer_base () = default;
			virtual std::shared_ptr<scene_obj_t> deserialize (scene_controller_base & scene, std::shared_ptr<resource_store> store, 
				const json & data, const cache_id_object_t & cache) const = 0;
	};
	
	class scene_serializer {
		private:
			struct scene_serializer_node {
				std::shared_ptr<scene_obj_t> object;
				int id;
			};

			std::list<scene_serializer_node> m_geometry;
			std::list<scene_serializer_node> m_points;

			cache_object_id_t m_cache;

		public:
			scene_serializer ();
			~scene_serializer ();

			std::string get_data ();
			bool add_object (int id, std::shared_ptr<scene_obj_t> object);

			void reset ();
	};

	class scene_deserializer {
		private:
			struct object_deque_item {
				int id;
				std::shared_ptr<scene_obj_t> object;
			};

			cache_id_object_t m_cache;
			bool m_ready;

			std::deque<object_deque_item> m_objects;
			std::unordered_map<std::string, const object_deserializer_base *> m_deserializers;

			scene_controller_base & m_scene;
			std::shared_ptr<resource_store> m_store;

		public:
			scene_deserializer (scene_controller_base & scene, std::shared_ptr<resource_store> store);
			scene_deserializer (scene_controller_base & scene, std::shared_ptr<resource_store> store, const std::string & data);
			~scene_deserializer ();

			bool load_safe (const std::string & data);
			void load (const std::string & data);

			void reset ();
			bool has_next ();
			std::shared_ptr<scene_obj_t> get_next ();

		private:
			void m_init_deserializers ();
			void m_deserialize_point (const json & data);
			void m_deserialize_object (const json & data);
	};

	class empty_object_serializer : public object_serializer_base {
		public:
			static const empty_object_serializer & get_instance () {
				static empty_object_serializer serializer;
				return serializer;
			}

			virtual json serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const override;
	};

	template<typename T> class generic_object_serializer : public object_serializer_base {
		static_assert (std::is_base_of <scene_obj_t, T> {});

		public:
			static const generic_object_serializer<T> & get_instance () {
				static_assert (std::is_base_of <scene_obj_t, T> {});
				static generic_object_serializer<T> serializer;
				return serializer;
			}

			virtual json serialize (int id, std::shared_ptr<scene_obj_t> object, const cache_object_id_t & cache) const override;
	};

	template<typename T> class generic_object_deserializer : public object_deserializer_base {
		static_assert (std::is_base_of <scene_obj_t, T> {});

		public:
			static const generic_object_deserializer<T> & get_instance () {
				static_assert (std::is_base_of <scene_obj_t, T> {});
				static generic_object_deserializer<T> serializer;
				return serializer;
			}

			virtual std::shared_ptr<scene_obj_t> deserialize (scene_controller_base & scene, std::shared_ptr<resource_store> store, 
				const json & data, const cache_id_object_t & cache) const override;
	};
}