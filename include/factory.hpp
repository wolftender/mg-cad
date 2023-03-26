#pragma once
#include <functional>
#include <vector>

#include "object.hpp"
#include "store.hpp"

namespace mini {
	// this is probably the dumbest interface in existance
	// incomprehensible horror
	class object_factory final {
		private:
			using object_ctor_t = std::function<std::shared_ptr<scene_obj_t> (scene_controller_base &, std::shared_ptr<const resource_store>)>;

			struct object_factory_impl_t {
				object_ctor_t ctor;
				std::string name, description;

				object_factory_impl_t (object_ctor_t c, const std::string & n, const std::string & d);
			};

		private:
			std::vector<object_factory_impl_t> m_factories;
			std::shared_ptr<resource_store> m_store;

		public:
			object_factory (std::shared_ptr<resource_store> store);
			~object_factory () = default;

			object_factory (const object_factory &) = delete;
			object_factory & operator= (const object_factory &) = delete;

			std::shared_ptr<scene_obj_t> configure (scene_controller_base & scene) const;

		private:
			static std::shared_ptr<scene_obj_t> make_test_cube (scene_controller_base & scene, std::shared_ptr<const resource_store> store);
			static std::shared_ptr<scene_obj_t> make_torus (scene_controller_base & scene, std::shared_ptr<const resource_store> store);
			static std::shared_ptr<scene_obj_t> make_point (scene_controller_base & scene, std::shared_ptr<const resource_store> store);
	};
}