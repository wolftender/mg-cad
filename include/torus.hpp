#pragma once
#include <vector>
#include <array>
#include <algorithm>

#include "object.hpp"
#include "surface.hpp"

namespace mini {
	class torus_object : public scene_obj_t, public differentiable_surface_base {
		private:
			float m_inner_radius, m_outer_radius;
			int m_div_u, m_div_v;

			bool m_requires_rebuild, m_is_wireframe;

			std::vector<float> m_positions;
			std::vector<float> m_uv;
			std::vector<float> m_domain_uv;
			std::vector<GLuint> m_indices;

			// standard opengl stuff
			GLuint m_pos_buffer, m_uv_buffer, m_tex_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_t> m_shader;
			
			trimmable_surface_domain m_domain;

		public:
			torus_object (scene_controller_base & scene, std::shared_ptr<shader_t> shader, float inner_radius, float outer_radius);
			~torus_object ();

			torus_object (const torus_object &) = delete;
			torus_object & operator= (const torus_object &) = delete;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;
			virtual const object_serializer_base & get_serializer () const;

			int get_div_u () const;
			int get_div_v () const;

			float get_inner_radius () const;
			float get_outer_radius () const;

			void set_div_u (int div_u);
			void set_div_v (int div_v);

			void set_inner_radius (float r);
			void set_outer_radius (float r);
			
		// differentiable surface interface
		public:
			virtual float get_min_u() const override;
			virtual float get_max_u() const override;
			virtual float get_min_v() const override;
			virtual float get_max_v() const override;

			virtual glm::vec3 sample(float u, float v) const override;
			virtual glm::vec3 normal(float u, float v) const override;

			virtual glm::vec3 ddu(float u, float v) const override;
			virtual glm::vec3 ddv(float u, float v) const override;

			virtual bool is_u_wrapped() const override;
			virtual bool is_v_wrapped() const override;

			virtual bool is_trimmable() const;
			virtual trimmable_surface_domain& get_trimmable_domain();

		private:
			void m_rebuild ();
			void m_generate_geometry ();

			void m_build_geometry (
				const std::vector<float> & positions, 
				const std::vector<float> & uv, 
				const std::vector<float> & domain_uv, 
				const std::vector<GLuint> & indices);

			void m_free_geometry ();
	};
}
