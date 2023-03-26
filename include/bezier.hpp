#pragma once
#include <vector>

#include "object.hpp"
#include "point.hpp"

namespace mini {
	class bezier_curve_c0 : public scene_obj_t {
		private:
			std::shared_ptr<shader_t> m_shader;
			std::vector<std::weak_ptr<point_object>> m_points;
			
		public:
			bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader);
			~bezier_curve_c0 ();

			bezier_curve_c0 (const bezier_curve_c0 &) = delete;
			bezier_curve_c0 & operator= (const bezier_curve_c0 &) = delete;

			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;

		protected:
			virtual void t_on_object_created (std::shared_ptr<scene_obj_t> object) override;
	};
}