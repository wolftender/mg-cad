#pragma once
#include <glad/glad.h>

#include "context.hpp"
#include "object.hpp"
#include "texture.hpp"

namespace mini {
	class billboard_object : public graphics_obj_t {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_index_buffer, m_uv_buffer, m_vao;

			std::shared_ptr<texture_t> m_texture;
			std::shared_ptr<shader_t> m_shader;

		public:
			billboard_object (std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture);
			~billboard_object ();

			billboard_object (const billboard_object &) = delete;
			billboard_object & operator= (const billboard_object &) = delete;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
	};
}