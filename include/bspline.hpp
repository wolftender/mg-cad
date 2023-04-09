#pragma once
#include "bezier.hpp"

namespace mini {
	constexpr const glm::mat4x4 BSPLINE_TO_BEZIER = {
		 1.0f / 6.0f,	0.0f,			0.0f,			0.0f,
		 2.0f / 3.0f,	2.0f / 3.0f,	1.0f / 3.0f,	1.0f / 6.0f,
		 1.0f / 6.0f,	1.0f / 3.0f,	2.0f / 3.0f,	2.0f / 3.0f,
		 0.0f,			0.0f,			0.0f,			1.0f / 6.0f
	};

	constexpr const glm::mat4x4 BEZIER_TO_BSPLINE = {
		 6.0f,  0.0f,  0.0f,  0.0f, 
		-7.0f,  2.0f, -1.0f,  2.0f, 
		 2.0f, -1.0f,  2.0f, -7.0f, 
		 0.0f,  0.0f,  0.0f,  6.0f
	};

	class bspline_curve : public curve_base {
		private:
			std::vector<std::shared_ptr<bezier_segment_base>> m_segments;
			std::vector<std::shared_ptr<point_object>> m_bezier_points;

			std::shared_ptr<shader_t> m_shader1, m_shader2, m_point_shader;
			std::shared_ptr<texture_t> m_point_texture;

		public:
			bspline_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, 
				std::shared_ptr<shader_t> shader2, std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture);
			~bspline_curve ();

			bspline_curve (const bspline_curve &) = delete;
			bspline_curve& operator= (const bspline_curve &) = delete;

			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		protected:
			virtual void t_rebuild_curve () override;
	};
}