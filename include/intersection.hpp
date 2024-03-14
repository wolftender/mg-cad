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

			bool m_from_cursor;

		public:
			intersection_controller(
				scene_controller_base & scene, 
				std::shared_ptr<resource_store> store, 
				bool from_cursor);

			~intersection_controller();

			intersection_controller(const intersection_controller &) = delete;
			intersection_controller& operator= (const intersection_controller &) = delete;

		private:
			inline void m_wrap_coordinates(const generic_surface_ptr& surface, float& u, float& v) const;

			void m_start_by_cursor(glm::vec2 & s1, glm::vec2 & s2) const;
			glm::vec2 m_get_cursor_projection(const generic_surface_ptr& surface, const glm::vec2& s) const;
			bool m_find_starting_points(glm::vec2 & p1, glm::vec2 & p2, const glm::vec2 & s1, const glm::vec2 & s2) const;
			void m_trace_intersection(const glm::vec2 & s1, const glm::vec2 & s2) const;
	};
}
