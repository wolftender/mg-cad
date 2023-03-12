#include "object.hpp"

namespace mini {
	scene_obj_t::scene_obj_t (const std::string & type_name) {
		m_type_name = type_name;
	}

	scene_obj_t::~scene_obj_t () { }

	const std::string & scene_obj_t::get_type_name () const {
		return m_type_name;
	}

	const float_vector_t & scene_obj_t::get_translation () const {
		return m_translation;
	}

	const float_vector_t & scene_obj_t::get_euler_angles () const {
		return m_euler_angles;
	}

	const float_vector_t & scene_obj_t::get_scale () const {
		return m_scale;
	}

	void scene_obj_t::set_translation (const float_vector_t & translation) {
		m_translation = translation;
	}

	void scene_obj_t::set_euler_angles (const float_vector_t & euler_angles) {
		m_euler_angles = euler_angles;
	}

	void scene_obj_t::set_scale (const float_vector_t & scale) {
		m_scale = scale;
	}

	void scene_obj_t::configure () {
		// todo: render common properties here
	}
}