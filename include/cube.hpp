#pragma once
#include <glad/glad.h>

#include "context.hpp"
#include "object.hpp"

namespace mini {
	class cube_object : public scene_obj_t {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_index_buffer, m_vao;
			std::shared_ptr<shader_t> m_shader;

		public:
			cube_object (std::shared_ptr<shader_t> shader);
			~cube_object ();

			cube_object (const cube_object &) = delete;
			cube_object & operator= (const cube_object &) = delete;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;
	};
}