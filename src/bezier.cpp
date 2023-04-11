#include "bezier.hpp"
#include "gui.hpp"

#include <algorithm>

namespace mini {
	/***********************/
	/*     BASE INTER      */
	/***********************/
	const std::array<point_wptr, 4> bezier_segment_base::t_get_points () const {
		return m_points;
	}

	bool bezier_segment_base::is_showing_polygon () const {
		return m_show_polygon;
	}

	void bezier_segment_base::set_showing_polygon (bool show) {
		m_show_polygon = show;
	}

	bezier_segment_base::bezier_segment_base (point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3) {
		m_points[0] = p0;
		m_points[1] = p1;
		m_points[2] = p2;
		m_points[3] = p3;

		m_show_polygon = false;
	}
	
	/***********************/
	/*     GPU IMPL        */
	/***********************/
	bezier_segment_gpu::bezier_segment_gpu (std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2, 
		point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3)
		: bezier_segment_base (p0, p1, p2, p3) {

		m_shader = shader1;
		m_line_shader = shader2;

		m_vao = m_vao_poly = 0;
		m_position_buffer = m_position_buffer_poly = 0;
		m_degree = m_last_degree = 0;
		m_ready = false;

		m_positions.resize (12);

		m_init_positions ();
		m_init_buffers ();
	}

	bezier_segment_gpu::~bezier_segment_gpu () {
		m_free_buffers ();
	}

	void bezier_segment_gpu::integrate (float delta_time) {
		if (m_ready) {
			m_update_buffers ();
		}
	}

	void bezier_segment_gpu::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (!m_ready) {
			return;
		}

		switch (m_degree) {
			default:
				return;

			case 1:
				glBindVertexArray (m_vao);
				m_bind_shader (context, m_line_shader, world_matrix);

				glDrawArrays (GL_LINES, 0, 6);
				break;

			case 2:
				[[fallthrough]];
			case 3:
				glBindVertexArray (m_vao);
				m_bind_shader (context, m_shader, world_matrix);

				m_shader->set_uniform ("u_start_t", 0.0f);
				m_shader->set_uniform ("u_end_t", 0.5f);
				glDrawArrays (GL_LINES_ADJACENCY, 0, 4);

				m_shader->set_uniform ("u_start_t", 0.5f);
				m_shader->set_uniform ("u_end_t", 1.0f);
				glDrawArrays (GL_LINES_ADJACENCY, 0, 4);

				break;
		}

		if (is_showing_polygon () && m_degree > 1) {
			glBindVertexArray (m_vao_poly);
			m_bind_shader (context, m_line_shader, world_matrix);
			glDrawArrays (GL_LINES, 0, m_degree * 6); // magic number 18, should be called something probably
		}

		glBindVertexArray (static_cast<GLuint> (NULL));
	}

	void bezier_segment_gpu::m_free_buffers () {
		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
		}

		if (m_vao_poly) {
			glDeleteVertexArrays (1, &m_vao_poly);
		}

		if (m_position_buffer) {
			glDeleteBuffers (1, &m_position_buffer);
		}

		if (m_position_buffer_poly) {
			glDeleteBuffers (1, &m_position_buffer_poly);
		}
	}

	void bezier_segment_gpu::m_bind_shader (app_context & context, std::shared_ptr<shader_t> shader, const glm::mat4x4 & world_matrix) const {
		shader->bind ();

		const auto & view_matrix = context.get_view_matrix ();
		const auto & proj_matrix = context.get_projection_matrix ();

		const auto & video_mode = context.get_video_mode ();

		glm::vec2 resolution = {
			static_cast<float> (video_mode.get_buffer_width ()),
			static_cast<float> (video_mode.get_buffer_height ())
		};

		shader->set_uniform ("u_world", world_matrix);
		shader->set_uniform ("u_view", view_matrix);
		shader->set_uniform ("u_projection", proj_matrix);
		shader->set_uniform ("u_resolution", resolution);
		shader->set_uniform ("u_line_width", 2.0f);
	}

	void bezier_segment_gpu::m_init_positions () {
		int degree = 0;
		glm::vec3 b[4];

		for (int index = 0; index < 4; ++index) {
			auto point = t_get_points ()[index].lock ();

			if (point) {
				b[degree] = point->get_translation ();
				degree++;
			}
		}

		degree = degree - 1;
		m_degree = degree;

		switch (m_degree) {
			default:
				m_degree = 0;
				return;

			case 1:
				m_positions.resize (6);

				// degree 1 is simply a line
				m_positions[0] = b[0].x;
				m_positions[1] = b[0].y;
				m_positions[2] = b[0].z;

				m_positions[3] = b[1].x;
				m_positions[4] = b[1].y;
				m_positions[5] = b[1].z;
				return;

			case 2:
				// increase degree of this polynomial
				{
					glm::vec3 d[4] = { { 0.0f, 0.0f, 0.0f } };
					for (int j = 1; j < 4; ++j) {
						float a = static_cast<float>(j) / (m_degree + 1);
						d[j].x = a * b[j - 1].x;
						d[j].y = a * b[j - 1].y;
						d[j].z = a * b[j - 1].z;
					}

					for (int j = 0; j < 3; ++j) {
						float a = static_cast<float>(m_degree + 1 - j) / (m_degree + 1);
						d[j].x += a * b[j].x;
						d[j].y += a * b[j].y;
						d[j].z += a * b[j].z;
					}

					m_positions.clear ();
					m_positions.resize (12);

					for (int index = 0; index < 4; ++index) {
						int offset = index * 3;

						m_positions[offset + 0] = d[index].x;
						m_positions[offset + 1] = d[index].y;
						m_positions[offset + 2] = d[index].z;
					}
				}
				
				break;
			case 3:
				m_positions.clear ();
				m_positions.resize (12);

				for (int index = 0; index < 4; ++index) {
					int offset = index * 3;

					m_positions[offset + 0] = b[index].x;
					m_positions[offset + 1] = b[index].y;
					m_positions[offset + 2] = b[index].z;
				}

				break;
		}

		// now allocate positions for the polygon object
		m_positions_poly.clear ();
		m_positions_poly.resize (m_degree * 6);

		// write the lines into the buffer
		for (int i = 0; i < m_degree; ++i) {
			int offset = 6 * i;

			// p1
			m_positions_poly[offset + 0] = b[i + 0].x;
			m_positions_poly[offset + 1] = b[i + 0].y;
			m_positions_poly[offset + 2] = b[i + 0].z;

			// p2
			m_positions_poly[offset + 3] = b[i + 1].x;
			m_positions_poly[offset + 4] = b[i + 1].y;
			m_positions_poly[offset + 5] = b[i + 1].z;
		}
	}

	void bezier_segment_gpu::m_init_buffers () {
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_position_buffer);

		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		if (m_degree > 1) {
			glGenVertexArrays (1, &m_vao_poly);
			glBindVertexArray (m_vao_poly);

			glGenBuffers (1, &m_position_buffer_poly);
			glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer_poly);
			glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions_poly.size (), reinterpret_cast<void *> (m_positions_poly.data ()), GL_DYNAMIC_DRAW);
			glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
			glEnableVertexAttribArray (a_position);
		}

		glBindVertexArray (static_cast<GLuint> (NULL));

		if (m_degree > 0 && m_degree < 4) {
			m_ready = true;
		}

		m_last_degree = m_degree;
	}

	void bezier_segment_gpu::m_update_buffers () {
		m_init_positions ();

		if (m_degree == m_last_degree) {
			glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer);
			glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (float) * m_positions.size (), m_positions.data ());
			glBindBuffer (GL_ARRAY_BUFFER, static_cast<GLuint> (NULL));

			if (m_degree > 1) {
				glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer_poly);
				glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (float) * m_positions_poly.size (), m_positions_poly.data ());
				glBindBuffer (GL_ARRAY_BUFFER, static_cast<GLuint> (NULL));
			}
		} else {
			m_free_buffers ();
			m_init_buffers ();
		}
	}

	/***********************/
	/*     CURVE BASE      */
	/***********************/
	void curve_base::m_moved_sighandler (signal_event_t sig, scene_obj_t & sender) {
		set_rebuild_queued (true);
	}

	void curve_base::rebuild_curve () {
		m_queue_curve_rebuild = false;
		t_rebuild_curve ();
	}

	bool curve_base::is_rebuild_queued () const {
		return m_queue_curve_rebuild;
	}

	bool curve_base::is_auto_extend () const {
		return m_auto_extend;
	}

	bool curve_base::is_show_polygon () const {
		return m_show_polygon;
	}

	void curve_base::set_rebuild_queued (bool rebuild) {
		m_queue_curve_rebuild = rebuild;
	}

	void curve_base::set_auto_extend (bool extend) {
		m_auto_extend = extend;
	}

	void curve_base::set_show_polygon (bool show) {
		m_show_polygon = show;
	}

	curve_base::curve_base (scene_controller_base & scene, const std::string & name) : scene_obj_t (scene, name, false, false, false) {
		m_auto_extend = false;
		m_show_polygon = false;
		m_queue_curve_rebuild = false;
		m_configured = false;

		t_set_handler (signal_event_t::moved, std::bind (&curve_base::m_moved_sighandler,
			this, std::placeholders::_1, std::placeholders::_2));

		for (auto iter = get_scene ().get_selected_objects (); iter->has (); iter->next ()) {
			auto object = std::dynamic_pointer_cast<point_object> (iter->get_object ());

			if (object) {
				m_points.push_back (point_wrapper_t (object));
			}
		}
	}

	const std::vector<curve_base::point_wrapper_t> & curve_base::t_get_points () const {
		return m_points;
	}

	std::vector<curve_base::point_wrapper_t> & curve_base::t_get_points () {
		return m_points;
	}

	void curve_base::configure () {
		// todo: move this somewhere else
		if (!m_configured) {
			for (const auto & wrapper : m_points) {
				auto ptr = wrapper.point.lock ();
				if (ptr) {
					t_listen (signal_event_t::moved, *ptr);
				}
			}

			m_configured = true;
		}

		if (ImGui::CollapsingHeader ("Curve Settings")) {
			gui::prefix_label ("Auto Extend: ", 250.0f);
			ImGui::Checkbox ("##auto_extend", &m_auto_extend);

			gui::prefix_label ("Show Polygon: ", 250.0f);
			ImGui::Checkbox ("##show_polygon", &m_show_polygon);

			ImGui::Text ("Control Points:");
			// point list
			if (ImGui::BeginListBox ("##pointlist", ImVec2 (-1.0f, 0.0f))) {
				for (auto & point_wrapper : m_points) {
					auto point = point_wrapper.point.lock ();

					if (point) {
						std::string full_name;
						bool selected = point_wrapper.selected;

						if (selected) {
							full_name = "*" + point->get_name () + " : (" + point->get_type_name () + ")";
						} else {
							full_name = point->get_name () + " : (" + point->get_type_name () + ")";
						}

						ImGui::Selectable (full_name.c_str (), &point_wrapper.selected);
					}
				}

				ImGui::EndListBox ();
			}

			ImGui::NewLine ();

			if (ImGui::Button ("Add Selected", ImVec2 (ImGui::GetWindowWidth () * 0.45f, 24.0f))) {
				auto iter = get_scene ().get_selected_objects ();

				for (; iter->has (); iter->next ()) {
					auto object = iter->get_object ();
					auto point = std::dynamic_pointer_cast<point_object> (object);

					if (point) {
						bool duplicate = false;
						for (auto point_wrapper : m_points) {
							if (point_wrapper.point.lock () == point) {
								duplicate = true;
								break;
							}
						}

						if (duplicate) {
							continue;
						} else {
							t_listen (signal_event_t::moved, *point);
							m_points.push_back (point_wrapper_t (point));
							m_queue_curve_rebuild = true;
						}
					}
				}
			}

			ImGui::SameLine ();

			if (ImGui::Button ("Delete Selected", ImVec2 (-1.0f, 24.0f))) {
				for (auto iter = m_points.begin (); iter != m_points.end (); ++iter) {
					if (iter->selected) {
						iter = m_points.erase (iter);
						m_queue_curve_rebuild = true;
					}

					if (iter == m_points.end ()) {
						break;
					}
				}
			}

			ImGui::NewLine ();
		}
	}

	void curve_base::t_on_object_created (std::shared_ptr<scene_obj_t> object) {
		if (!m_auto_extend) {
			return;
		}

		auto point = std::dynamic_pointer_cast<point_object> (object);

		if (point) {
			t_listen (signal_event_t::moved, *point);
			m_points.push_back (point_wrapper_t (point));
			m_queue_curve_rebuild = true;
		}
	}

	void curve_base::t_on_object_deleted (std::shared_ptr<scene_obj_t> object) {
		bool changed = false;

		for (auto iter = m_points.begin (); iter != m_points.end (); ++iter) {
			auto point = iter->point.lock ();

			if (point) {
				if (object == point) {
					iter = m_points.erase (iter);
					changed = true;
				}
			} else {
				iter = m_points.erase (iter);
				changed = true;
			}

			if (iter == m_points.end ()) {
				break;
			}
		}

		if (changed) {
			m_queue_curve_rebuild = true;
		}
	}

	/***********************/
	/*      BEZIER C0      */
	/***********************/
	bezier_curve_c0::bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader1,
		std::shared_ptr<shader_t> shader2, bool is_gpu) :
		curve_base (scene, (is_gpu) ? "gpu_bezier_c0" : "bezier_c0"),
		m_is_gpu (is_gpu) {

		m_shader1 = shader1;
		m_shader2 = shader2;

		rebuild_curve ();
	}

	bezier_curve_c0::~bezier_curve_c0 () {}

	void bezier_curve_c0::integrate (float delta_time) {
		if (is_rebuild_queued ()) {
			rebuild_curve ();
		} else {
			for (auto & segment : m_segments) {
				segment->set_showing_polygon (is_show_polygon ());
				segment->integrate (delta_time);
			}
		}
	}

	void bezier_curve_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		for (auto segment : m_segments) {
			context.draw (segment, world_matrix);
		}
	}

	void bezier_curve_c0::t_rebuild_curve () {
		m_segments.clear ();

		point_wptr points[4];
		int index = 0, array_index = 0;

		for (auto & point_wrapper : t_get_points ()) {
			auto point = point_wrapper.point.lock ();

			if (point) {
				array_index = index % 4;
				points[array_index] = point;

				index++;

				if (index > 0 && index % 4 == 0) {
					if (m_is_gpu) {
						m_segments.push_back (std::make_shared<bezier_segment_gpu> (
							m_shader1, m_shader2, points[0], points[1], points[2], points[3])
						);
					} else {
						m_segments.push_back (std::make_shared<bezier_segment_cpu> (
							get_scene (), m_shader1, m_shader2, points[0], points[1], points[2], points[3])
						);
					}

					points[0] = points[3];
					index++;
				}
			}
		}

		// todo: make gpu segments be rendered by gpu
		if (index > 0) {
			if (index % 4 == 2) {
				if (m_is_gpu) {
					m_segments.push_back (std::make_shared<bezier_segment_gpu> (
						m_shader1, m_shader2, points[0], points[1], point_wptr (), point_wptr ())
					);
				} else {
					m_segments.push_back (std::make_shared<bezier_segment_cpu> (
						get_scene (), m_shader1, m_shader2, points[0], points[1], point_wptr (), point_wptr ())
					);
				}
			} else if (index % 4 == 3) {
				if (m_is_gpu) {
					m_segments.push_back (std::make_shared<bezier_segment_gpu> (
						m_shader1, m_shader2, points[0], points[1], points[2], point_wptr ())
					);
				} else {
					m_segments.push_back (std::make_shared<bezier_segment_cpu> (
						get_scene (), m_shader1, m_shader2, points[0], points[1], points[2], point_wptr ())
					);
				}
			}
		}
	}

	/***********************/
	/*     CPU IMPL        */
	/***********************/
	bezier_segment_cpu::bezier_segment_cpu (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2,
		point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3) : 
		bezier_segment_base (p0, p1, p2, p3), m_scene (scene) {

		m_shader = shader1;
		m_poly_shader = shader2;

		m_vao = m_position_buffer = 0;
		m_ready = false;

		m_divisions = 64;
		m_degree = 0;
	}

	bezier_segment_cpu::~bezier_segment_cpu () {
		m_free_buffers ();
	}

	void bezier_segment_cpu::integrate (float delta_time) {
		m_update_buffers ();
	}

	void bezier_segment_cpu::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (!m_ready) {
			return;
		}

		m_shader->bind ();

		const auto & view_matrix = context.get_view_matrix ();
		const auto & proj_matrix = context.get_projection_matrix ();

		const auto & video_mode = context.get_video_mode ();

		glm::vec2 resolution = {
			static_cast<float> (video_mode.get_buffer_width ()),
			static_cast<float> (video_mode.get_buffer_height ())
		};

		m_shader->set_uniform ("u_world", world_matrix);
		m_shader->set_uniform ("u_view", view_matrix);
		m_shader->set_uniform ("u_projection", proj_matrix);
		m_shader->set_uniform ("u_resolution", resolution);
		m_shader->set_uniform ("u_line_width", 2.0f);

		glBindVertexArray (m_vao);
		glDrawArrays (GL_LINES, 0, m_divisions * 2 + 2);

		// now draw the polygon if asked to
		if (is_showing_polygon () && m_degree > 1) {
			m_poly_shader->bind ();

			m_poly_shader->set_uniform ("u_world", world_matrix);
			m_poly_shader->set_uniform ("u_view", view_matrix);
			m_poly_shader->set_uniform ("u_projection", proj_matrix);
			m_poly_shader->set_uniform ("u_resolution", resolution);
			m_poly_shader->set_uniform ("u_line_width", 2.0f);

			glBindVertexArray (m_poly_vao);
			glDrawArrays (GL_LINES, 0, m_degree * 6); // magic number 18, should be called something probably
		}

		glBindVertexArray (static_cast<GLuint> (NULL));
	}

	void bezier_segment_cpu::m_free_buffers () {
		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
		}

		if (m_position_buffer) {
			glDeleteBuffers (1, &m_position_buffer);
		}

		if (m_poly_vao) {
			glDeleteVertexArrays (1, &m_poly_vao);
		}

		if (m_position_buffer) {
			glDeleteBuffers (1, &m_position_buffer_poly);
		}
	}

	void bezier_segment_cpu::m_init_buffers () {
		constexpr GLuint a_position = 0;
		
		if (!m_update_positions ()) {
			m_ready = false;
			return;
		}

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_position_buffer);

		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindVertexArray (static_cast<GLuint> (NULL));

		// second vao, do not allocate for degree 1
		if (m_degree > 1) {
			glGenVertexArrays (1, &m_poly_vao);
			glGenBuffers (1, &m_position_buffer_poly);

			glBindVertexArray (m_poly_vao);

			glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer_poly);
			glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions_poly.size (), reinterpret_cast<void *> (m_positions_poly.data ()), GL_STATIC_DRAW);
			glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
			glEnableVertexAttribArray (a_position);
		}

		m_ready = true;
	}

	void bezier_segment_cpu::m_update_buffers () {
		m_free_buffers ();
		m_init_buffers ();
	}

	bool bezier_segment_cpu::m_update_positions () {
		m_positions.clear ();

		// get bernstein basis coefficients
		glm::vec3 b[4];
		int degree = 0;

		for (int i = 0; i < 4; ++i) {
			auto point = t_get_points ()[i].lock ();

			if (point) {
				b[degree] = point->get_translation ();
				degree++;
			}
		}

		degree = degree - 1;
		m_degree = degree;

		if (degree == 0) {
			return false;
		}

		if (degree == 1) {
			// degree 1, so it is simply a line
			m_divisions = 1;
			m_positions.resize (m_divisions * 6);

			// degree 1 is simply a line
			m_positions[0] = b[0].x;
			m_positions[1] = b[0].y;
			m_positions[2] = b[0].z;

			m_positions[3] = b[1].x;
			m_positions[4] = b[1].y;
			m_positions[5] = b[1].z;
		} else {
			// degree 2 or 3 so it is a curve
			// calculate number of divisions based on curve length
			const auto & projection = m_scene.get_camera ().get_projection_matrix ();
			const auto & view = m_scene.get_camera ().get_view_matrix ();

			float res_x = static_cast<float> (m_scene.get_video_mode ().get_buffer_width ());
			float res_y = static_cast<float> (m_scene.get_video_mode ().get_buffer_height ());

			float curve_length = 0.0f;

			for (int i = 0; i < m_degree - 1; ++i) {
				glm::vec4 p1 = projection * view * glm::vec4 (b[i + 0], 1.0f);
				glm::vec4 p2 = projection * view * glm::vec4 (b[i + 1], 1.0f);

				p1 = p1 / p1.w;
				p2 = p2 / p2.w;

				curve_length += glm::distance (glm::vec2 { p1.x * res_x, p1.y * res_y }, glm::vec2 { p2.x * res_x, p2.y * res_y });
			}

			m_divisions = glm::min (150, glm::max (15, static_cast<int> (curve_length / 50.0f)));

			m_positions.resize (m_divisions * 6); // for each division theres a line
			float step = 1.0f / m_divisions;

			if (m_degree == 2) {
				// sample the polynomial at intervals
				for (int i = 0; i < m_divisions; ++i) {
					int offset = 6 * i;
					float t = step * static_cast<float> (i);

					m_positions[offset + 0] = m_decasteljeu (b[0].x, b[1].x, b[2].x, t);
					m_positions[offset + 1] = m_decasteljeu (b[0].y, b[1].y, b[2].y, t);
					m_positions[offset + 2] = m_decasteljeu (b[0].z, b[1].z, b[2].z, t);

					m_positions[offset + 3] = m_decasteljeu (b[0].x, b[1].x, b[2].x, t + step);
					m_positions[offset + 4] = m_decasteljeu (b[0].y, b[1].y, b[2].y, t + step);
					m_positions[offset + 5] = m_decasteljeu (b[0].z, b[1].z, b[2].z, t + step);
				}
			} else if (m_degree == 3) {
				for (int i = 0; i < m_divisions; ++i) {
					int offset = 6 * i;
					float t = step * static_cast<float> (i);

					m_positions[offset + 0] = m_decasteljeu (b[0].x, b[1].x, b[2].x, b[3].x, t);
					m_positions[offset + 1] = m_decasteljeu (b[0].y, b[1].y, b[2].y, b[3].y, t);
					m_positions[offset + 2] = m_decasteljeu (b[0].z, b[1].z, b[2].z, b[3].z, t);

					m_positions[offset + 3] = m_decasteljeu (b[0].x, b[1].x, b[2].x, b[3].x, t + step);
					m_positions[offset + 4] = m_decasteljeu (b[0].y, b[1].y, b[2].y, b[3].y, t + step);
					m_positions[offset + 5] = m_decasteljeu (b[0].z, b[1].z, b[2].z, b[3].z, t + step);
				}
			}

			// now allocate positions for the polygon object
			m_positions_poly.clear ();
			m_positions_poly.resize (degree * 6);

			// write the lines into the buffer
			for (int i = 0; i < degree; ++i) {
				int offset = 6 * i;

				// p1
				m_positions_poly[offset + 0] = b[i + 0].x;
				m_positions_poly[offset + 1] = b[i + 0].y;
				m_positions_poly[offset + 2] = b[i + 0].z;

				// p2
				m_positions_poly[offset + 3] = b[i + 1].x;
				m_positions_poly[offset + 4] = b[i + 1].y;
				m_positions_poly[offset + 5] = b[i + 1].z;
			}
		}

		// update degree of this polynomial fragment
		m_degree = degree;
		return true;
	}

	float bezier_segment_cpu::m_decasteljeu (float b00, float b01, float b02, float b03, float t) const {
		float t1 = t;
		float t0 = 1.0f - t;

		float b10, b11, b12;
		float b20, b21;
		float b30;

		b10 = t0 * b00 + t1 * b01;
		b11 = t0 * b01 + t1 * b02;
		b12 = t0 * b02 + t1 * b03;

		b20 = t0 * b10 + t1 * b11;
		b21 = t0 * b11 + t1 * b12;

		b30 = t0 * b20 + t1 * b21;

		return b30;
	}

	float bezier_segment_cpu::m_decasteljeu (float b00, float b01, float b02, float t) const {
		float t1 = t;
		float t0 = 1.0f - t;

		float b10, b11;
		float b20;

		b10 = t0 * b00 + t1 * b01;
		b11 = t0 * b01 + t1 * b02;

		b20 = t0 * b10 + t1 * b11;

		return b20;
	}
}