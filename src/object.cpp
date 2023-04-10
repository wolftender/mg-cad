#include "gui.hpp"
#include "object.hpp"

namespace mini {
	scene_obj_t::scene_obj_t (scene_controller_base & scene, const std::string & type_name, bool movable, bool rotatable, bool scalable) :
		m_scene (scene) {
		m_type_name = type_name;

		m_euler_angles = { 0.0f, 0.0f, 0.0f };
		m_translation = { 0.0f, 0.0f, 0.0f };
		m_scale = { 1.0f, 1.0f, 1.0f };

		m_movable = movable;
		m_rotatable = rotatable;
		m_scalable = scalable;

		m_selected = false;
		m_disposed = false;
	}

	scene_obj_t::~scene_obj_t () { }

	void scene_obj_t::m_listen (signal_event_t sig, std::shared_ptr<scene_obj_t> listener) {
		auto & list = m_listeners[static_cast<int>(sig)];

		for (auto iter = list.begin (); iter != list.end (); ) {
			auto el = iter->lock ();
			if (!el) {
				iter = list.erase (iter);
				continue;
			}

			if (el == listener) {
				return;
			}

			iter++;
		}

		list.push_back (listener);
	}

	void scene_obj_t::m_ignore (signal_event_t sig, std::shared_ptr<scene_obj_t> listener) {
		auto & list = m_listeners[static_cast<int>(sig)];

		for (auto iter = list.begin (); iter != list.end (); ) {
			auto el = iter->lock ();
			if (!el) {
				iter = list.erase (iter);
				continue;
			}

			if (el == listener) {
				list.erase (iter);
				return;
			}

			iter++;
		}
	}

	void scene_obj_t::t_listen (signal_event_t sig, scene_obj_t & target) {
		target.m_listen (sig, shared_from_this ());
	}

	void scene_obj_t::t_ignore (signal_event_t sig, scene_obj_t & target) {
		target.m_ignore (sig, shared_from_this ());
	}

	void scene_obj_t::m_notify (signal_event_t sig) {
		auto & set = m_listeners[static_cast<int>(sig)];
		for (auto iter = set.begin (); iter != set.end (); ) {
			auto listener = iter->lock();

			if (listener) {
				listener->m_receive (sig, *this);
				iter++;
			} else {
				iter = set.erase (iter);
			}
		}
	}

	void scene_obj_t::m_receive (signal_event_t sig, scene_obj_t & emitter) {
		int sig_id = static_cast<int>(sig);
		if (m_handlers[sig_id]) {
			m_handlers[sig_id] (sig, emitter);
		}
	}

	void scene_obj_t::t_set_handler (signal_event_t sig, signal_handler_t handler) {
		int sig_id = static_cast<int>(sig);
		m_handlers[sig_id] = handler;
	}

	void scene_obj_t::notify_object_created (std::shared_ptr<scene_obj_t> object) {
		t_on_object_created (object);
	}

	void scene_obj_t::notify_object_selected (std::shared_ptr<scene_obj_t> object) {
		t_on_object_selected (object);
	}

	void scene_obj_t::notify_object_deleted (std::shared_ptr<scene_obj_t> object) {
		t_on_object_deleted (object);
	}

	const std::string & scene_obj_t::get_type_name () const {
		return m_type_name;
	}

	const std::string & scene_obj_t::get_name () const {
		return m_name;
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

	bool scene_obj_t::is_disposed () const {
		return m_disposed;
	}

	scene_controller_base & scene_obj_t::get_scene () const {
		return m_scene;
	}

	void scene_obj_t::dispose () {
		m_disposed = true;
	}

	void scene_obj_t::set_translation (const glm::vec3 & translation) {
		m_translation = translation;
		m_notify (signal_event_t::moved);
	}

	void scene_obj_t::set_euler_angles (const glm::vec3 & euler_angles) {
		m_euler_angles = euler_angles;
		m_notify (signal_event_t::rotated);
	}

	void scene_obj_t::set_scale (const glm::vec3 & scale) {
		m_scale = scale;
		m_notify (signal_event_t::scaled);
	}

	void scene_obj_t::set_selected (bool selected) {
		if (selected != m_selected) {
			m_selected = selected;
			t_on_selection (selected);

			m_notify (signal_event_t::selected);
		}		
	}

	void scene_obj_t::set_name (const std::string & name) {
		m_name = name;
	}

	glm::mat4x4 scene_obj_t::compose_matrix (const glm::vec3 & translation, const glm::vec3 & euler_angles, const glm::vec3 & scale) const {
		glm::mat4x4 world (1.0f);

		if (m_movable) {
			//world = glm::translate (world, translation);
			world = world * make_translation (translation);
		}

		if (m_rotatable) {
			//world = glm::rotate (world, euler_angles[2], { 0.0f, 0.0f, 1.0f });
			//world = glm::rotate (world, euler_angles[1], { 0.0f, 1.0f, 0.0f });
			//world = glm::rotate (world, euler_angles[0], { 1.0f, 0.0f, 0.0f });
			world = world * make_rotation_z (euler_angles[2]);
			world = world * make_rotation_y (euler_angles[1]);
			world = world * make_rotation_x (euler_angles[0]);
		}

		if (m_scalable) {
			//world = glm::scale (world, scale);
			world = world * make_scale (scale);
		}

		return world;
	}

	glm::mat4x4 scene_obj_t::get_matrix () const {
		return compose_matrix (m_translation, m_euler_angles, m_scale);
	}

	void scene_obj_t::integrate (float delta_time) { }

	void scene_obj_t::configure () {
		if (ImGui::CollapsingHeader ("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
			if (m_movable) {
				if (gui::vector_editor ("Translation", m_translation)) {
					m_notify (signal_event_t::moved);
				}
			}

			if (m_rotatable) {
				if (gui::vector_editor ("Rotation", m_euler_angles)) {
					m_notify (signal_event_t::rotated);
				}
			}
			
			if (m_scalable) {
				if (gui::vector_editor ("Scale", m_scale)) {
					m_notify (signal_event_t::scaled);
				}
			}
		}
	}

	bool scene_obj_t::hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const {
		return false;
	}

	hit_test_data_t::hit_test_data_t (mini::camera & cam, const glm::vec2 & mouse_screen, 
		const glm::vec2 & screen_res, const glm::vec3 & mouse_ray) :
		camera (cam), 
		mouse_screen (mouse_screen),
		screen_res (screen_res),
		mouse_ray (mouse_ray) {
	}
}