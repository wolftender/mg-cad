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

	const float_matrix_t & camera::get_view_matrix () const {
		return m_view;
	}

	const float_matrix_t & camera::get_projection_matrix () const {
		return m_projection;
	}

	const float_matrix_t & camera::get_view_inverse () const {
		return m_view_inv;
	}

	const float_matrix_t & camera::get_projection_inverse () const {
		return m_projection_inv;
	}

	const float_vector_t & camera::get_position () const {
		return m_position;
	}

	const float_vector_t & camera::get_target () const {
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

	void camera::set_position (const float_vector_t & position) {
		m_position = position;
		m_recalculate_view ();
	}

	void camera::set_target (const float_vector_t & target) {
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

	camera::camera (const float_vector_t & position, const float_vector_t & target) {
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
		float t = tan (m_fov / 2.0f);
		float a = m_aspect;
		float zm = m_far - m_near;
		float zp = m_far + m_near;

		m_projection = float_matrix_t {
			1.0f / (t * a), 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f / t, 0.0f, 0.0f,
			0.0f, 0.0f, -zp / zm, -(2.0f * m_far * m_near) / zm,
			0.0f, 0.0f, -1.0f, 0.0f
		};

		m_projection_inv = invert (m_projection);
	}

	void camera::m_recalculate_view () {
		float_vector_t dir = m_position - m_target;
		float_vector_t up { 0.0f, 1.0f, 0.0f };

		float_vector_t f = normalize (dir);
		float_vector_t r = normalize (float_vector_t::cross (f, up));
		float_vector_t u = normalize (float_vector_t::cross (f, r));

		float_matrix_t m1{
			r[0], r[1], r[2], 0.0f,
			u[0], u[1], u[2], 0.0f,
			f[0], f[1], f[2], 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		m_view = m1 * make_translation (-m_position[0], -m_position[1], -m_position[2]);
		m_view_inv = invert (m_view);
	}
}