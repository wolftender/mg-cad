#include "gui.hpp"
#include "object.hpp"

namespace mini {
	scene_obj_t::scene_obj_t (const std::string & type_name, bool movable, bool rotatable, bool scalable) {
		m_type_name = type_name;

		m_euler_angles = { 0.0f, 0.0f, 0.0f };
		m_translation = { 0.0f, 0.0f, 0.0f };
		m_scale = { 1.0f, 1.0f, 1.0f };
	}

	scene_obj_t::~scene_obj_t () { }

	const std::string & scene_obj_t::get_type_name () const {
		return m_type_name;
	}

	const glm::vec3 & scene_obj_t::get_translation () const {
		return m_translation;
	}

	const glm::vec3 & scene_obj_t::get_euler_angles () const {
		return m_euler_angles;
	}

	const glm::vec3 & scene_obj_t::get_scale () const {
		return m_scale;
	}

	bool scene_obj_t::is_selected () const {
		return m_selected;
	}

	bool scene_obj_t::is_rotatable () const {
		return m_rotatable;
	}

	bool scene_obj_t::is_scalable () const {
		return m_scalable;
	}

	bool scene_obj_t::is_movable () const {
		return m_movable;
	}

	void scene_obj_t::set_translation (const glm::vec3 & translation) {
		m_translation = translation;
	}

	void scene_obj_t::set_euler_angles (const glm::vec3 & euler_angles) {
		m_euler_angles = euler_angles;
	}

	void scene_obj_t::set_scale (const glm::vec3 & scale) {
		m_scale = scale;
	}

	void scene_obj_t::set_selected (bool selected) {
		m_selected = selected;
	}

	glm::mat4x4 scene_obj_t::get_matrix () const {
		glm::mat4x4 world (1.0f);

		if (m_movable) {
			world = glm::translate (world, m_translation);
		}
		
		if (m_rotatable) {
			world = glm::rotate (world, m_euler_angles[0], { 1.0f, 0.0f, 0.0f });
			world = glm::rotate (world, m_euler_angles[1], { 0.0f, 1.0f, 0.0f });
			world = glm::rotate (world, m_euler_angles[2], { 0.0f, 0.0f, 1.0f });
		}

		if (m_scalable) {
			world = glm::scale (world, m_scale);
		}

		return world;
	}

	void scene_obj_t::configure () {
		if (ImGui::CollapsingHeader ("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (m_movable) {
				gui::vector_editor ("Translation", m_translation);
			}

			if (m_rotatable) {
				gui::vector_editor ("Rotation", m_euler_angles);
			}
			
			if (m_scalable) {
				gui::vector_editor ("Scale", m_scale);
			}
		}
	}
}