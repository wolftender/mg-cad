#include "app.hpp"

// this implementation is split to a different file from app.cpp to reduce amount of code in one file
namespace mini {
	application::group_logic_object::group_logic_object (scene_controller_base & scene) : 
		scene_obj_t (scene, "group") {
		m_origin = glm::vec3 (0.0f);
	}

	void application::group_logic_object::group_add (std::shared_ptr<object_wrapper_t> object) {
		// first, before anything is done, apply all changes
		m_apply_all ();

		// now reset group transforms, since group has changed theres no reason to keep them
		m_reset_group_transforms ();

		m_group.push_back (grouped_object_wrapper (object));

		// now the tricky part, we need to recalculate new origin and all origin space transforms
		m_calc_origin ();
	}

	void application::group_logic_object::group_remove (std::shared_ptr<object_wrapper_t> object) {
		m_apply_all ();
		bool erased = false;

		for (auto iter = m_group.begin (); iter != m_group.end (); ++iter) {
			if ((*iter).ptr == object) {
				(*iter).ptr->selected = false;
				iter = m_group.erase (iter);

				erased = true;
			}

			if (iter == m_group.end ()) {
				break;
			}
		}

		if (erased) {
			m_reset_group_transforms ();
			m_calc_origin ();
		}
	}

	void application::group_logic_object::group_clear () {
		m_apply_all ();

		for (auto & el : m_group) {
			el.ptr->selected = false;
		}

		m_group.clear ();

		m_reset_group_transforms ();
		m_calc_origin ();
	}

	void application::group_logic_object::group_destroy_all () {
		for (auto & object : m_group) {
			object.ptr->destroy = true;
		}
	}

	scene_controller_base::selected_object_iter_ptr application::group_logic_object::get_iterator () {
		return std::unique_ptr<object_collection> (new object_collection (m_group));
	}

	uint32_t application::group_logic_object::group_size () const {
		return static_cast<uint32_t> (m_group.size ());
	}

	glm::vec3 application::group_logic_object::get_origin () const {
		return glm::vec3 (make_translation (m_origin) * get_matrix () * glm::vec4 (0.0f, 0.0f, 0.0f, 1.0f));
	}

	std::shared_ptr<application::object_wrapper_t> application::group_logic_object::group_pop () {
		if (m_group.size () > 0) {
			auto & back = m_group.back ();
			return back.ptr;
		}

		return nullptr;
	}

	void application::group_logic_object::update () {
		if (m_group.size () == 0) {
			return;
		}

		for (auto iter = m_group.begin (); iter != m_group.end (); ++iter) {
			if (!(*iter).ptr->selected || (*iter).ptr->destroy) {
				iter = m_group.erase (iter);
			}

			if (iter == m_group.end ()) {
				break;
			}
		}

		m_apply_all ();
	}

	// does nothing
	void application::group_logic_object::render (app_context & context, const glm::mat4x4 & world_matrix) const {}

	void application::group_logic_object::configure () {
		return scene_obj_t::configure ();
	}

	void application::group_logic_object::m_reset_group_transforms () {
		set_translation (glm::vec3 (0.0f));
		set_euler_angles (glm::vec3 (0.0f));
		set_scale (glm::vec3 (1.0f));
	}

	void application::group_logic_object::m_apply_all () {
		if (m_group.size () <= 1) {
			return;
		}

		glm::mat4x4 group_transform = get_matrix ();

		for (auto & element : m_group) {
			m_apply_group_transform (element, group_transform);
		}
	}

	void application::group_logic_object::m_calc_origin () {
		if (m_group.size () == 0) {
			m_origin = glm::vec3 (0.0f);
			return;
		}

		// this function does all the necessary stuff when origin has changed
		glm::vec3 origin = { 0.0f, 0.0f, 0.0f };
		int weight = 0;

		for (const auto & el : m_group) {
			const auto & translation = el.ptr->object->get_translation ();
			origin.x += translation.x;
			origin.y += translation.y;
			origin.z += translation.z;
			weight++;
		}

		origin.x = origin.x / static_cast<float> (weight);
		origin.y = origin.y / static_cast<float> (weight);
		origin.z = origin.z / static_cast<float> (weight);
		m_origin = origin;

		for (auto & el : m_group) {
			// to get the origin space transform, we just need to move the world space transform by -origin
			el.os_transform = el.ptr->object->get_matrix ();
			el.os_transform = make_translation (-m_origin) * el.os_transform;
		}
	}

	void application::group_logic_object::m_apply_group_transform (grouped_object_wrapper & item, const glm::mat4x4 & group_transform) {
		// this function applies all the transforms using the stored origin space transform of each object
		// basically it works like this:

		// this is the matrix that converts from origin space to world space
		glm::mat4x4 origin_transform = make_translation (m_origin) * group_transform;

		// convert into world space transforms
		glm::mat4x4 world_matrix = origin_transform * item.os_transform;
		glm::vec3 w_scale, w_translate, w_euler_angles, w_skew;
		glm::vec4 w_projection;
		glm::quat w_orientation;

		glm::decompose (world_matrix, w_scale, w_orientation, w_translate, w_skew, w_projection);
		w_euler_angles = glm::eulerAngles (w_orientation); // todo: migrate the whole program to quaternions because euler angles are dumb and doodoo

		// apply all world space transforms
		item.ptr->object->set_translation (w_translate);
		item.ptr->object->set_euler_angles (w_euler_angles);
		item.ptr->object->set_scale (w_scale);
	}

	application::group_logic_object::grouped_object_wrapper::grouped_object_wrapper (std::shared_ptr<object_wrapper_t> _ptr) {
		ptr = _ptr;
		os_transform = glm::mat4x4 (1.0f);
	}

	// collection
	application::group_logic_object::object_collection::object_collection (std::list<grouped_object_wrapper> & list) : 
		m_list (list),
		m_iter (list.begin ()) {
	}

	bool application::group_logic_object::object_collection::next () {
		if (m_iter != m_list.end ()) {
			m_iter++;
		}

		return (m_iter != m_list.end ());
	}

	bool application::group_logic_object::object_collection::has () {
		return (m_iter != m_list.end ());
	}

	std::shared_ptr<scene_obj_t> application::group_logic_object::object_collection::get_object () {
		if (m_iter != m_list.end ()) {
			return m_iter->ptr->object;
		}

		return nullptr;
	}
}