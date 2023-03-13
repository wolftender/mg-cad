#include "gui.hpp"
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

	const float_matrix_t & scene_obj_t::get_matrix () const {
		float_matrix_t world = make_identity ();

		world = world * make_translation (m_translation);
		world = world * make_rotation_z (m_euler_angles[2]);
		world = world * make_rotation_y (m_euler_angles[1]);
		world = world * make_rotation_x (m_euler_angles[0]);

		return world;
	}

	void scene_obj_t::configure () {
		if (ImGui::CollapsingHeader ("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::vector_editor ("Translation", m_translation);
			gui::vector_editor ("Rotation", m_euler_angles);
			gui::vector_editor ("Scale", m_scale);
		}
	}
}