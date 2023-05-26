#pragma once
#include "object.hpp"
#include "surface.hpp"

namespace mini {
	class bspline_surface : public bicubic_surface {
		public:
			bspline_surface (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader,
				unsigned int patches_x,
				unsigned int patches_y,
				const std::vector<point_ptr> & points
			);

			bspline_surface (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader,
				unsigned int patches_x,
				unsigned int patches_y,
				const std::vector<point_ptr> & points,
				const std::vector<GLuint> topology
			);

			virtual const object_serializer_base & get_serializer () const;

		protected:
			virtual void t_calc_idx_buffer (std::vector<GLuint> & indices, std::vector<GLuint> & grid_indices) override;
	};
}