#include "gui.hpp"
#include "object.hpp"

namespace mini {
	uint64_t scene_controller_base::t_parent_object (scene_obj_t & object) {
		if (object.get_id () != 0UL) {
			throw std::runtime_error ("cannot parent this object to more than one scene");
		}

		auto id = m_next_id;
		m_next_id += 100UL;

		object.m_set_id (id);
		return id;
	}

	uint64_t scene_controller_base::t_unparent_object (scene_obj_t & object) {
		auto id = object.get_id ();
		if (id == 0UL) {
			throw std::runtime_error ("this object was not parented to any scene");
		}

		object.m_set_id (0UL);
		return id;
	}

	scene_controller_base::scene_controller_base () {
		m_next_id = 100UL;
	}


	////////////////////////////////////////////

	uint64_t scene_obj_t::s_allocated_count = 0;

	scene_obj_t::scene_obj_t (scene_controller_base & scene, const std::string & type_name, bool movable, bool rotatable, bool scalable) :
		m_scene (scene) {
		m_type_name = type_name;

		m_id = 0;

		m_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		m_translation = { 0.0f, 0.0f, 0.0f };
		m_scale = { 1.0f, 1.0f, 1.0f };
		m_euler_angles = { 0.0f, 0.0f, 0.0f };

		m_movable = movable;
		m_rotatable = rotatable;
		m_scalable = scalable;

		m_selected = false;
		m_disposed = false;
		m_mouse_lock = false;
		m_is_deletable = true;

		s_allocated_count++;
	}

	scene_obj_t::~scene_obj_t () { 
		s_allocated_count--;
	}

	void scene_obj_t::t_set_mouse_lock (bool lock) {
		m_mouse_lock = lock;
	}

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

	uint64_t scene_obj_t::get_id () const {
		return m_id;
	}

	void scene_obj_t::m_set_id (uint64_t id) {
		m_id = id;
	}

	void scene_obj_t::t_notify (signal_event_t sig) {
		m_notify (sig);
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

	bool scene_obj_t::is_deletabe () const {
		return m_is_deletable;
	}

	scene_controller_base & scene_obj_t::get_scene () const {
		return m_scene;
	}

	void scene_obj_t::dispose () {
		m_disposed = true;
	}

	void scene_obj_t::set_translation (const glm::vec3 & translation) {
		if (!m_movable) {
			return;
		}

		auto old_translation = m_translation;
		m_translation = translation;
		
		if (old_translation != translation) {
			m_notify (signal_event_t::moved);
		}
	}

	void scene_obj_t::set_euler_angles (const glm::vec3 & euler_angles) {
		if (!m_rotatable) {
			return;
		}

		glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

		rotation = rotation * glm::angleAxis (euler_angles[2], glm::vec3 { 0.0f, 0.0f, 1.0f });
		rotation = rotation * glm::angleAxis (euler_angles[1], glm::vec3 { 0.0f, 1.0f, 0.0f });
		rotation = rotation * glm::angleAxis (euler_angles[0], glm::vec3 { 1.0f, 0.0f, 0.0f });

		auto old_rotation = m_rotation;

		m_euler_angles = euler_angles;
		m_rotation = rotation;

		if (m_rotation != old_rotation) {
			m_notify (signal_event_t::rotated);
		}
	}

	void scene_obj_t::set_scale (const glm::vec3 & scale) {
		if (!m_scalable) {
			return;
		}

		auto old_scale = m_scale;
		m_scale = scale;
		
		if (old_scale != scale) {
			m_notify (signal_event_t::scaled);
		}
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

	void scene_obj_t::set_deletable (bool deletable) {
		m_is_deletable = deletable;
	}

	glm::mat4x4 scene_obj_t::compose_matrix (const glm::vec3 & translation, const glm::quat & quaternion, const glm::vec3 & scale) const {
		glm::mat4x4 world (1.0f);

		if (m_movable) {
			//world = glm::translate (world, translation);
			world = world * make_translation (translation);
		}

		if (m_rotatable) {
			world = world * glm::toMat4 (m_rotation);
		}

		if (m_scalable) {
			//world = glm::scale (world, scale);
			world = world * make_scale (scale);
		}

		return world;
	}

	glm::mat4x4 scene_obj_t::get_matrix () const {
		return compose_matrix (m_translation, m_rotation, m_scale);
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
					set_euler_angles (m_euler_angles);
				}
			}
			
			if (m_scalable) {
				if (gui::vector_editor ("Scale", m_scale)) {
					m_notify (signal_event_t::scaled);
				}
			}
		}
	}

	void scene_obj_t::alt_select () {
		t_on_alt_select ();
	}

	bool scene_obj_t::hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const {
		return false;
	}

	bool scene_obj_t::box_test (const box_test_data_t & data) const {
		return false;
	}

	const glm::quat & scene_obj_t::get_rotation () const {
		return m_rotation;
	}

	bool scene_obj_t::is_mouse_lock () const {
		return m_mouse_lock;
	}

	void scene_obj_t::set_rotation (const glm::quat & rotation) {
		if (!m_rotatable) {
			return;
		}

		auto old_rotation = m_rotation;

		m_rotation = rotation;
		m_euler_angles = glm::eulerAngles (m_rotation);

		if (old_rotation != rotation) {
			m_notify (signal_event_t::rotated);
		}
	}

	const object_serializer_base & scene_obj_t::get_serializer () const {
		return generic_object_serializer<scene_obj_t>::get_instance ();
	}

	glm::vec3 scene_obj_t::get_transform_origin () const {
		return m_translation;
	}

	hit_test_data_t::hit_test_data_t (const mini::camera & cam, const glm::vec2 & mouse_screen, 
		const glm::vec2 & screen_res, const glm::vec3 & mouse_ray) :
		camera (cam), 
		mouse_screen (mouse_screen),
		screen_res (screen_res),
		mouse_ray (mouse_ray) {

		valid = true;
	}

	box_test_data_t::box_test_data_t (const mini::camera & cam, const glm::vec2 & top_left_screen, 
		const glm::vec2 & bottom_right_screen, const glm::vec2 & screen_res) :
		camera (cam),
		top_left_screen (top_left_screen),
		bottom_right_screen (bottom_right_screen),
		screen_res (screen_res) {

		valid = true;
	}
}