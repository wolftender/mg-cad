#include <array>

#include "intersection.hpp"
#include "curve.hpp"

namespace mini {
	// gaussian method
	// solve equation Ax = b
	inline void gauss(const glm::mat4x4 & A, const glm::vec4 & b, glm::vec4 & x) {
		glm::mat4x4 M = glm::transpose(A);
		glm::vec4 c = b;

		for (int k = 0; k < 4; ++k) {
			int pivot = k;
			float pivot_val = 0.0f;

			// find pivot
			for (int i = k; i < 4; ++i) {
				float a = fabsf(M[i][k]);
				if (a > pivot_val) {
					pivot = i;
					pivot_val = a;
				}
			}

			// swap rows k and pivot
			if (pivot != k) {
				for (int i = k; i < 4; ++i) {
					std::swap(M[pivot][i], M[k][i]);
				}

				std::swap(c[pivot], c[k]);
			}

			// elimination
			for (int i = k + 1; i < 4; ++i) {
				float m = M[i][k] / M[k][k];
				for (int j = k; j < 4; ++j) {
					M[i][j] -= m * M[k][j];
				}

				c[i] -= m * c[k];
			}
		}

		// back substitution
		for (int k = 3; k >= 0; --k) {
			float sum = c[k];
			for (int j = k + 1; j < 4; ++j) {
				sum -= M[k][j] * x[j];
			}

			x[k] = sum / M[k][k];
		}
	}

	intersection_controller::intersection_controller(
		scene_controller_base & scene, std::shared_ptr<resource_store> store) : m_scene(scene) {

		m_store = store;

		for (auto iter = m_scene.get_selected_objects(); iter->has(); iter->next()) {
			auto surface = std::dynamic_pointer_cast<differentiable_surface_base> (iter->get_object());

			if (surface) {
				if (!m_surface1) {
					m_surface1 = surface;
				} else if (!m_surface2) {
					m_surface2 = surface;
					break;
				}
			}
		}

		glm::vec2 p1, p2;
		std::array<glm::vec2, 3> s = {glm::vec2{0.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 1.0f}};

		bool starting_points_found = false;
		for (int i = 0; i < 3; ++i) {
			if (m_find_starting_points(p1, p2, s[i], s[i])) {
				starting_points_found = true;
				break;
			}
		}

		if (!starting_points_found) {
			std::cout << "no intersection found between surfaces" << std::endl;
			return;
		}

		std::cout << "begin tracing intersection..." << std::endl;
		m_trace_intersection(p1, p2);
	}

	intersection_controller::~intersection_controller() { }

	inline void intersection_controller::m_wrap_coordinates(const generic_surface_ptr& surface, float& u, float& v) const {
		bool is_u_wrapped = surface->is_u_wrapped();
		bool is_v_wrapped = surface->is_v_wrapped();

		if (u > 1.0f) {
			if (is_u_wrapped) {
				u = u - 1.0f;
			} else {
				u = 1.0f;
			}
		} else if (u < 0.0f) {
			if (is_u_wrapped) {
				u = u + 1.0f;
			} else {
				u = 0.0f;
			}
		}

		if (v > 1.0f) {
			if (is_v_wrapped) {
				v = v - 1.0f;
			} else {
				v = 1.0f;
			}
		} else if (v < 0.0f) {
			if (is_v_wrapped) {
				v = v + 1.0f;
			} else {
				v = 0.0f;
			}
		}
	}

	bool intersection_controller::m_find_starting_points(glm::vec2 & p1, glm::vec2 & p2, const glm::vec2 & s1, const glm::vec2 & s2) const {
		if (!m_surface1 || !m_surface2) {
			return false;
		}

		// surface1 is parameterized by u,v
		// surface2 is parameterized by p,q
		// we look using gradients for such (u,v,p,q) that the distance function
		// d(u,v,p,q) = |S1(u,v) - S2(p,q)|
		// is minimal

		const auto ddu = [this](float u, float v, float p, float q) -> float {
			const auto der = 2.0f * m_surface1->ddu(u, v) * (m_surface1->sample(u, v) - m_surface2->sample(p, q));
			return der.x + der.y + der.z;
		};

		const auto ddv = [this](float u, float v, float p, float q) -> float {
			const auto der = 2.0f * m_surface1->ddv(u, v) * (m_surface1->sample(u, v) - m_surface2->sample(p, q));
			return der.x + der.y + der.z;
		};

		const auto ddp = [this](float u, float v, float p, float q) -> float {
			const auto der = 2.0f * m_surface2->ddu(u, v) * (m_surface2->sample(p, q) - m_surface1->sample(u, v));
			return der.x + der.y + der.z;
		};

		const auto ddq = [this](float u, float v, float p, float q) -> float {
			const auto der = 2.0f * m_surface2->ddv(u, v) * (m_surface2->sample(p, q) - m_surface1->sample(u, v));
			return der.x + der.y + der.z;
		};

		const auto d = [this](float u, float v, float p, float q) -> float {
			const auto dist = m_surface1->sample(u,v) - m_surface2->sample(p,q);
			return glm::dot(dist, dist);
		};
		

		glm::vec4 current = { s1.x, s1.y, s2.x, s2.y };
		glm::vec4 previous = current;

		constexpr float c_start_step = 0.005f;
		constexpr float c_start_epsilon = 0.0001f;
		constexpr float c_step_mult = 1.0f / 2.0f;
		constexpr float c_eps_mult = 1.0f / 10.0f;
		constexpr int c_max_steps = 1000;
		constexpr int c_max_epsd = 5;

		float step = c_start_step;
		float epsilon = c_start_epsilon;
		float d1 = 0.0f, d2 = 0.0f;

		int num_steps = 0, epsd = 0;

		do {
			glm::vec4 direction = {
				ddu(current[0], current[1], current[2], current[3]),
				ddv(current[0], current[1], current[2], current[3]),
				ddp(current[0], current[1], current[2], current[3]),
				ddq(current[0], current[1], current[2], current[3])
			};

			direction = direction * step;
			previous = current;

			current = current - direction;

			m_wrap_coordinates(m_surface1, current.x, current.y);
			m_wrap_coordinates(m_surface2, current.z, current.w);
			
			float du = current[0] - previous[0];
			float dv = current[1] - previous[1];
			float dp = current[2] - previous[2];
			float dq = current[3] - previous[3];

			d1 = glm::sqrt(du*du + dv*dv);
			d2 = glm::sqrt(dp*dp + dq*dq);

			num_steps++;
			if (d1 < epsilon && d2 < epsilon && epsd < c_max_epsd) {
				epsd++;
				epsilon = epsilon * c_eps_mult;
				step = step * c_step_mult;

				std::cout << "at step " << num_steps << ": " << epsd << " " << epsilon << " " << step << std::endl;
			}

			//std::cout << "step " << direction.x << ", " << direction.y << ", " << direction.z << ", " << direction.w << std::endl;
			//std::cout << "pos " << current.x << ", " << current.y << ", " << current.z << ", " << current.w << std::endl;
		} while (d1 > epsilon || d2 > epsilon && num_steps < c_max_steps);

		// check if this is actually a point of intersection
		auto pos1 = m_surface1->sample(current[0], current[1]);
		auto pos2 = m_surface2->sample(current[2], current[3]);

		auto dist = glm::distance(pos1, pos2);

		if (dist > 0.01f) {
			return false;
		}

		std::cout << dist << std::endl;

		p1 = { current[0], current[1] };
		p2 = { current[2], current[3] };

		const auto point_obj1 = std::make_shared<point_object>(m_scene,
			m_store->get_billboard_s_shader(),
			m_store->get_point_texture());

		const auto point_obj2 = std::make_shared<point_object>(m_scene,
			m_store->get_billboard_s_shader(),
			m_store->get_point_texture());

		point_obj1->set_translation(pos1);
		point_obj2->set_translation(pos2);

		point_obj1->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
		point_obj2->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });

		m_scene.add_object("debug_point", point_obj1);
		m_scene.add_object("debug_point", point_obj2);

		return true;
	}

	void intersection_controller::m_trace_intersection(const glm::vec2 & s1, const glm::vec2 & s2) const {		
		glm::vec3 t;
		glm::vec3 P0;

		const float d = 0.01f;

		const auto f = [&](float u, float v, float p, float q) -> glm::vec4 {
			auto P = m_surface1->sample(u,v);
			auto Q = m_surface2->sample(p,q);

			return {
				P.x - Q.x,
				P.y - Q.y,
				P.z - Q.z,
				glm::dot(P - P0, t) - d
			};
		};

		const auto J = [&](float u, float v, float p, float q) -> glm::mat4x4 {
			auto dPdu = m_surface1->ddu(u, v);
			auto dPdv = m_surface1->ddv(u, v);
			auto dQdp = m_surface2->ddu(p, q);
			auto dQdq = m_surface2->ddv(p, q);

			float m14 = 0.0f;
			float m24 = 0.0f;
			float m34 = 0.0f;
			float m44 = 0.0f;

			for (int i = 0; i < 3; ++i) {
				m14 = m14 + t[i] * dPdu[i];
				m24 = m24 + t[i] * dPdv[i];
				m34 = m34 + t[i] * dQdp[i];
				m44 = m44 + t[i] * dQdq[i];
			}

			glm::mat4x4 jacobian{
				dPdu.x, dPdu.y, dPdu.z, m14,
				dPdv.x, dPdv.y, dPdv.z, m24,
				-dQdp.x, -dQdp.y, -dQdp.z, m34,
				-dQdq.x, -dQdq.y, -dQdq.z, m44
			};

			return jacobian;
		};

		const auto trace = [&](glm::vec2 p1, glm::vec2 p2, float sign, std::vector<glm::vec2> & s1_out, std::vector<glm::vec2> & s2_out) {
			P0 = m_surface1->sample(p1.x, p1.y);

			for (int i = 0; i < 1000; ++i) {
				auto n1 = m_surface1->normal(p1.x, p1.y);
				auto n2 = m_surface2->normal(p2.x, p2.y);

				t = sign * glm::normalize(glm::cross(n1, n2));

				// newton method
				glm::vec4 x = { p1.x, p1.y, p2.x, p2.y };

				for (int j = 0; j < 50; ++j) {
					auto value = f(x[0], x[1], x[2], x[3]);
					auto jacobian = J(x[0], x[1], x[2], x[3]);

					auto dist = glm::length(value);
					if (dist < 0.0001f) {
						break;
					}

					//glm::vec4 next = x - glm::inverse(jacobian) * value;

					glm::vec4 s;
					gauss(jacobian, -value, s);

					x = x + s * 0.05f;
				}

				p1.x = x.x;
				p1.y = x.y;
				p2.x = x.z;
				p2.y = x.w;

				m_wrap_coordinates(m_surface1, p1.x, p1.y);
				m_wrap_coordinates(m_surface2, p2.x, p2.y);

				P0 = m_surface1->sample(p1.x, p1.y);
				auto P1 = m_surface2->sample(p2.x, p2.y);
				//std::cout << glm::length(P0 - P1) << std::endl;

				s1_out.push_back(p1);
				s2_out.push_back(p2);
			}
		};

		std::vector<glm::vec2> s11, s21;
		std::vector<glm::vec2> s12, s22;

		trace(s1, s2, +1.0f, s11, s21);
		trace(s1, s2, -1.0f, s12, s22);

		std::vector<glm::vec3> curve_points1, curve_points2;
		for (const auto& p : s11) {
			curve_points1.push_back(m_surface1->sample(p.x, p.y));
		}

		for (const auto& p : s22) {
			curve_points2.push_back(m_surface1->sample(p.x, p.y));
		}

		auto curve1 = std::make_shared<curve>(m_scene, m_store->get_line_shader(), curve_points1);
		auto curve2 = std::make_shared<curve>(m_scene, m_store->get_line_shader(), curve_points1);

		curve1->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
		curve2->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });

		curve1->set_line_width(3.0f);
		curve2->set_line_width(3.0f);

		m_scene.add_object("intersection_curve", curve1);
		m_scene.add_object("intersection_curve", curve2);
	}
}