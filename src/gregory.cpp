#include "gregory.hpp"

namespace mini {
	patch_offset_t operator+(const patch_offset_t & a, const patch_offset_t & b) {
		return { a.x + b.x, a.y + b.y };
	}

	patch_offset_t operator-(const patch_offset_t & a, const patch_offset_t & b) {
		return { a.x - b.x, a.y - b.y };
	}

	bool operator==(const patch_offset_t & a, const patch_offset_t & b) {
		return (a.x == b.x && a.y == b.y);
	}

	bool operator!=(const patch_offset_t & a, const patch_offset_t & b) {
		return (a.x != b.x || a.y != b.y);
	}

	gregory_surface::gregory_surface (
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> line_shader,
		std::shared_ptr<shader_t> bezier_shader,
		const bicubic_surface::surface_patch & patch1, 
		const bicubic_surface::surface_patch & patch2, 
		const bicubic_surface::surface_patch & patch3,
		const patch_indexing_t & index1,
		const patch_indexing_t & index2,
		const patch_indexing_t & index3) :
		scene_obj_t (scene, "gregory_patch", false, false, false) {

		m_solid_shader = shader;
		m_line_shader = line_shader;
		m_patch1 = patch1;
		m_patch2 = patch2;
		m_patch3 = patch3;

		m_index1 = index1;
		m_index2 = index2;
		m_index3 = index3;

		m_res_u = m_res_v = 16;

		// reset opengl buffers to null
		m_line_vao = m_line_buffer = 0;
		m_ready = false;

		m_calculate_points ();
		m_initialize_buffers ();
	}

	constexpr int at (int x, int y) {
		return (y * 4) + x;
	};

	gregory_surface::~gregory_surface () {
		m_destroy_buffers ();
	}

	void gregory_surface::configure () {
	}

	void gregory_surface::integrate (float delta_time) {
		
	}

	void gregory_surface::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_ready) {
			glBindVertexArray (m_vao);

			//if (m_use_wireframe) {
				glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
			//}

			m_bind_shader (context, *m_solid_shader.get (), world_matrix);

			// first render pass - u,v
			m_solid_shader->set_uniform_uint ("u_resolution_v", static_cast<GLuint> (m_res_v));
			m_solid_shader->set_uniform_uint ("u_resolution_u", static_cast<GLuint> (m_res_u));

			glPatchParameteri (GL_PATCH_VERTICES, 20);
			glDrawArrays (GL_PATCHES, 0, m_positions.size ());

			glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

			// draw control points
			glBindVertexArray (m_line_vao);
			m_bind_shader (context, *m_line_shader, world_matrix);
			m_line_shader->set_uniform ("u_color", glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });
			glDrawArrays (GL_LINES, 0, m_line_positions.size ());
			glBindVertexArray (0);
		}
	}

	constexpr GLuint a_position = 0;

	void gregory_surface::m_initialize_buffers () {
		glCreateVertexArrays (1, &m_vao);
		glCreateBuffers (1, &m_pos_buffer);

		glBindVertexArray (m_vao);
		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		// control lines

		glCreateVertexArrays (1, &m_line_vao);
		glCreateBuffers (1, &m_line_buffer);

		glBindVertexArray (m_line_vao);
		glBindBuffer (GL_ARRAY_BUFFER, m_line_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_line_positions.size (), reinterpret_cast<void *> (m_line_positions.data ()), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);

		m_ready = true;
	}

	void gregory_surface::m_destroy_buffers () {
		m_ready = false;

		if (m_pos_buffer) {
			glDeleteBuffers (1, &m_pos_buffer);
		}

		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
		}

		if (m_line_buffer) {
			glDeleteBuffers (1, &m_line_buffer);
		}

		if (m_line_vao) {
			glDeleteVertexArrays (1, &m_line_vao);
		}

		m_line_vao = m_line_buffer = 0;
	}

	void gregory_surface::m_bind_shader (app_context & context, shader_t & shader, const glm::mat4x4 & world_matrix) const {
		shader.bind ();

		const auto & view_matrix = context.get_view_matrix ();
		const auto & proj_matrix = context.get_projection_matrix ();

		const auto & video_mode = context.get_video_mode ();

		glm::vec2 resolution = {
			static_cast<float> (video_mode.get_buffer_width ()),
			static_cast<float> (video_mode.get_buffer_height ())
		};

		shader.set_uniform ("u_world", world_matrix);
		shader.set_uniform ("u_view", view_matrix);
		shader.set_uniform ("u_projection", proj_matrix);
		shader.set_uniform ("u_resolution", resolution);
		shader.set_uniform ("u_line_width", 2.0f);
		shader.set_uniform ("u_color", glm::vec4 {1.0f, 0.0f, 0.0f, 1.0f});
	}

	void gregory_surface::m_calculate_points () {
		std::array<std::array<quarter_surface, 2>, 3> patch;

		// subdivide each adjacent patch in two
		// then we will use those for algorithm from the lecture
		m_calculate_adjacent_surf (patch[0][0], patch[0][1], m_patch1, m_index1);
		m_calculate_adjacent_surf (patch[1][0], patch[1][1], m_patch2, m_index2);
		m_calculate_adjacent_surf (patch[2][0], patch[2][1], m_patch3, m_index3);

		// calculate the control points on the edges
		std::array<glm::vec3, 3> A, B, D, E, Q;
		glm::vec3 C;

		for (int i = 0; i < 3; ++i) {
			A[i] = patch[i][1][at (0,0)];
			B[i] = patch[i][1][at (0,1)];
			
			// obtained from C1 continuity
			D[i] = A[i] + (A[i] - B[i]);
			Q[i] = (3.0f * D[i] - A[i]) / 2.0f;
		}

		C = (Q[0] + Q[1] + Q[2]) / 3.0f;
		for (int i = 0; i < 3; ++i) {
			E[i] = (2.0f * Q[i] + C) / 3.0f;
		}

		m_line_positions.clear ();
		m_positions.clear ();

		// macros
		auto add_line = [this](const std::initializer_list<glm::vec3> & p) constexpr -> void {
			auto first = p.begin(), second = p.begin() + 1;

			for (;second != p.end ();first++,second++) {
				const auto &a = *first;
				const auto &b = *second;

				m_line_positions.push_back (a[0]);
				m_line_positions.push_back (a[1]);
				m_line_positions.push_back (a[2]);
				m_line_positions.push_back (b[0]);
				m_line_positions.push_back (b[1]);
				m_line_positions.push_back (b[2]);
			}
		};

		auto add_positions = [this](const std::initializer_list<glm::vec3> & p) constexpr -> void {
			for (auto iter = p.begin (); iter != p.end (); ++iter) {
				m_positions.insert (m_positions.end (), { iter->x, iter->y, iter->z });
			}
		};

		for (int i = 0; i < 3; ++i) {
			int j = (i + 1) % 3;
			const auto & patch1 = patch[i][1];
			const auto & patch2 = patch[j][0];

			glm::vec3 p0 = patch1[at (0, 0)];
			glm::vec3 p1 = patch2[at (0, 0)];
			glm::vec3 p2 = patch2[at (3, 0)];
			glm::vec3 p3 = C;

			glm::vec3 e00 = D[i];
			glm::vec3 e01 = patch1[at (1, 0)];
			glm::vec3 e10 = patch1[at (2, 0)];
			glm::vec3 e11 = patch2[at (1, 0)];
			glm::vec3 e20 = patch2[at (2, 0)];
			glm::vec3 e21 = D[j];
			glm::vec3 e30 = E[j];
			glm::vec3 e31 = E[i];

			glm::vec3 f01 = patch1[at (1, 0)] + (patch1[at (1, 0)] - patch1[at (1, 1)]);
			glm::vec3 f00 = f01;
			glm::vec3 f10 = patch1[at (2, 0)] + (patch1[at (2, 0)] - patch1[at (2, 1)]);
			glm::vec3 f11 = patch2[at (1, 0)] + (patch2[at (1, 0)] - patch2[at (1, 1)]);
			glm::vec3 f20 = patch2[at (2, 0)] + (patch2[at (2, 0)] - patch2[at (2, 1)]);
			glm::vec3 f21 = f20;
			glm::vec3 f30 = p3 + ((e31 - p3) + (e30 - p3));
			glm::vec3 f31 = f30;

			add_line ({ p0, e01, e10, p1, e11, e20, p2, e21, e30, p3, e31, e00, p0 });

			add_line ({ e00, f00 });
			add_line ({ e01, f01 });
			add_line ({ e10, f10 });
			add_line ({ e11, f11 });
			add_line ({ e20, f20 });
			add_line ({ e21, f21 });
			add_line ({ e30, f30 });
			add_line ({ e31, f31 });

			add_positions ({ p0, p1, p2, p3, e00, e01, e10, e11, e20, e21, e30, e31, f00, f01, f10, f11, f20, f21, f30, f31 });
		}
	}

	void gregory_surface::m_calculate_adjacent_surf (
		quarter_surface & surface1,
		quarter_surface & surface2,
		const bicubic_surface::surface_patch & patch, 
		const patch_indexing_t idx) {

		constexpr std::array<std::pair<patch_offset_t, patch_offset_t>, 4> clockwise = {
			std::pair<patch_offset_t, patch_offset_t> {{0, 0}, {3, 0}}, {{3, 0}, {3, 3}}, {{3, 3}, {0, 3}}, {{0, 3}, {0, 0}}
		};

		// first reindex the surface so that it is coherent
		// we need to reindex as patches could have been merged on any two sides
		// so we don't really know which way u and v go
		patch_offset_t u_dir = idx.end - idx.start;
		patch_offset_t v_dir;

		bool is_clockwise = false;
		for (const auto & kv : clockwise) {
			if (kv.first == idx.start && kv.second == idx.end) {
				is_clockwise = true;
				break;
			}
		}

		if (is_clockwise) {
			v_dir = { -u_dir.y, u_dir.x };
		} else {
			v_dir = { u_dir.y, -u_dir.x };
		}

		u_dir.x /= 3;
		u_dir.y /= 3;
		v_dir.x /= 3;
		v_dir.y /= 3;

		patch_offset_t u_offset = idx.start, v_offset;
		quarter_surface surface;

		std::cout << std::endl;
		for (int u = 0; u < 4; ++u) {
			v_offset = u_offset;

			for (int v = 0; v < 4; ++v) {
				surface[at (u,v)] = patch.points[v_offset.x][v_offset.y]->get_translation ();
				std::cout << patch.points[v_offset.x][v_offset.y]->get_name () << " ";

				v_offset = v_offset + v_dir;
			}

			u_offset = u_offset + u_dir;
			std::cout << std::endl;
		}
		std::cout << std::endl;

		// now the easy part, just divide the surface in two using decasteljeu algorithm
		for (int v = 0; v < 4; ++v) {
			glm::vec3 b00 = surface[at (0, v)];
			glm::vec3 b01 = surface[at (1, v)];
			glm::vec3 b02 = surface[at (2, v)];
			glm::vec3 b03 = surface[at (3, v)];

			glm::vec3 b10 = (b00 + b01) / 2.0f;
			glm::vec3 b11 = (b01 + b02) / 2.0f;
			glm::vec3 b12 = (b02 + b03) / 2.0f;

			glm::vec3 b20 = (b10 + b11) / 2.0f;
			glm::vec3 b21 = (b11 + b12) / 2.0f;

			glm::vec3 b30 = (b20 + b21) / 2.0f;

			surface1[at (0, v)] = b00;
			surface1[at (1, v)] = b10;
			surface1[at (2, v)] = b20;
			surface1[at (3, v)] = b30;

			surface2[at (0, v)] = b30;
			surface2[at (1, v)] = b21;
			surface2[at (2, v)] = b12;
			surface2[at (3, v)] = b03;
		}
	}
}