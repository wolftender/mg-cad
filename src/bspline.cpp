#include <deque>

#include "bspline.hpp"

namespace mini {
	bspline_curve::bspline_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, 
		std::shared_ptr<shader_t> shader2, std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture) :
		curve_base (scene, "bspline_c2") {

		m_shader1 = shader1;
		m_shader2 = shader2;
		m_point_texture = point_texture;
		m_point_shader = point_shader;

		rebuild_curve ();
	}

	bspline_curve::~bspline_curve () { }

	void bspline_curve::integrate (float delta_time) {
		if (is_rebuild_queued ()) {
			rebuild_curve ();
		} else {
			// convert bspline points to bezier points

			for (auto & segment : m_segments) {
				segment->set_showing_polygon (is_show_polygon ());
				segment->integrate (delta_time);
			}
		}
	}

	void bspline_curve::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		for (auto segment : m_segments) {
			context.draw (segment, world_matrix);
		}
	}

	void bspline_curve::t_rebuild_curve () {
		m_bezier_points.clear ();
		m_segments.clear ();

		const auto& points = t_get_points ();
		if (points.size () < 4) {
			return;
		}

		std::deque<std::shared_ptr<point_object>> point_q;
		std::shared_ptr<point_object> point;

		for (const auto& point_wrapper : points) {
			point = point_wrapper.point.lock ();

			if (point) {
				point_q.push_back (point);
			}

			if (point_q.size () >= 4) {
				if (point_q.size () != 4) {
					point_q.pop_front ();
				}

				glm::vec4 curve_x_bs, curve_y_bs, curve_z_bs;
				for (int i = 0; i < 4; ++i) {
					const auto& pos = point_q[i]->get_translation ();

					curve_x_bs[i] = pos.x;
					curve_y_bs[i] = pos.y;
					curve_z_bs[i] = pos.z;
				}

				// get coordinates in bezier basis
				glm::vec4 curve_x_bz = BSPLINE_TO_BEZIER * curve_x_bs;
				glm::vec4 curve_y_bz = BSPLINE_TO_BEZIER * curve_y_bs;
				glm::vec4 curve_z_bz = BSPLINE_TO_BEZIER * curve_z_bs;

				auto p1 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);
				auto p2 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);
				auto p3 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);
				auto p4 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);

				p1->set_translation ({ curve_x_bz[0], curve_y_bz[0], curve_z_bz[0] });
				p2->set_translation ({ curve_x_bz[1], curve_y_bz[1], curve_z_bz[1] });
				p3->set_translation ({ curve_x_bz[2], curve_y_bz[2], curve_z_bz[2] });
				p4->set_translation ({ curve_x_bz[3], curve_y_bz[3], curve_z_bz[3] });

				m_bezier_points.push_back (p1);
				m_bezier_points.push_back (p2);
				m_bezier_points.push_back (p3);
				m_bezier_points.push_back (p4);

				m_segments.push_back (std::make_shared<bezier_segment_gpu> (
					m_shader1, m_shader2, p1, p2, p3, p4
				));
			}
		}
	}
}