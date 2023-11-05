#pragma once
#include "surface.hpp"
#include "beziersurf.hpp"
#include "bsplinesurf.hpp"
#include "intersection.hpp"

namespace mini {
	class application;

	class padlock_milling_script final {
		private:
			std::shared_ptr<bezier_surface_c0> m_padlock_keyhole;
			std::shared_ptr<bspline_surface> m_padlock_body;
			std::shared_ptr<bspline_surface> m_padlock_shackle;
			std::shared_ptr<bezier_surface_c0> m_model_base;

			std::vector<float> m_heightmap;
			uint32_t m_hm_width;
			uint32_t m_hm_height;

			float m_hm_units_x;
			float m_hm_units_y;

			float m_base_width;
			float m_base_height;
			float m_base_depth;
			float m_scale;

			application& m_app;

			intersection_controller::result_t m_int_body_hole;
			intersection_controller::result_t m_int_body_shackle1;
			intersection_controller::result_t m_int_body_shackle2;
			intersection_controller::result_t m_int_base_body;
			intersection_controller::result_t m_int_base_shackle1;
			intersection_controller::result_t m_int_base_shackle2;

			std::vector<glm::vec3> m_path_1;
			std::vector<glm::vec3> m_path_2;
			std::vector<glm::vec3> m_path_3;
			std::vector<glm::vec3> m_path_4;

		public:
			padlock_milling_script(application& app);
			~padlock_milling_script();

			padlock_milling_script(const padlock_milling_script&) = delete;
			padlock_milling_script& operator=(const padlock_milling_script&) = delete;

			void run();

		private:
			void m_prepare_base();
			void m_identify_model();
			void m_prepare_heightmap();

			float m_query_bitmap(float x, float y) const;

			void m_gen_path_1();
			void m_gen_path_2();
	};
}