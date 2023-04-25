#pragma once
#include "bezier.hpp"

namespace mini {
	class interpolating_curve : public curve_base {
		private:
			using float_array_t = std::vector<float>;

			std::shared_ptr<shader_t> m_shader1, m_shader2;
			std::vector<glm::vec3> m_bezier_points;

		public:
			interpolating_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1,
				std::shared_ptr<shader_t> shader2);
			~interpolating_curve ();

			interpolating_curve (const interpolating_curve &) = delete;
			interpolating_curve & operator= (const interpolating_curve &) = delete;

			virtual void configure () override;
			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		protected:
			virtual void t_rebuild_curve () override;

		private:
			float_array_t m_solve_tridiag (
				const float_array_t & a, 
				const float_array_t & b, 
				const float_array_t & c,
				const float_array_t & d);
	};
}