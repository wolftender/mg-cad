#pragma once
#include "object.hpp"
#include "bezier.hpp"
#include "surface.hpp"

namespace mini {
	class bezier_surface_c0 : public bicubic_surface {
		private:
			static std::vector<GLuint> s_gen_grid_topology (
				unsigned int patches_x, 
				unsigned int patches_y, 
				const std::vector<GLuint> & topology
			);

		public:
			bezier_surface_c0 (
				scene_controller_base & scene, 
				std::shared_ptr<shader_t> shader, 
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader, 
				unsigned int patches_x, 
				unsigned int patches_y, 
				const std::vector<point_ptr> & points
			);

			bezier_surface_c0 (
				scene_controller_base & scene, 
				std::shared_ptr<shader_t> shader, 
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader, 
				unsigned int patches_x, 
				unsigned int patches_y, 
				const std::vector<point_ptr> & points,
				const std::vector<GLuint> & topology
			);

			virtual const object_serializer_base & get_serializer () const;
			
		protected:
			virtual void t_calc_idx_buffer (std::vector<GLuint> & indices, std::vector<GLuint> & grid_indices) override;
	};
}