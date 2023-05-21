#include <deque>
#include <sstream>
#include <iomanip>

#include "bspline.hpp"
#include "gui.hpp"
#include "serializer.hpp"

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

		m_drag = false;
		m_drag_index = -1;

		rebuild_curve ();
	}

	bspline_curve::bspline_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2, 
		std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, const point_list & points) :
		curve_base (scene, "bspline_c2") {

		m_shader1 = shader1;
		m_shader2 = shader2;
		m_point_texture = point_texture;
		m_point_shader = point_shader;

		m_show_bezier = false;

		m_drag = false;
		m_drag_index = -1;

		t_set_points (points);
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

					for (auto & point_wrapper : m_bezier_points) {
						auto & point = point_wrapper.point;
						const auto & pos = point->get_translation ();

						name.str (std::string ());

						if (point_wrapper.selected) {
							name << "*";
						}

						name << "point " << (point_wrapper.index) << " (" << 
							std::setprecision (2) << pos.x << "; " << 
							std::setprecision (2) << pos.y << "; " <<
							std::setprecision (2) << pos.z << ")";
						
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
				segment->set_color (get_color ());
				segment->integrate (delta_time);
			}
		}

		if (m_drag && m_drag_index >= 0 && m_drag_index < m_bezier_points.size ()) {
			t_set_mouse_lock (true);

			point_wrapper & wrapper = m_bezier_points[m_drag_index];
			point_ptr point = wrapper.point;

			wrapper.selected = true;
			wrapper.point->set_selected (true);

			const auto & camera = get_scene ().get_camera ();

			glm::vec3 plane_normal = normalize (camera.get_position () - camera.get_target ());
			glm::vec3 direction = get_scene ().get_mouse_direction ();

			float nt = glm::dot ((m_drag_start - camera.get_position ()), plane_normal);
			float dt = glm::dot (direction, plane_normal);

			point->set_translation ((nt / dt) * direction + camera.get_position ());

			// we need to calculate the new de boor points
			// to do this we need to first know which segment this point belongs to
			int left = (wrapper.index - 1) / 3;
			int right = wrapper.index / 3;

			if (left < 0) {
				left = right;
			} else if (right >= m_segments.size ()) {
				right = left;
			}

			m_calc_deboor_points (left);

			if (right != left) {
				m_calc_deboor_points (right);
			}
		} else {
			t_set_mouse_lock (false);
		}

		curve_base::integrate (delta_time);
	}

	void bspline_curve::m_calc_deboor_points (int segment) {
		int base = segment * 3;
		glm::vec3 p0 = m_bezier_points[base + 0].point->get_translation ();
		glm::vec3 p1 = m_bezier_points[base + 1].point->get_translation ();
		glm::vec3 p2 = m_bezier_points[base + 2].point->get_translation ();
		glm::vec3 p3 = m_bezier_points[base + 3].point->get_translation ();

		glm::vec4 curve_x_bz = { p0.x, p1.x, p2.x, p3.x };
		glm::vec4 curve_y_bz = { p0.y, p1.y, p2.y, p3.y };
		glm::vec4 curve_z_bz = { p0.z, p1.z, p2.z, p3.z };

		glm::vec4 curve_x_bs = BEZIER_TO_BSPLINE * curve_x_bz;
		glm::vec4 curve_y_bs = BEZIER_TO_BSPLINE * curve_y_bz;
		glm::vec4 curve_z_bs = BEZIER_TO_BSPLINE * curve_z_bz;

		const auto & points = t_get_points ();

		for (int i = 0; i < 4; ++i) {
			auto point = points[segment + i].point.lock ();
			if (point) {
				point->set_translation ({ curve_x_bs[i], curve_y_bs[i], curve_z_bs[i] });
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

	const object_serializer_base & bspline_curve::get_serializer () const {
		return generic_object_serializer<bspline_curve>::get_instance ();
	}

	bool bspline_curve::on_mouse_button (int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			if (action == GLFW_PRESS) {
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
					m_begin_drag (*sel);
				}

				return (sel != nullptr);
			} else if (action == GLFW_RELEASE) {
				if (m_drag) {
					m_end_drag ();
					return true;
				}
			}
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
		int point_index = 0;

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

					m_bezier_points.push_back ({ p1, false, point_index++ });
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
				
				m_bezier_points.push_back ({ p2, false, point_index++ });
				m_bezier_points.push_back ({ p3, false, point_index++ });
				m_bezier_points.push_back ({ p4, false, point_index++ });

				prev_end = p4;

				auto segment = std::make_shared<bezier_segment_gpu> (
					m_shader1, m_shader2, p1, p2, p3, p4
				);

				segment->set_showing_polygon (is_show_polygon ());
				segment->set_color (get_color ());

				m_segments.push_back (std::move (segment));
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

	void bspline_curve::m_begin_drag (point_wrapper & wrapper) {
		m_drag = true;
		m_drag_index = wrapper.index;
		m_drag_start = wrapper.point->get_translation ();
	}

	void bspline_curve::m_end_drag () {
		m_drag = false;
		m_drag_index = -1;
	}
}
