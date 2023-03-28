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
		m_poly_shader = shader2;

		m_vao = m_color_buffer = m_position_buffer = 0;

		m_ready = false;

		m_positions.resize (12);
		
		m_colors = {
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f
		};

		m_init_buffers ();
	}

	bezier_segment_gpu::~bezier_segment_gpu () {
		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
		}

		if (m_position_buffer) {
			glDeleteBuffers (1, &m_position_buffer);
		}

		if (m_color_buffer) {
			glDeleteBuffers (1, &m_color_buffer);
		}
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

		glBindVertexArray (m_vao);

		m_bind_shader (context, m_shader, world_matrix);
		glDrawArrays (GL_LINES_ADJACENCY, 0, 4);

		if (is_showing_polygon ()) {
			m_bind_shader (context, m_poly_shader, world_matrix);
			glDrawArrays (GL_LINES_ADJACENCY, 0, 4);
		}

		glBindVertexArray (static_cast<GLuint> (NULL));
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

	void bezier_segment_gpu::m_init_buffers () {
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		for (int index = 0; index < 4; ++index) {
			auto point = t_get_points ()[index].lock ();

			if (!point) {
				m_ready = false;
				return;
			}

			const auto& pos = point->get_translation ();
			int offset = index * 3;

			m_positions[offset + 0] = pos.x;
			m_positions[offset + 1] = pos.y;
			m_positions[offset + 2] = pos.z;
		}

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_position_buffer);
		//glGenBuffers (1, &m_color_buffer);

		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		// unused
		/*glBindBuffer (GL_ARRAY_BUFFER, m_color_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_colors.size (), reinterpret_cast<void *> (m_colors.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_color, 4, GL_FLOAT, false, sizeof (float) * 4, (void *)0);
		glEnableVertexAttribArray (a_color);*/

		glBindVertexArray (static_cast<GLuint> (NULL));
		m_ready = true;
	}

	void bezier_segment_gpu::m_update_buffers () {
		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		for (int index = 0; index < 4; ++index) {
			auto point = t_get_points ()[index].lock ();

			if (!point) {
				m_ready = false;
				return;
			}

			const auto & pos = point->get_translation ();
			int offset = index * 3;

			m_positions[offset + 0] = pos.x;
			m_positions[offset + 1] = pos.y;
			m_positions[offset + 2] = pos.z;
		}

		glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer);
		glBufferSubData (GL_ARRAY_BUFFER, 0, sizeof (float) * m_positions.size (), m_positions.data ());
		glBindBuffer (GL_ARRAY_BUFFER, static_cast<GLuint> (NULL));
	}


	bezier_curve_c0::bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader1,
		std::shared_ptr<shader_t> shader2, bool is_gpu) :
		scene_obj_t (scene, (is_gpu) ? "gpu_bezier_c0" : "bezier_c0", false, false, false),
		m_is_gpu (is_gpu) {

		for (auto iter = get_scene ().get_selected_objects (); iter->has (); iter->next ()) {
			auto object = std::dynamic_pointer_cast<point_object> (iter->get_object ());

			if (object) {
				m_points.push_back (point_wrapper_t (object));
			}
		}

		m_shader1 = shader1;
		m_shader2 = shader2;

		m_auto_extend = false;
		m_show_polygon = false;
		m_queue_curve_rebuild = false;

		m_rebuild_curve ();
	}

	bezier_curve_c0::~bezier_curve_c0 () {}

	void bezier_curve_c0::integrate (float delta_time) {
		if (m_queue_curve_rebuild) {
			m_rebuild_curve ();
		} else {
			for (auto & segment : m_segments) {
				segment->set_showing_polygon (m_show_polygon);
				segment->integrate (delta_time);
			}
		}
	}

	void bezier_curve_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		for (auto segment : m_segments) {
			context.draw (segment, world_matrix);
		}
	}

	void bezier_curve_c0::configure () {
		if (ImGui::CollapsingHeader ("Bezier Curve")) {
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

	void bezier_curve_c0::t_on_object_created (std::shared_ptr<scene_obj_t> object) {
		if (!m_auto_extend) {
			return;
		}

		auto point = std::dynamic_pointer_cast<point_object> (object);

		if (point) {
			m_points.push_back (point_wrapper_t (point));
			m_queue_curve_rebuild = true;
		}
	}

	void bezier_curve_c0::t_on_object_deleted (std::shared_ptr<scene_obj_t> object) {
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

	void bezier_curve_c0::m_rebuild_curve () {
		m_queue_curve_rebuild = false;
		m_segments.clear ();

		point_wptr points[4];
		int index = 0, array_index = 0;

		for (auto & point_wrapper : m_points) {
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
							m_shader1, m_shader2, points[0], points[1], points[2], points[3])
						);
					}

					points[0] = points[3];
					index++;
				}
			}
		}

		// todo: ending segment of degree smaller than 3
	}

	/***********************/
	/*     CPU IMPL        */
	/***********************/
	bezier_segment_cpu::bezier_segment_cpu (std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2,
		point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3) : 
		bezier_segment_base (p0, p1, p2, p3) {

		m_shader = shader1;
		m_poly_shader = shader2;

		m_vao = m_position_buffer = 0;
		m_ready = false;

		m_divisions = 64;
	}

	bezier_segment_cpu::~bezier_segment_cpu () {
		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
		}

		if (m_position_buffer) {
			glDeleteBuffers (1, &m_position_buffer);
		}
	}

	void bezier_segment_cpu::integrate (float delta_time) {
		m_init_buffers ();
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

		glBindVertexArray (static_cast<GLuint> (NULL));
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
		m_ready = true;
	}

	void bezier_segment_cpu::m_update_buffers () {
		if (m_vao) {
			glDeleteVertexArrays (1, &m_vao);
		}

		if (m_position_buffer) {
			glDeleteBuffers (1, &m_position_buffer);
		}

		m_init_buffers ();
	}

	bool bezier_segment_cpu::m_update_positions () {
		m_positions.clear ();
		m_positions.resize (m_divisions * 6); // for each division theres a line

		float step = 1.0f / m_divisions;

		// get bernstein basis coefficients
		glm::vec3 b[4];
		for (int i = 0; i < 4; ++i) {
			auto point = t_get_points ()[i].lock ();

			if (!point) {
				return false;
			}

			b[i] = point->get_translation ();
		}

		// sample the polynomial at intervals
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

	float bezier_segment_cpu::m_decasteljeu (float b00, float b01, float b02, float b03, float t) const {
		float t1 = t;
		float t0 = 1.0 - t;

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
}