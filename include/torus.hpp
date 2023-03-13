#pragma once
#include <vector>
#include <array>
#include <algorithm>

#include "object.hpp"

namespace mini {
	class torus_object : public scene_obj_t {
		private:
			float m_inner_radius, m_outer_radius;
			int m_div_u, m_div_v;

			bool m_requires_rebuild, m_is_wireframe;

			std::vector<float> m_positions;
			std::vector<float> m_uv;
			std::vector<GLuint> m_indices;

			// standard opengl stuff
			GLuint m_pos_buffer, m_uv_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_alt_shader;

		public:
			torus_object (std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> alt_shader, float inner_radius, float outer_radius);
			~torus_object ();

			torus_object (const torus_object &) = delete;
			torus_object & operator= (const torus_object &) = delete;

			virtual void render (app_context & context, const float_matrix_t & world_matrix) const override;
			virtual void configure () override;

		private:
			void m_generate_geometry ();
			void m_build_geometry (const std::vector<float> & positions, const std::vector<float> & uv, const std::vector<GLuint> & indices);
			void m_free_geometry ();
	};
}