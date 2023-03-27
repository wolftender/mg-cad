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
				m_points.push_back (object);
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
			ImGui::NewLine ();
		}
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

		point_wptr points[4];
		int index = 0, array_index = 0;

		for (auto & point_weak : m_points) {
			auto point = point_weak.lock ();

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

					}

					points[0] = points[3];
					index++;
				}
			}
		}

		// todo: ending segment of degree smaller than 3
	}
}