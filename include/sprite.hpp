#pragma once
#include <glad/glad.h>

#include "context.hpp"
#include "texture.hpp"

namespace mini {
	class sprite : public graphics_obj_t {
		private:
			GLuint m_pos_buffer, m_color_buffer, m_index_buffer, m_uv_buffer, m_vao;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<texture_t> m_texture;

			glm::vec2 m_size;
			glm::vec4 m_color_tint;

		public:
			const glm::vec2 & get_size () const;
			const glm::vec4 & get_color_tint () const;

			void set_size (const glm::vec2 & size);
			void set_color_tint (const glm::vec4 & color_tint);

			sprite (std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture);
			~sprite ();

			sprite (const sprite &) = delete;
			sprite & operator= (const sprite &) = delete;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
	};
}