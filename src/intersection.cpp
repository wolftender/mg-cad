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

	constexpr std::array<glm::vec2, 5> c_offsets = { glm::vec2{0.0f, 0.0f}, {0.5f, 0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 0.0f} };

	intersection_controller::intersection_controller(
		scene_controller_base & scene, std::shared_ptr<resource_store> store, bool from_cursor) : 
		m_scene(scene),
		m_from_cursor(from_cursor) {

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

		std::cout << "finding first intersection point..." << std::endl;
		glm::vec2 p1, p2;
		bool starting_points_found = false;

		if (m_from_cursor) {
			glm::vec2 s1, s2;

			m_start_by_cursor(s1, s2);
			if (m_find_starting_points(p1, p2, s1, s2)) {
				starting_points_found = true;
			} else {
				std::cout << "warning! failed to find intersection by cursor!" << std::endl;
			}
		}

		if (!starting_points_found) {
			for (const auto& sp : c_offsets) {
				if (m_find_starting_points(p1, p2, sp, sp)) {
					starting_points_found = true;
					break;
				}
			}
		}

		if (!starting_points_found) {
			std::cout << "no intersection found between surfaces" << std::endl;
			return;
		}

		std::cout << "begin tracing intersection..." << std::endl;
		m_trace_intersection(p1, p2);
	}

	intersection_controller::intersection_controller(
		scene_controller_base& scene, 
		generic_surface_ptr surface1, 
		generic_surface_ptr surface2, 
		std::shared_ptr<resource_store> store, 
		bool from_cursor) :
		m_scene(scene),
		m_from_cursor(from_cursor) {

		m_store = store;
		m_surface1 = surface1;
		m_surface2 = surface2;

		std::cout << "finding first intersection point..." << std::endl;
		glm::vec2 p1, p2;
		bool starting_points_found = false;

		if (m_from_cursor) {
			glm::vec2 s1, s2;

			m_start_by_cursor(s1, s2);
			if (m_find_starting_points(p1, p2, s1, s2)) {
				starting_points_found = true;
			} else {
				std::cout << "warning! failed to find intersection by cursor!" << std::endl;
			}
		}

		if (!starting_points_found) {
			for (const auto& sp : c_offsets) {
				if (m_find_starting_points(p1, p2, sp, sp)) {
					starting_points_found = true;
					break;
				}
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

	const intersection_controller::result_t& intersection_controller::get_verbose() const {
		return m_result;
	}

	inline void intersection_controller::m_wrap_coordinates(const generic_surface_ptr& surface, float& u, float& v) const {
		bool is_u_wrapped = surface->is_u_wrapped();
		bool is_v_wrapped = surface->is_v_wrapped();

		const auto min_u = surface->get_min_u();
		const auto min_v = surface->get_min_v();
		const auto max_u = surface->get_max_u();
		const auto max_v = surface->get_max_v();

		if (u > max_u) {
			if (is_u_wrapped) {
				u = u - max_u;
			} else {
				u = max_u;
			}
		} else if (u < min_u) {
			if (is_u_wrapped) {
				u = u + max_u;
			} else {
				u = min_u;
			}
		}

		if (v > max_v) {
			if (is_v_wrapped) {
				v = v - max_v;
			} else {
				v = max_v;
			}
		} else if (v < min_v) {
			if (is_v_wrapped) {
				v = v + max_v;
			} else {
				v = min_v;
			}
		}
	}

	void intersection_controller::m_start_by_cursor(glm::vec2& s1, glm::vec2& s2) const {
		std::cout << "finding initial points..." << std::endl;
		glm::vec2 proj1, proj2;

		const auto& cursor = m_scene.get_cursor_pos();
		auto dist1 = 100000.0f;
		auto dist2 = 100000.0f;

		for (const auto& sp : c_offsets) {
			glm::vec2 cproj1, cproj2;

			cproj1 = m_get_cursor_projection(m_surface1, sp);
			cproj2 = m_get_cursor_projection(m_surface2, sp);

			auto cdist1 = glm::distance(m_surface1->sample(cproj1.x, cproj1.y), cursor);
			auto cdist2 = glm::distance(m_surface2->sample(cproj2.x, cproj2.y), cursor);

			if (cdist1 < dist1) {
				proj1 = cproj1;
				dist1 = cdist1;
			}

			if (cdist2 < dist2) {
				proj2 = cproj2;
				dist2 = cdist2;
			}
		}

		s1 = proj1;
		s2 = proj2;
	}

	glm::vec2 intersection_controller::m_get_cursor_projection(const generic_surface_ptr& surface, const glm::vec2& s) const {
		const auto& cursor = m_scene.get_cursor_pos();

		const auto ddu = [&](float u, float v) -> float {
			const auto der = -2.0f * (cursor - surface->sample(u, v)) * surface->ddu(u, v);
			return der.x + der.y + der.z;
		};

		const auto ddv = [&](float u, float v) -> float {
			const auto der = -2.0f * (cursor - surface->sample(u, v)) * surface->ddv(u, v);
			return der.x + der.y + der.z;
		};

		glm::vec2 current = s;
		glm::vec2 previous = current;

		constexpr float c_start_step = 0.005f;
		constexpr float c_start_epsilon = 0.0001f;
		constexpr int c_max_steps = 200;

		float step = c_start_step;
		float epsilon = c_start_epsilon;
		float d = 0.0f;

		int num_steps = 0, epsd = 0;

		do {
			glm::vec2 direction = {
				ddu(current.x, current.y),
				ddv(current.x, current.y)
			};

			direction = direction * step;
			previous = current;

			current = current - direction;

			m_wrap_coordinates(surface, current.x, current.y);

			float du = current.x - previous.x;
			float dv = current.y - previous.y;

			d = glm::sqrt(du * du + dv * dv);

			num_steps++;
		} while (num_steps < c_max_steps);

		return current;
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
			const auto der = 2.0f * m_surface2->ddu(p, q) * (m_surface2->sample(p, q) - m_surface1->sample(u, v));
			return der.x + der.y + der.z;
		};

		const auto ddq = [this](float u, float v, float p, float q) -> float {
			const auto der = 2.0f * m_surface2->ddv(p, q) * (m_surface2->sample(p, q) - m_surface1->sample(u, v));
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
		} while ((d1 > epsilon || d2 > epsilon) && num_steps < c_max_steps);

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

	void intersection_controller::m_trace_intersection(const glm::vec2 & s1, const glm::vec2 & s2) {		
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

		const auto trace = [&](
			glm::vec2 p1, 
			glm::vec2 p2, 
			float sign, 
			std::vector<glm::vec2> & s1_out, 
			std::vector<glm::vec2> & s2_out,
			std::vector<glm::vec2> & d1_out,
			std::vector<glm::vec2> & d2_out) {

			P0 = m_surface1->sample(p1.x, p1.y);

			for (int i = 0; i < 1200; ++i) {
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

				glm::vec2 d1, d2;
				d1.x = x.x - p1.x;
				d1.y = x.y - p1.y;
				d2.x = x.z - p2.x;
				d2.y = x.w - p2.y;

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
				d1_out.push_back(d1);
				d2_out.push_back(d2);
			}
		};

		std::vector<glm::vec2> s11, s21, d11, d21;
		std::vector<glm::vec2> s12, s22, d12, d22;

		// reserve buffer for all
		s11.reserve(2000);
		s21.reserve(2000);
		s12.reserve(2000);
		s22.reserve(2000);
		d11.reserve(2000);
		d21.reserve(2000);
		d12.reserve(2000);
		d22.reserve(2000);

		trace(s1, s2, +1.0f, s11, s21, d11, d21);
		trace(s1, s2, -1.0f, s12, s22, d12, d22);

		std::vector<glm::vec3> curve_points1, curve_points2;
		for (const auto& p : s11) {
			curve_points1.push_back(m_surface1->sample(p.x, p.y));
		}

		for (const auto& p : s12) {
			curve_points2.push_back(m_surface1->sample(p.x, p.y));
		}

		if (m_surface1->is_trimmable()) {
			auto& domain = m_surface1->get_trimmable_domain();

			domain.trim_directions(s1, d11);
			domain.trim_directions(s1, d12);

			domain.update_texture();
		}

		if (m_surface2->is_trimmable()) {
			auto& domain = m_surface2->get_trimmable_domain();

			domain.trim_directions(s2, d21);
			domain.trim_directions(s2, d22);

			domain.update_texture();
		}

		auto curve1 = std::make_shared<curve>(m_scene, m_store->get_line_shader(), curve_points1);
		auto curve2 = std::make_shared<curve>(m_scene, m_store->get_line_shader(), curve_points2);

		curve1->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });
		curve2->set_color({ 1.0f, 0.0f, 0.0f, 1.0f });

		curve1->set_line_width(3.0f);
		curve2->set_line_width(3.0f);

		//m_scene.add_object("intersection_curve", curve1);
		//m_scene.add_object("intersection_curve", curve2);

		m_result.s11 = s11;
		m_result.s21 = s21;
		m_result.d11 = d11;
		m_result.d21 = d21;

		m_result.s12 = s12;
		m_result.s22 = s22;
		m_result.d12 = d12;
		m_result.d22 = d22;
	}
}