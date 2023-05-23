#include "surface.hpp"
#include "gui.hpp"

namespace mini {
	const std::vector<point_ptr> & bicubic_surface::t_get_points () const {
		return m_points;
	}

	bool bicubic_surface::is_showing_polygon () const {
		return m_show_polygon;
	}

	void bicubic_surface::set_showing_polygon (bool show) {
		m_show_polygon = true;
	}

	unsigned int bicubic_surface::get_patches_x () const {
		return m_patches_x;
	}

	unsigned int bicubic_surface::get_patches_y () const {
		return m_patches_y;
	}

	unsigned int bicubic_surface::get_num_points () const {
		return m_points.size ();
	}

	unsigned int bicubic_surface::get_num_patches () const {
		return m_patches_x * m_patches_y;
	}

	int bicubic_surface::get_res_u () const {
		return m_res_u;
	}

	int bicubic_surface::get_res_v () const {
		return m_res_v;
	}

	void bicubic_surface::set_res_u (int u) {
		m_res_u = u;
	}

	void bicubic_surface::set_res_v (int v) {
		m_res_v = v;
	}

	bicubic_surface::bicubic_surface (
		const std::string & type_name, 
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> solid_shader,
		std::shared_ptr<shader_t> grid_shader, 
		unsigned int patches_x, 
		unsigned int patches_y, 
		const std::vector<point_ptr> & points) :
		scene_obj_t (scene, type_name, false, false, false), 
		m_patches_x (patches_x), 
		m_patches_y (patches_y) {

		// validity check
		if ((patches_y * patches_x * 9 + patches_x * 3 + patches_y * 3 + 1) != points.size ()) {
			throw std::runtime_error ("invalid input data for a bezier surface patch");
		}

		m_res_u = 25;
		m_res_v = 25;

		m_ready = false;
		m_use_solid = true;
		m_use_wireframe = true;
		m_queued_update = false;
		m_signals_setup = false;

		m_vao = 0;
		m_pos_buffer = m_index_buffer = 0;
		m_color = { 1.0f, 1.0f, 1.0f, 1.0f };

		m_shader = shader;
		m_solid_shader = solid_shader;
		m_grid_shader = grid_shader;

		t_set_handler (signal_event_t::moved, std::bind (&bicubic_surface::m_moved_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		m_points.reserve (points.size ());
		for (const auto & point : points) {
			point->set_deletable (false);
			m_points.push_back (point);
		}

		// create topology
		m_rebuild_buffers (true);
	}

	bicubic_surface::bicubic_surface (
		const std::string & type_name, 
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> solid_shader,
		std::shared_ptr<shader_t> grid_shader, 
		unsigned int patches_x, 
		unsigned int patches_y,
		const std::vector<point_ptr> & points, 
		const std::vector<GLuint> topology) :
		scene_obj_t (scene, type_name, false, false, false), 
		m_patches_x (patches_x), 
		m_patches_y (patches_y) {

		// topology validity check
		if ((patches_x * patches_y * num_control_points) != topology.size ()) {
			throw std::runtime_error ("invalid topology data for a bezier surface patch");
		}

		m_res_u = 25;
		m_res_v = 25;

		m_ready = false;
		m_use_solid = true;
		m_use_wireframe = true;
		m_queued_update = false;
		m_signals_setup = false;

		m_vao = 0;
		m_pos_buffer = m_index_buffer = 0;
		m_color = { 1.0f, 1.0f, 1.0f, 1.0f };

		m_shader = shader;
		m_solid_shader = solid_shader;
		m_grid_shader = grid_shader;

		t_set_handler (signal_event_t::moved, std::bind (&bicubic_surface::m_moved_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		m_points.reserve (points.size ());
		for (const auto & point : points) {
			point->set_deletable (false);
			m_points.push_back (point);
		}

		// do not create topology automatically
		m_indices = topology;
		m_rebuild_buffers (false);
	}

	bicubic_surface::~bicubic_surface () {
		for (const auto & point : m_points) {
			point->set_deletable (true);
		}

		m_destroy_buffers ();
	}

	void bicubic_surface::configure () {
		if (ImGui::CollapsingHeader ("Surface Settings")) {
			gui::prefix_label ("Show Polygon: ", 250.0f);
			ImGui::Checkbox ("##surf_show_polygon", &m_show_polygon);

			gui::prefix_label ("Show Solid: ", 250.0f);
			ImGui::Checkbox ("##surf_show_solid", &m_use_solid);

			gui::prefix_label ("Show Wireframe: ", 250.0f);
			ImGui::Checkbox ("##surf_show_frame", &m_use_wireframe);

			gui::prefix_label ("Draw Res. U: ", 250.0f);
			ImGui::InputInt ("##surf_res_u", &m_res_u);

			gui::prefix_label ("Draw Res. V: ", 250.0f);
			ImGui::InputInt ("##surf_res_v", &m_res_v);

			gui::prefix_label ("Color: ", 250.0f);
			gui::color_editor ("##surf_Color", m_color);
			ImGui::NewLine ();
		}

		gui::clamp (m_res_u, 4, 64);
		gui::clamp (m_res_v, 4, 64);
	}

	void bicubic_surface::integrate (float delta_time) {
		if (!m_signals_setup) {
			m_setup_signals ();
		}

		if (m_queued_update) {
			m_update_buffers ();
			m_queued_update = false;
		}
	}

	void bicubic_surface::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_ready) {
			glBindVertexArray (m_vao);

			if (m_use_solid) {
				const auto & view_matrix = context.get_view_matrix ();
				const auto & proj_matrix = context.get_projection_matrix ();

				if (m_use_wireframe) {
					glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
				}

				m_bind_shader (context, *m_solid_shader.get (), world_matrix);

				// first render pass - u,v
				m_solid_shader->set_uniform_uint ("u_resolution_v", static_cast<GLuint> (m_res_v));
				m_solid_shader->set_uniform_uint ("u_resolution_u", static_cast<GLuint> (m_res_u));

				glPatchParameteri (GL_PATCH_VERTICES, 16);
				glDrawElements (GL_PATCHES, m_indices.size (), GL_UNSIGNED_INT, 0);

				glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
			} else {
				m_bind_shader (context, *m_shader.get (), world_matrix);

				// first render pass - u,v
				m_shader->set_uniform_int ("u_vertical", true);
				m_shader->set_uniform_uint ("u_resolution_v", static_cast<GLuint> (m_res_v));
				m_shader->set_uniform_uint ("u_resolution_u", static_cast<GLuint> (m_res_u));

				glPatchParameteri (GL_PATCH_VERTICES, 16);
				glDrawElements (GL_PATCHES, m_indices.size (), GL_UNSIGNED_INT, 0);

				// second render pass = v,u
				m_shader->set_uniform_int ("u_vertical", false);
				m_shader->set_uniform_uint ("u_resolution_v", static_cast<GLuint> (m_res_u));
				m_shader->set_uniform_uint ("u_resolution_u", static_cast<GLuint> (m_res_v));

				glPatchParameteri (GL_PATCH_VERTICES, 16);
				glDrawElements (GL_PATCHES, m_indices.size (), GL_UNSIGNED_INT, 0);
			}

			if (m_show_polygon) {
				glBindVertexArray (m_grid_vao);
				m_bind_shader (context, *m_grid_shader, world_matrix);

				glDrawElements (GL_LINES, m_grid_indices.size (), GL_UNSIGNED_INT, 0);
			}

			glUseProgram (0);
			glBindVertexArray (0);
		}
	}

	std::vector<uint64_t> bicubic_surface::serialize_points () {
		std::vector<uint64_t> serialized;
		serialized.reserve (m_points.size ());

		for (const auto & point : m_points) {
			serialized.push_back (point->get_id ());
		}

		return serialized;
	}

	std::vector<bicubic_surface::serialized_patch> bicubic_surface::serialize_patches () {
		std::vector<serialized_patch> patches;
		patches.reserve (get_num_patches ());

		for (unsigned int i = 0; i < m_indices.size (); ) {
			serialized_patch patch;

			for (int j = 0; j < num_control_points; ++j, ++i) {
				auto index = m_indices[i];
				patch[j] = m_points[index]->get_id ();
			}

			patches.push_back (patch);
		}

		return patches;
	}

	constexpr GLuint a_position = 0;

	void bicubic_surface::m_bind_shader (app_context & context, shader_t & shader, const glm::mat4x4 & world_matrix) const {
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
		shader.set_uniform ("u_color", m_color);
	}

	void bicubic_surface::m_rebuild_buffers (bool recalculate_indices) {
		m_ready = false;
		m_destroy_buffers ();

		// allocate new buffers
		m_positions.clear ();
		m_positions.resize (get_num_points () * 3);

		if (!m_calc_pos_buffer ()) {
			m_ready = false;
			return;
		}

		if (recalculate_indices) {
			m_indices.clear ();
			m_indices.resize (get_num_patches () * num_control_points);
			m_calc_idx_buffer ();
		}

		// put data into buffers
		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_pos_buffer);
		glGenBuffers (1, &m_index_buffer);

		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_index_buffer);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * m_indices.size (), m_indices.data (), GL_STATIC_DRAW);

		glBindVertexArray (0);

		glGenVertexArrays (1, &m_grid_vao);
		glGenBuffers (1, &m_grid_index_buffer);

		glBindVertexArray (m_grid_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, m_grid_index_buffer);
		glBufferData (GL_ELEMENT_ARRAY_BUFFER, sizeof (GLuint) * m_grid_indices.size (), m_grid_indices.data (), GL_STATIC_DRAW);

		glBindVertexArray (0);

		m_ready = true;
	}

	bool bicubic_surface::m_calc_pos_buffer () {
		// copy positions from points into buffer data
		for (unsigned int index = 0; index < m_points.size (); ++index) {
			auto & point = m_points[index];

			if (point) {
				const auto & pos = point->get_translation ();
				m_positions[3 * index + 0] = pos.x;
				m_positions[3 * index + 1] = pos.y;
				m_positions[3 * index + 2] = pos.z;
			} else {
				return false;
			}
		}

		return true;
	}

	void bicubic_surface::m_calc_idx_buffer () {
		// create indices for patches
		unsigned int i = 0;
		unsigned int width = m_patches_x * 3 + 1;
		unsigned int height = m_patches_y * 3 + 1;

		for (unsigned int py = 0; py < m_patches_y; ++py) {
			for (unsigned int px = 0; px < m_patches_x; ++px) {
				// add all control points to the patch
				unsigned int bx = px * 3;
				unsigned int by = py * 3;

				for (unsigned int y = 0; y < 4; ++y) {
					for (unsigned int x = 0; x < 4; ++x) {
						unsigned int cx = bx + x;
						unsigned int cy = by + y;

						unsigned int index = (cy * width) + cx;
						m_indices[i++] = index;
					}
				}
			}
		}

		// for the grid we will need a different procedure
		m_grid_indices.resize (2 * (((width - 1) * height) + (width * (height - 1))));
		i = 0;

		for (unsigned int cy = 0; cy < height; ++cy) {
			unsigned int i1, i2;

			for (unsigned int cx = 0; cx < width - 1; ++cx) {
				i1 = (cy * width) + cx;
				i2 = (cy * width) + cx + 1;

				m_grid_indices[i++] = i1;
				m_grid_indices[i++] = i2;
			}

			if (cy == height - 1) {
				break;
			}

			for (unsigned int cx = 0; cx < width; ++cx) {
				i1 = (cy * width) + cx;
				i2 = ((cy + 1) * width) + cx;

				m_grid_indices[i++] = i1;
				m_grid_indices[i++] = i2;
			}
		}
	}

	void bicubic_surface::m_update_buffers () {
		if (!m_calc_pos_buffer ()) {
			m_ready = false;
			return;
		}

		glBindVertexArray (m_vao);
		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferSubData (GL_ARRAY_BUFFER, 0, m_positions.size () * sizeof (float), m_positions.data ());
		glBindBuffer (GL_ARRAY_BUFFER, 0);
		glBindVertexArray (0);
	}

	void bicubic_surface::m_destroy_buffers () {
		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
			m_vao = 0;
		}

		if (m_pos_buffer) {
			glDeleteBuffers (1, &m_pos_buffer);
			m_pos_buffer = 0;
		}

		if (m_index_buffer) {
			glDeleteBuffers (1, &m_index_buffer);
			m_index_buffer = 0;
		}

		m_ready = false;
	}

	void bicubic_surface::m_moved_sighandler (signal_event_t sig, scene_obj_t & sender) {
		m_queued_update = true;
	}

	void bicubic_surface::m_setup_signals () {
		for (auto & point : m_points) {
			t_listen (signal_event_t::moved, *point);
		}

		m_signals_setup = true;
	}
}