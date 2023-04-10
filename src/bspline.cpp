#include <deque>
#include <sstream>
#include <iomanip>

#include "bspline.hpp"
#include "gui.hpp"

#include <GLFW/glfw3.h>

namespace mini {
	bspline_curve::bspline_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, 
		std::shared_ptr<shader_t> shader2, std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture) :
		curve_base (scene, "bspline_c2") {

		m_shader1 = shader1;
		m_shader2 = shader2;
		m_point_texture = point_texture;
		m_point_shader = point_shader;

		m_show_bezier = false;

		rebuild_curve ();
	}

	bspline_curve::~bspline_curve () { }

	void bspline_curve::configure () {
		if (ImGui::CollapsingHeader ("B-Spline Curve")) {
			gui::prefix_label ("Show Bernstein: ", 250.0f);
			ImGui::Checkbox ("##show_bezier", &m_show_bezier);

			if (m_show_bezier) {
				ImGui::Text ("Bernstein Points:");
				// point list
				if (ImGui::BeginListBox ("##bezierlist", ImVec2 (-1.0f, 0.0f))) {
					std::stringstream name;
					int index = 0;;

					for (auto & point_wrapper : m_bezier_points) {
						auto & point = point_wrapper.point;
						const auto & pos = point->get_translation ();

						name.str (std::string ());

						if (point_wrapper.selected) {
							name << "*";
						}

						name << "point " << (index++) << " (" << std::setprecision (2) << pos.x << "; " << std::setprecision (2) << pos.y << ")";
						
						if (ImGui::Selectable (name.str ().c_str (), &point_wrapper.selected)) {
							m_select_point (point_wrapper);
						}
					}

					ImGui::EndListBox ();
				}
			}

			ImGui::NewLine ();
		}

		curve_base::configure ();
	}

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

		// draw the control points if asked to
		if (m_show_bezier && is_selected ()) {
			for (const auto & point : m_bezier_points) {
				context.draw (point.point, point.point->get_matrix ());
			}
		}
	}

	bool bspline_curve::on_mouse_button (int button, int action, int mods) {
		if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
			auto hit_data = get_scene ().get_hit_test_data ();

			glm::vec3 cam_pos = get_scene ().get_camera ().get_position ();
			glm::vec3 hit_pos;
			float dist, best_dist = 1000000.0f;

			point_wrapper * sel = nullptr;
			for (auto & point : m_bezier_points) {
				if (point.point->hit_test (hit_data, hit_pos)) {
					dist = glm::distance (cam_pos, hit_pos);
					if (dist < best_dist) {
						sel = &point;
					}
				}
			}

			if (sel) {
				m_select_point (*sel);
			}

			return (sel != nullptr);
		}

		return false;
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

		point_ptr prev_end = nullptr;

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

				point_ptr p1 = nullptr;
				if (prev_end) {
					p1 = prev_end;
				} else {
					p1 = std::make_shared<point_object> (get_scene (), m_point_shader, m_point_texture);
					p1->set_translation ({ curve_x_bz[0], curve_y_bz[0], curve_z_bz[0] });
					p1->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
					p1->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });

					m_bezier_points.push_back ({ p1, false });
				}

				point_ptr p2 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);
				point_ptr p3 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);
				point_ptr p4 = std::make_shared<point_object>(get_scene (), m_point_shader, m_point_texture);
				
				p2->set_translation ({ curve_x_bz[1], curve_y_bz[1], curve_z_bz[1] });
				p3->set_translation ({ curve_x_bz[2], curve_y_bz[2], curve_z_bz[2] });
				p4->set_translation ({ curve_x_bz[3], curve_y_bz[3], curve_z_bz[3] });
				
				p2->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				p3->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				p4->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });

				p2->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
				p3->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
				p4->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
				
				m_bezier_points.push_back ({p2, false});
				m_bezier_points.push_back ({p3, false});
				m_bezier_points.push_back ({p4, false});

				prev_end = p4;

				m_segments.push_back (std::make_shared<bezier_segment_gpu> (
					m_shader1, m_shader2, p1, p2, p3, p4
				));
			}
		}
	}

	void bspline_curve::m_select_point (point_wrapper & wrapper) {
		for (auto & p : m_bezier_points) {
			p.selected = false;
			p.point->set_selected (false);
		}

		wrapper.selected = true;
		wrapper.point->set_selected (true);
	}
}