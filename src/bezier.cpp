#include "bezier.hpp"
#include "gui.hpp"

#include <algorithm>

namespace mini {
	bezier_curve_c0::bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader) : 
		scene_obj_t (scene, "bezier c0", false, false, false) {

		for (auto iter = get_scene ().get_selected_objects (); iter->has (); iter->next ()) {
			auto object = std::dynamic_pointer_cast<point_object> (iter->get_object ());

			if (object) {
				m_points.push_back (object);
			}
		}
		
		m_shader = shader;
		m_auto_extend = false;
		m_queue_curve_rebuild = false;

		m_rebuild_curve ();
	}

	bezier_curve_c0::~bezier_curve_c0 () { }

	void bezier_curve_c0::integrate (float delta_time) { 
		if (m_queue_curve_rebuild) {
			m_rebuild_curve ();
		}
	}

	void bezier_curve_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		for (auto segment : m_segments) {
			context.draw (segment, world_matrix);
		}
	}

	void bezier_curve_c0::configure () {
		gui::prefix_label ("Auto Extend: ", 250.0f);
		ImGui::Checkbox ("##auto_extend", &m_auto_extend);
		ImGui::NewLine ();
	}

	void bezier_curve_c0::t_on_object_created (std::shared_ptr<scene_obj_t> object) {
		if (!m_auto_extend) {
			return;
		}

		auto point = std::dynamic_pointer_cast<point_object> (object);

		if (point) {
			m_points.push_back (point);
			m_queue_curve_rebuild = true;
		}
	}

	void bezier_curve_c0::t_on_object_deleted (std::shared_ptr<scene_obj_t> object) {
		bool changed = false;

		for (auto iter = m_points.begin (); iter != m_points.end (); ++iter) {
			auto point = iter->lock ();

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

		glm::vec3 points[4];
		int index = 0, array_index = 0;

		std::fill_n (points, 4, glm::vec3 { 0.0f, 0.0f, 0.0f });

		for (auto & point_weak : m_points) {
			auto point = point_weak.lock ();

			if (point) {
				array_index = index % 4;
				points[array_index] = point->get_translation ();

				index++;

				if (index > 0 && index % 4 == 0) {
					m_segments.push_back (std::make_shared<bezier_segment> (
						m_shader, points[0], points[1], points[2], points[3])
					);

					points[0] = points[3];
					index++;
				}
			}
		}

		// todo: ending segment of degree smaller than 3
	}

	bezier_curve_c0::bezier_segment::bezier_segment (std::shared_ptr<shader_t> shader, const glm::vec3 & p0, 
		const glm::vec3 & p1, const glm::vec3 & p2, const glm::vec3 & p3) {

		m_shader = shader;
		m_vao = m_color_buffer = m_position_buffer = 0;

		constexpr GLuint a_position = 0;
		constexpr GLuint a_color = 1;

		m_positions = {
			p0.x, p0.y, p0.z,
			p1.x, p1.y, p1.z,
			p2.x, p2.y, p2.z,
			p3.x, p3.y, p3.z,
		};

		m_colors = {
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f,
			1.0f, 1.0f, 1.0f, 1.0f
		};

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_position_buffer);
		glGenBuffers (1, &m_color_buffer);

		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_position_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_positions.size (), reinterpret_cast<void *> (m_positions.data ()), GL_STATIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindBuffer (GL_ARRAY_BUFFER, m_color_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_colors.size (), reinterpret_cast<void *> (m_colors.data ()), GL_STATIC_DRAW);
		glVertexAttribPointer (a_color, 4, GL_FLOAT, false, sizeof (float) * 4, (void *)0);
		glEnableVertexAttribArray (a_color);

		glBindVertexArray (static_cast<GLuint> (NULL));
	}

	bezier_curve_c0::bezier_segment::~bezier_segment () {
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

	void bezier_curve_c0::bezier_segment::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		glBindVertexArray (m_vao);

		m_shader->bind ();

		// the good stuff
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

		// lets go
		glDrawArrays (GL_LINES_ADJACENCY, 0, 4);

		glBindVertexArray (static_cast<GLuint> (NULL));
	}
}