#include "intersection.hpp"

namespace mini {
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
		glm::vec2 s1 = { 0.5f, 0.5f };
		glm::vec2 s2 = { 0.5f, 0.5f };

		if (!m_find_starting_points(p1, p2, s1, s2)) {
			std::cout << "no intersection found between surfaces" << std::endl;
			return;
		}
	}

	intersection_controller::~intersection_controller() { }

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

		float step = 0.001f;
		float epsilon = 0.00001f;
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

			current = glm::clamp(current - direction, 0.0f, 1.0f);
			
			float du = current[0] - previous[0];
			float dv = current[1] - previous[1];
			float dp = current[2] - previous[2];
			float dq = current[3] - previous[3];

			d1 = glm::sqrt(du*du + dv*dv);
			d2 = glm::sqrt(dp*dp + dq*dq);

			num_steps++;
			if (d1 < epsilon && d2 < epsilon && epsd < 3) {
				epsd++;
				epsilon = epsilon / 5.0f;
				step = step / 5.0f;
			}

			//std::cout << "step " << direction.x << ", " << direction.y << ", " << direction.z << ", " << direction.w << std::endl;
			//std::cout << "pos " << current.x << ", " << current.y << ", " << current.z << ", " << current.w << std::endl;
		} while (d1 > epsilon || d2 > epsilon && num_steps < 500);

		// check if this is actually a point of intersection
		auto pos1 = m_surface1->sample(current[0], current[1]);
		auto pos2 = m_surface2->sample(current[2], current[3]);

		auto dist = glm::distance(pos1, pos2);

		if (dist > 0.01f) {
			return false;
		}

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
}