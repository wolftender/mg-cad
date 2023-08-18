#pragma once
#include "object.hpp"
#include "surface.hpp"

namespace mini {
	class intersection_controller final {
		using generic_surface_ptr = std::shared_ptr<differentiable_surface_base>;

		private:
			generic_surface_ptr m_surface1;
			generic_surface_ptr m_surface2;

			scene_controller_base & m_scene;
			std::shared_ptr<resource_store> m_store;

		public:
			intersection_controller(scene_controller_base & scene, std::shared_ptr<resource_store> store);

			~intersection_controller();

			intersection_controller(const intersection_controller &) = delete;
			intersection_controller& operator= (const intersection_controller &) = delete;

		private:
			void m_find_starting_points(const glm::vec2 & p1, const glm::vec2 & p2) const;
	};
}