#include "beziersurf.hpp"
#include "gui.hpp"

namespace mini {
	const std::vector<point_ptr> & bezier_patch_c0::t_get_points () const {
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

	bezier_patch_c0::bezier_patch_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points) :
		scene_obj_t (scene, "bezier_surf_c0", false, false, false), m_patches_x (patches_x), m_patches_y (patches_y) {

		// validity check
		if ((patches_y * patches_x * 9 + patches_x * 3 + patches_y * 3 + 1) != points.size ()) {
			throw std::runtime_error ("invalid input data for a bezier surface patch");
		}

		m_res_u = 25;
		m_res_v = 25;

		m_ready = false;
		m_use_solid = false;
		m_use_wireframe = true;
		m_queued_update = false;
		m_signals_setup = false;

		m_vao = 0;
		m_pos_buffer = m_index_buffer = 0;

		m_shader = shader;
		m_solid_shader = solid_shader;
		m_grid_shader = grid_shader;

		t_set_handler (signal_event_t::moved, std::bind (&bezier_patch_c0::m_moved_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		m_points.reserve (points.size ());
		for (const auto & point : points) {
			point->set_deletable (false);
			m_points.push_back (point);
		}

		m_rebuild_buffers ();
	}

	bezier_patch_c0::~bezier_patch_c0 () {
		for (const auto & point : m_points) {
			point->set_deletable (true);
		}

		m_destroy_buffers ();
	}

	void bezier_patch_c0::configure () {
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
			ImGui::NewLine ();
		}

		gui::clamp (m_res_u, 4, 64);
		gui::clamp (m_res_v, 4, 64);
	}

	void bezier_patch_c0::integrate (float delta_time) {
		if (!m_signals_setup) {
			m_setup_signals ();
		}

		if (m_queued_update) {
			m_update_buffers ();
			m_queued_update = false;
		}
	}

	void bezier_patch_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_ready) {
			glBindVertexArray (m_vao);
			
			if (m_use_solid) {
				const auto & view_matrix = context.get_view_matrix ();
				const auto & proj_matrix = context.get_projection_matrix ();

				if (m_use_wireframe) {
					glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
				}

				m_solid_shader->bind ();
				m_solid_shader->set_uniform ("u_world", world_matrix);
				m_solid_shader->set_uniform ("u_view", view_matrix);
				m_solid_shader->set_uniform ("u_projection", proj_matrix);

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

			glUseProgram (0);
			glBindVertexArray (0);
		}
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
		m_ready = false;
		m_destroy_buffers ();

		// allocate new buffers
		m_positions.clear ();
		m_indices.clear ();

		m_positions.resize (get_num_points () * 3);
		m_indices.resize (get_num_patches () * num_control_points);

		if (!m_calc_pos_buffer ()) {
			m_ready = false;
			return;
		}

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
		m_ready = true;
	}

	bool bezier_patch_c0::m_calc_pos_buffer () {
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

		m_ready = false;
	}

	void bezier_patch_c0::m_moved_sighandler (signal_event_t sig, scene_obj_t & sender) {
		m_queued_update = true;
	}

	void bezier_patch_c0::m_setup_signals () {
		for (auto & point : m_points) {
			t_listen (signal_event_t::moved, *point);
		}

		m_signals_setup = true;
	}


	//////////////////////////////////////////////////////////


	bezier_patch_c0_template::bezier_patch_c0_template (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, 
		unsigned int patches_x, unsigned int patches_y) : 
		scene_obj_t (scene, "bezier_surf_c0", true, true, true) {

		m_shader = shader;
		m_solid_shader = solid_shader;
		m_grid_shader = grid_shader;
		m_point_shader = point_shader;
		m_point_texture = point_texture;

		m_patches_x = patches_x;
		m_patches_y = patches_y;
		m_rebuild = false;

		m_combo_item = 0;
		m_rebuild_surface (build_mode_t::mode_default);
	}

	void bezier_patch_c0_template::configure () {
		if (ImGui::CollapsingHeader ("Builder Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			constexpr const char * items[] = { "default", "cylinder", "cowboy" };
			gui::prefix_label ("Shape: ", 250.0f);
		
			if (ImGui::Combo ("##surf_builder_shape", &m_combo_item, items, 3)) {
				build_mode_t new_mode;
				switch (m_combo_item) {
					case 1: new_mode = build_mode_t::mode_cylinder; break;
					case 2: new_mode = build_mode_t::mode_hat; break;
					default: new_mode = build_mode_t::mode_default; break;
				}

				if (new_mode != m_build_mode) {
					m_rebuild = true;
					m_build_mode = new_mode;
				}
			}

			gui::prefix_label ("Patches X: ", 250.0f);
			if (ImGui::InputInt ("##surf_patches_x", &m_patches_x)) {
				m_rebuild = true;
			}

			gui::prefix_label ("Patches Y: ", 250.0f);
			if (ImGui::InputInt ("##surf_patches_y", &m_patches_y)) {
				m_rebuild = true;
			}

			if (m_build_mode == build_mode_t::mode_cylinder) {
				gui::clamp (m_patches_x, 3, 15);
			} else {
				gui::clamp (m_patches_x, 1, 15);
			}

			gui::clamp (m_patches_y, 1, 15);

			ImGui::NewLine ();
			if (ImGui::Button ("Create Surface", ImVec2 (ImGui::GetWindowWidth (), 24.0f))) {
				m_add_to_scene ();
			}
			ImGui::NewLine ();
		}

		if (m_patch) {
			m_patch->configure ();
		}
	}

	void bezier_patch_c0_template::integrate (float delta_time) {
		if (m_rebuild) {
			m_rebuild_surface (m_build_mode);
			m_rebuild = false;
		}
	}

	void bezier_patch_c0_template::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_patch) {
			m_patch->render (context, world_matrix);
		}

		for (const auto & point : m_points) {
			point->render (context, point->get_matrix ());
		}
	}

	void bezier_patch_c0_template::m_add_to_scene () {
		auto & scene = get_scene ();
		scene.add_object (get_name (), std::move (m_patch));

		unsigned int points_x = (m_patches_x * 3) + 1;
		unsigned int points_y = (m_patches_y * 3) + 1;

		for (unsigned int x = 0; x < points_x; ++x) {
			for (unsigned int y = 0; y < points_y; ++y) {
				unsigned int index = (y * points_x) + x;

				m_points[index]->set_color (point_object::s_color_default);
				m_points[index]->set_select_color (point_object::s_select_default);

				scene.add_object ("point", m_points[index]);
			}
		}

		dispose ();
	}

	void bezier_patch_c0_template::m_rebuild_surface (bezier_patch_c0_template::build_mode_t mode) {
		m_build_mode = mode;
		m_points.clear ();

		// common endpoints, so each patch "owns" 3 points
		// its first point is also the last point of the previous patch
		unsigned int points_x = (m_patches_x * 3) + 1;
		unsigned int points_y = (m_patches_y * 3) + 1;

		m_patch = nullptr;
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

				if (mode == build_mode_t::mode_cylinder && x == points_x - 1) {
					m_points[index] = m_points[y * points_x];
					continue;
				}

				m_points[index] = std::make_shared<point_object> (get_scene (), m_point_shader, m_point_texture);

				float tx = 0.0f, ty = 0.0f, tz = 0.0f;

				switch (mode) {
					case build_mode_t::mode_default:
						tx = (static_cast<float> (x) * spacing) + pos.x;
						ty = 0.0f;
						tz = (static_cast<float> (y) * spacing) + pos.z;
						break;

					case build_mode_t::mode_cylinder:
						{
							float t = static_cast<float> (x) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float>();
							float r = 2.0f;

							int xmod = x % 3;
							if (xmod == 0) {
								tx = r * glm::cos (t);
								ty = r * glm::sin (t);
								tz = (static_cast<float> (y) * spacing) + pos.z;
							} else {
								float tp;
								if (xmod == 1) {
									tp = static_cast<float> (x - 1) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float> ();
								} else {
									tp = static_cast<float> (x + 1) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float> ();
								}

								float xp = r * glm::cos (tp);
								float yp = r * glm::sin (tp);

								float dx = -glm::sin (tp);
								float dy = glm::cos (tp);

								if (xmod != 1) {
									dx = -dx;
									dy = -dy;
								}

								float dang = (2.0f * glm::pi<float> () / static_cast<float> (points_x));
								float len = r * glm::tan (dang);

								tx = xp + len * dx;
								ty = yp + len * dy;
								tz = (static_cast<float> (y) * spacing) + pos.z;
							}
						}
						break;

					case build_mode_t::mode_hat:
						{
							tx = (static_cast<float> (x) * spacing) + pos.x;
							tz = (static_cast<float> (y) * spacing) + pos.z;

							float dx = tx - center.x;
							float dy = tz - center.z;

							ty = -glm::exp (1.0f / (1.0f +  0.25f * (dx * dx + dy * dy)));
						}

						break;

					default: break;
				}

				m_points[index]->set_translation ({ tx, ty, tz });
				m_points[index]->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				m_points[index]->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
			}
		}

		m_patch = std::make_shared<bezier_patch_c0> (get_scene (), m_shader, m_solid_shader, m_grid_shader, m_patches_x, m_patches_y, m_points);
	}
}