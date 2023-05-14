#include "beziersurf.hpp"

namespace mini {
	const std::vector<point_wptr> & bezier_patch_c0::t_get_points () const {
		return m_points;
	}

	bool bezier_patch_c0::is_showing_polygon () const {
		return m_show_polygon;
	}

	void bezier_patch_c0::set_showing_polygon (bool show) {
		m_show_polygon = true;
	}

	unsigned int bezier_patch_c0::get_patches_x () const {
		return m_patches_x;
	}

	unsigned int bezier_patch_c0::get_patches_y () const {
		return m_patches_y;
	}

	unsigned int bezier_patch_c0::get_num_points () const {
		return m_points.size ();
	}

	unsigned int bezier_patch_c0::get_num_patches () const {
		return m_patches_x * m_patches_y;
	}

	bezier_patch_c0::bezier_patch_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> grid_shader,
		unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points) :
		scene_obj_t (scene, "bezier_surf_c0", false, false, false), m_patches_x (patches_x), m_patches_y (patches_y) {

		// validity check
		assert ((patches_y * patches_x * 9 + patches_x * 3 + patches_y * 3 + 1) == points.size ());

		m_vao = 0;
		m_pos_buffer = m_index_buffer = 0;

		m_shader = shader;
		m_grid_shader = grid_shader;

		m_points.reserve (points.size ());
		for (const auto & point : points) {
			m_points.push_back (point);
		}

		m_rebuild_buffers ();
	}

	bezier_patch_c0::~bezier_patch_c0 () {
		m_destroy_buffers ();
	}

	void bezier_patch_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		glBindVertexArray (m_vao);
		m_bind_shader (context, *m_shader.get (), world_matrix);

		m_shader->set_uniform_sampler ("u_vertical", true);
		glPatchParameteri (GL_PATCH_VERTICES, 16);
		glDrawElements (GL_PATCHES, m_indices.size (), GL_UNSIGNED_INT, 0);

		m_shader->set_uniform_sampler ("u_vertical", false);
		glPatchParameteri (GL_PATCH_VERTICES, 16);
		glDrawElements (GL_PATCHES, m_indices.size (), GL_UNSIGNED_INT, 0);

		glUseProgram (0);
		glBindVertexArray (0);
	}

	constexpr GLuint a_position = 0;

	void bezier_patch_c0::m_bind_shader (app_context & context, shader_t & shader, const glm::mat4x4 & world_matrix) const {
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
	}

	void bezier_patch_c0::m_rebuild_buffers () {
		m_destroy_buffers ();

		// allocate new buffers
		m_positions.clear ();
		m_indices.clear ();

		m_positions.resize (get_num_points () * 3);
		m_indices.resize (get_num_patches () * num_control_points);

		m_calc_pos_buffer ();
		m_calc_idx_buffer ();

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
	}

	void bezier_patch_c0::m_calc_pos_buffer () {
		// copy positions from points into buffer data
		for (unsigned int index = 0; index < m_points.size (); ++index) {
			auto point = m_points[index].lock ();
			assert (point);

			if (point) {
				const auto & pos = point->get_translation ();
				m_positions[3 * index + 0] = pos.x;
				m_positions[3 * index + 1] = pos.y;
				m_positions[3 * index + 2] = pos.z;
			}
		}
	}

	void bezier_patch_c0::m_calc_idx_buffer () {
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
	}

	void bezier_patch_c0::m_update_buffers () {
		m_calc_pos_buffer ();
	}

	void bezier_patch_c0::m_destroy_buffers () {
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
	}

	bezier_patch_c0_template::bezier_patch_c0_template (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> grid_shader, 
		std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, unsigned int patches_x, unsigned int patches_y) : 
		scene_obj_t (scene, "bezier_surf_c0", true, true, true) {

		m_shader = shader;
		m_grid_shader = grid_shader;
		m_point_shader = point_shader;
		m_point_texture = point_texture;

		m_patches_x = patches_x;
		m_patches_y = patches_y;

		m_rebuild_surface ();
	}

	void bezier_patch_c0_template::configure () {
	}

	void bezier_patch_c0_template::integrate (float delta_time) {
	}

	void bezier_patch_c0_template::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		for (const auto & point : m_points) {
			point->render (context, point->get_matrix ());
		}

		if (m_patch) {
			m_patch->render (context, world_matrix);
		}
	}

	void bezier_patch_c0_template::m_rebuild_surface () {
		m_points.clear ();

		// common endpoints, so each patch "owns" 3 points
		// its first point is also the last point of the previous patch
		unsigned int points_x = (m_patches_x * 3) + 1;
		unsigned int points_y = (m_patches_y * 3) + 1;

		m_patch.reset (nullptr);
		m_points.clear ();

		m_points.resize (points_x * points_y);

		constexpr float spacing = 0.75f;

		const auto & center = get_translation ();
		const glm::vec3 pos{
			center.x - (static_cast<float>(points_x - 1) * spacing / 2.0f),
			center.y,
			center.z - (static_cast<float>(points_y - 1) * spacing / 2.0f)
		};

		for (unsigned int x = 0; x < points_x; ++x) {
			for (unsigned int y = 0; y < points_y; ++y) {
				unsigned int index = (y * points_x) + x;

				m_points[index] = std::make_shared<point_object> (get_scene (), m_point_shader, m_point_texture);


				float tx = (static_cast<float> (x) * spacing) + pos.x;
				float tz = (static_cast<float> (y) * spacing) + pos.z;

				float h = (tx - center.x) / (pos.x - center.x);
				float ty = (h * h) - 1.0f;

				m_points[index]->set_translation ({ tx, ty, tz });
				m_points[index]->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				m_points[index]->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
			}
		}

		m_patch = std::make_unique<bezier_patch_c0> (get_scene (), m_shader, m_grid_shader, m_patches_x, m_patches_y, m_points);
	}
}