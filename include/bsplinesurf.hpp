#pragma once
#include "object.hpp"
#include "surface.hpp"

namespace mini {
	class bspline_surface : public bicubic_surface, public differentiable_surface_base {
		private:
			static std::vector<GLuint> s_gen_grid_topology (
				unsigned int patches_x,
				unsigned int patches_y,
				const std::vector<GLuint> & topology
			);

			bool m_u_wrapped, m_v_wrapped;

		public:
			bspline_surface (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader,
				unsigned int patches_x,
				unsigned int patches_y,
				const std::vector<point_ptr> & points,
				bool u_wrapped,
				bool v_wrapped
			);

			bspline_surface (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
				std::shared_ptr<shader_t> solid_shader,
				std::shared_ptr<shader_t> grid_shader,
				unsigned int patches_x,
				unsigned int patches_y,
				const std::vector<point_ptr> & points,
				const std::vector<GLuint> & topology,
				bool u_wrapped,
				bool v_wrapped
			);

			virtual const object_serializer_base & get_serializer () const;

			// differentiable surface interface
		public:
			// domain information
			virtual float get_min_u() const override;
			virtual float get_max_u() const override;
			virtual float get_min_v() const override;
			virtual float get_max_v() const override;

			// surface point
			virtual glm::vec3 sample(float u, float v) const override;

			// normal
			virtual glm::vec3 normal(float u, float v) const override;

			// first derivatives
			virtual glm::vec3 ddu(float u, float v) const override;
			virtual glm::vec3 ddv(float u, float v) const override;

			virtual bool is_u_wrapped() const;
			virtual bool is_v_wrapped() const;

		protected:
			virtual void t_calc_idx_buffer (std::vector<GLuint> & indices, std::vector<GLuint> & grid_indices) override;
	};
}