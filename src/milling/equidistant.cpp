#include <glm/glm.hpp>

#include "milling/equidistant.hpp"

namespace mini {
	equidistant_surface::equidistant_surface(
		std::shared_ptr<const differentiable_surface_base> surface, 
		float dist) {

		m_surface = surface;
		m_dist = dist;
	}

	equidistant_surface::~equidistant_surface() {
	}

	float equidistant_surface::get_min_u() const {
		return m_surface->get_min_u();
	}

	float equidistant_surface::get_max_u() const {
		return m_surface->get_max_u();
	}

	float equidistant_surface::get_min_v() const {
		return m_surface->get_min_v();
	}

	float equidistant_surface::get_max_v() const {
		return m_surface->get_max_v();
	}

	glm::vec3 equidistant_surface::sample(float u, float v) const {
		auto du = m_surface->ddu(u,v);
		auto dv = m_surface->ddv(u,v);
		auto n = glm::normalize(glm::cross(du,dv));

		return m_surface->sample(u,v) + n * m_dist;
	}

	glm::vec3 equidistant_surface::normal(float u, float v) const {
		return m_surface->normal(u, v);
	}

	glm::vec3 equidistant_surface::ddu(float u, float v) const {
		return m_surface->ddu(u, v);
	}

	glm::vec3 equidistant_surface::ddv(float u, float v) const {
		return m_surface->ddv(u, v);
	}

	bool equidistant_surface::is_u_wrapped() const {
		return m_surface->is_u_wrapped();
	}

	bool equidistant_surface::is_v_wrapped() const {
		return m_surface->is_v_wrapped();
	}
}