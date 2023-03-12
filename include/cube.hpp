#pragma once
#include <glad/glad.h>

#include "../include/context.hpp"

namespace mini {
	class cube_object : public graphics_obj_t {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_t> m_shader;

		public:
			cube_object (std::shared_ptr<shader_t> shader);
			~cube_object ();

			cube_object (const cube_object &) = delete;
			cube_object & operator= (const cube_object &) = delete;

			virtual void render (app_context & context, const float_matrix_t & world_matrix) const override;
	};
}