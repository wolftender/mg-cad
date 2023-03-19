#include "camera.hpp"

namespace mini {
	float camera::get_fov () const {
		return m_fov;
	}

	float camera::get_near () const {
		return m_near;
	}

	float camera::get_far () const {
		return m_far;
	}

	float camera::get_aspect () const {
		return m_aspect;
	}

	const glm::mat4x4 & camera::get_view_matrix () const {
		return m_view;
	}

	const glm::mat4x4 & camera::get_projection_matrix () const {
		return m_projection;
	}

	const glm::mat4x4 & camera::get_view_inverse () const {
		return m_view_inv;
	}

	const glm::mat4x4 & camera::get_projection_inverse () const {
		return m_projection_inv;
	}

	const glm::vec3 & camera::get_position () const {
		return m_position;
	}

	const glm::vec3 & camera::get_target () const {
		return m_target;
	}

	void camera::set_fov (float fov) {
		m_fov = fov;
		m_recalculate_projection ();
	}

	void camera::set_near (float near) {
		m_near = near;
		m_recalculate_projection ();
	}

	void camera::set_far (float far) {
		m_far = far;
		m_recalculate_projection ();
	}

	void camera::set_aspect (float aspect) {
		m_aspect = aspect;
		m_recalculate_projection ();
	}

	void camera::set_position (const glm::vec3 & position) {
		m_position = position;
		m_recalculate_view ();
	}

	void camera::set_target (const glm::vec3 & target) {
		m_target = target;
		m_recalculate_view ();
	}

	camera::camera () {
		m_aspect = 1.0f;
		m_far = 100.0f;
		m_near = 0.1f;
		m_fov = 3.141592f / 3.0f;
		m_position = { 0.0f, 0.0f, 1.0f };
		m_target = { 0.0f, 0.0f, 0.0f };
		
		m_recalculate_view ();
		m_recalculate_projection ();
	}

	camera::camera (const glm::vec3 & position, const glm::vec3 & target) {
		m_aspect = 1.0f;
		m_far = 100.0f;
		m_near = 0.1f;
		m_fov = 3.141592f / 3.0f;
		m_position = position;
		m_target = target;

		m_recalculate_view ();
		m_recalculate_projection ();
	}

	void camera::m_recalculate_projection () {
		float t = glm::tan (m_fov / 2.0f);
		float a = m_aspect;
		float zm = m_far - m_near;
		float zp = m_far + m_near;

		m_projection = {
			1.0f / (t * a), 0.0f,     0.0f,     0.0f,
			0.0f,           1.0f / t, 0.0f,     0.0f,
			0.0f,           0.0f,     -zp / zm, -1.0f,
			0.0f,           0.0f,     -(2.0f * m_far * m_near) / zm,    0.0f
		};

		m_projection_inv = glm::inverse (m_projection);

		/*m_projection = glm::perspective (m_fov, m_aspect, m_near, m_far);
		m_projection_inv = glm::inverse (m_projection);*/
	}

	void camera::m_recalculate_view () {
		glm::vec3 dir = m_position - m_target;
		glm::vec3 up = { 0.0f, 1.0f, 0.0f };

		glm::vec3 f = glm::normalize (dir);
		glm::vec3 r = glm::normalize (glm::cross (up, f));
		glm::vec3 u = glm::normalize (glm::cross (f, r));

		m_view = {
			r[0], u[0], f[0], 0.0f,
			r[1], u[1], f[1], 0.0f,
			r[2], u[2], f[2], 0.0f,
			-glm::dot (r, m_position), -glm::dot (u, m_position), -glm::dot (f, m_position), 1.0f
		};

		m_view_inv = glm::inverse (m_view);

		/*m_view = glm::lookAt (m_position, m_target, {0.0f, 1.0f, 0.0f});
		m_view_inv = glm::inverse (m_view);*/
	}
}