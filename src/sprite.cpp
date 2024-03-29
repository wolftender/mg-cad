#include "sprite.hpp"

namespace mini {
	constexpr std::array<float, 12> sprite_vertices = {
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
	};

	constexpr std::array<float, 8> sprite_texcoords = {
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0,
		0.0, 1.0
	};

	constexpr std::array<float, 16> sprite_colors = {
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 1.0f
	};

	constexpr std::array<GLuint, 6> sprite_indices = {
		0, 1, 3,
		1, 2, 3
	};

	const glm::vec2 & sprite::get_size () const {
		return m_size;
	}

	const glm::vec4 & sprite::get_color_tint () const {
		return m_color_tint;
	}

	void sprite::set_size (const glm::vec2 & size) {
		m_size = size;
	}

	void sprite::set_color_tint (const glm::vec4 & color_tint) {
		m_color_tint = color_tint;
	}

	sprite::sprite (std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture) {
		m_shader = shader;
		m_texture = texture;

		constexpr int num_vertices = static_cast<int> (sprite_vertices.size ()) / 3;
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;
		constexpr GLuint a_uv = 2;

		m_color_tint = { 1.0f, 1.0f, 1.0f, 1.0f };
		m_size = { 1.0f, 1.0f };
		m_shader = shader;
		m_pos_buffer = m_color_buffer = m_index_buffer = m_uv_buffer = m_vao = 0;

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_pos_buffer);
		glGenBuffers (1, &m_color_buffer);
		glGenBuffers (1, &m_index_buffer);
		glGenBuffers (1, &m_uv_buffer);
		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * num_vertices * 3, sprite_vertices.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ARRAY_BUFFER, m_color_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * num_vertices * 4, sprite_colors.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (a_color, 4, GL_FLOAT, false, sizeof (float) * 4, (void *)0);
		glEnableVertexAttribArray (a_color);

		glBindBuffer (GL_ARRAY_BUFFER, m_uv_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * num_vertices * 2, sprite_texcoords.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (a_uv, 2, GL_FLOAT, false, sizeof (float) * 2, (void *)0);
		glEnableVertexAttribArray (a_uv);

		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * sprite_indices.size (), sprite_indices.data (), GL_STATIC_DRAW);

		glBindVertexArray (0);
	}

	sprite::~sprite () {
		glDeleteVertexArrays (1, &m_vao);
		glDeleteBuffers (1, &m_pos_buffer);
		glDeleteBuffers (1, &m_color_buffer);
		glDeleteBuffers (1, &m_index_buffer);
		glDeleteBuffers (1, &m_uv_buffer);
	}

	void sprite::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		glBindVertexArray (m_vao);
		glDisable (GL_DEPTH_TEST);
		glEnable (GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (m_texture) {
			m_texture->bind ();
		}

		m_shader->bind ();

		// set uniforms
		const auto & view_matrix = context.get_view_matrix ();
		const auto & proj_matrix = context.get_projection_matrix ();

		float screen_width = static_cast<float> (context.get_video_mode ().get_buffer_width ());
		float screen_height = static_cast<float> (context.get_video_mode ().get_buffer_height ());

		m_shader->set_uniform ("u_size", m_size);
		m_shader->set_uniform ("u_color", m_color_tint);
		m_shader->set_uniform ("u_world", world_matrix);
		m_shader->set_uniform ("u_resolution", glm::vec2 (screen_width, screen_height));

		glDrawElements (GL_TRIANGLES, static_cast<GLsizei> (quad_indices.size ()), GL_UNSIGNED_INT, NULL);
		glBindVertexArray (static_cast<GLuint>(NULL));
		glEnable (GL_DEPTH_TEST);
		glDisable (GL_BLEND);
	}
}