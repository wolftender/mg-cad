#include "cube.hpp"

namespace mini {
	constexpr std::array<float, 72> cube_positions = { 
		 1,  1, -1, 
		-1,  1, -1, 
		-1,  1,  1, 
		 1,  1,  1, 
		 1, -1,  1, 
		 1,  1,  1, 
		-1,  1,  1, 
		-1, -1,  1, 
		-1, -1,  1, 
		-1,  1,  1, 
		-1,  1, -1, 
		-1, -1, -1, 
		-1, -1, -1, 
		 1, -1, -1, 
		 1, -1,  1, 
		-1, -1,  1, 
		 1, -1, -1, 
		 1,  1, -1, 
		 1,  1,  1, 
		 1, -1,  1, 
		-1, -1, -1, 
		-1,  1, -1, 
		 1,  1, -1, 
		 1, -1, -1 
	};

	constexpr std::array<float, 96> cube_colors = {
		1, 0, 0, 1,
		0, 1, 0, 1, 
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1,
		1, 0, 0, 1,
		0, 1, 0, 1,
		0, 0, 1, 1
	};

	constexpr std::array<float, 72> cube_normals = { 
		 0,  1,  0, 
		 0,  1,  0, 
		 0,  1,  0, 
		 0,  1,  0, 
		 0,  0,  1, 
		 0,  0,  1, 
		 0,  0,  1, 
		 0,  0,  1, 
		-1,  0,  0, 
		-1,  0,  0, 
		-1,  0,  0, 
		-1,  0,  0, 
		 0, -1,  0, 
		 0, -1,  0, 
		 0, -1,  0, 
		 0, -1,  0, 
		 1,  0,  0, 
		 1,  0,  0, 
		 1,  0,  0,
		 1,  0,  0, 
		 0,  0, -1, 
		 0,  0, -1, 
		 0,  0, -1, 
		 0,  0, -1 
	};

	constexpr std::array<float, 48> cube_uv = { 
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1, 
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1, 
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1, 
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1, 
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1, 
		0, 0, 
		1, 0, 
		1, 1, 
		0, 1 
	};

	constexpr std::array<uint32_t, 36> cube_indices = { 
		0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11, 12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23 
	};

	cube_object::cube_object (std::shared_ptr<shader_t> shader) : scene_obj_t ("colored cube") {
		constexpr int num_vertices = 24;
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		m_shader = shader;
		m_pos_buffer = m_color_buffer = m_index_buffer = m_vao = 0;

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_pos_buffer);
		glGenBuffers (1, &m_color_buffer);
		glGenBuffers (1, &m_index_buffer);
		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * num_vertices * 3, cube_positions.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ARRAY_BUFFER, m_color_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * num_vertices * 4, cube_colors.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (a_color, 4, GL_FLOAT, false, sizeof (float) * 4, (void *)0);
		glEnableVertexAttribArray (a_color);

		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * cube_indices.size (), cube_indices.data (), GL_STATIC_DRAW);

		glBindVertexArray (static_cast<GLuint>(NULL));
	}

	cube_object::~cube_object () {
		glDeleteVertexArrays (1, &m_vao);
		glDeleteBuffers (1, &m_pos_buffer);
		glDeleteBuffers (1, &m_color_buffer);
		glDeleteBuffers (1, &m_index_buffer);
	}

	void cube_object::render (app_context & context, const float_matrix_t & world_matrix) const {
		glBindVertexArray (m_vao);

		m_shader->bind ();

		// set uniforms
		const auto& view_matrix = context.get_view_matrix ();
		const auto& proj_matrix = context.get_projection_matrix ();

		//const auto & view_matrix = make_identity ();
		//const auto& proj_matrix = make_identity ();

		m_shader->set_uniform ("u_world", world_matrix);
		m_shader->set_uniform ("u_view", view_matrix);
		m_shader->set_uniform ("u_projection", proj_matrix);

		glDrawElements (GL_TRIANGLES, cube_indices.size (), GL_UNSIGNED_INT, NULL);
		glBindVertexArray (static_cast<GLuint>(NULL));
	}

	void cube_object::configure () {
		// basic configuration properties
		scene_obj_t::configure ();
	}
}
