#include "point.hpp"

namespace mini {
	bool point_family_base::get_destroy_allowed () const {
		return m_destroy_allowed;
	}

	bool point_family_base::get_merge_allowed () const {
		return m_merge_allowed;
	}

	point_family_base::point_family_base (
		scene_controller_base & scene, 
		const std::string & type_name,
		bool destroy_allowed, 
		bool merge_allowed) :
		scene_obj_t (scene, type_name, false, false, false) {

		m_merge_allowed = merge_allowed;
		m_destroy_allowed = destroy_allowed;
	}

	void point_family_base::t_set_destroy_allowed (bool value) {
		m_destroy_allowed = value;
	}

	void point_family_base::t_set_merge_allowed (bool value) {
		m_merge_allowed = value;
	}

	void point_family_base::m_point_destroy (const point_ptr point) {
		t_on_point_destroy (point);
	}

	void point_family_base::m_point_merge (const point_ptr point, const point_ptr merge) {
		t_on_point_merge (point, merge);
	}

	/////////////////////////////////////////////////////////

	const glm::vec4 & point_object::get_color () const {
		return m_color;
	}

	const glm::vec4 & point_object::get_select_color () const {
		return m_selected_color;
	}

	bool point_object::is_mergeable () const {
		return m_mergeable;
	}

	void point_object::set_color (const glm::vec4 & color) {
		if (!is_selected ()) {
			m_billboard.set_color_tint (color);
		}

		m_color = color;
	}

	void point_object::set_select_color (const glm::vec4 & color) {
		if (is_selected ()) {
			m_billboard.set_color_tint (color);
		}

		m_selected_color = color;
	}

	point_object::point_object (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture) :
		scene_obj_t (scene, "point", true, false, false),
		m_billboard (shader, texture) { 

		m_color = s_color_default;
		m_selected_color = s_select_default;

		m_mergeable = true;
		m_billboard.set_size ({ 16.0f, 16.0f });
	}

	void point_object::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (get_scene ().get_show_points ()) {
			glDisable (GL_DEPTH_TEST);
			m_billboard.render (context, world_matrix);
			glEnable (GL_DEPTH_TEST);
		}
	}

	void point_object::configure () {
		return scene_obj_t::configure ();
	}

	bool point_object::hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const {
		// project the point onto the screen
		glm::vec4 affine_pos = glm::vec4 (get_translation (), 1.0f);
		glm::vec4 projected_pos = data.camera.get_projection_matrix () * data.camera.get_view_matrix () * affine_pos;

		projected_pos = projected_pos / projected_pos.w;

		glm::vec2 screen_pos = projected_pos;
		glm::vec2 pixel_pos = {
			((screen_pos.x + 1.0f) / 2.0f) * data.screen_res.x,
			((screen_pos.y + 1.0f) / 2.0f) * data.screen_res.y
		};

		if (glm::distance (pixel_pos, data.mouse_screen) < 25.0f) {
			hit_pos = get_translation ();
			return true;
		}

		return false;
	}

	bool point_object::box_test (const box_test_data_t & data) const {
		// project the point onto the screen
		glm::vec4 affine_pos = glm::vec4 (get_translation (), 1.0f);
		glm::vec4 projected_pos = data.camera.get_projection_matrix () * data.camera.get_view_matrix () * affine_pos;

		projected_pos = projected_pos / projected_pos.w;

		glm::vec2 screen_pos = projected_pos;
		glm::vec2 pixel_pos = {
			((screen_pos.x + 1.0f) / 2.0f) * data.screen_res.x,
			((screen_pos.y + 1.0f) / 2.0f) * data.screen_res.y
		};

		if (data.top_left_screen.x <= pixel_pos.x &&
			pixel_pos.x <= data.bottom_right_screen.x &&
			data.top_left_screen.y <= pixel_pos.y &&
			pixel_pos.y <= data.bottom_right_screen.y) {

			return true;
		}
		
		return false;
	}

	void point_object::add_parent (std::shared_ptr<point_family_base> family) {
		if (!family->get_destroy_allowed ()) {
			set_deletable (false);
		}

		if (!family->get_merge_allowed ()) {
			m_mergeable = false;
		}

		m_parents.push_back (family);
	}

	void point_object::clear_parent (const point_family_base & family) {
		for (auto iter = m_parents.begin (); iter != m_parents.end (); ) {
			auto parent = iter->lock ();
			if (!parent || parent->get_id () == family.get_id ()) {
				iter = m_parents.erase (iter);
				continue;
			}

			++iter;
		}

		m_check_deletable ();
	}

	void point_object::merge (point_ptr point) {
		auto self = std::dynamic_pointer_cast<point_object>(shared_from_this ());

		for (auto iter = m_parents.begin (); iter != m_parents.end (); ) {
			auto parent = iter->lock ();
			if (parent) {
				parent->m_point_merge (self, point);
			}

			iter = m_parents.erase (iter);
		}

		// no more parents, no need to keep it non-deletable
		set_deletable (true);
		m_mergeable = true;
	}

	void point_object::t_on_selection (bool selected) {
		if (selected) {
			m_billboard.set_color_tint (m_selected_color);
		} else {
			m_billboard.set_color_tint (m_color);
		}
	}

	void point_object::m_check_deletable () {
		bool deletable = true, mergeable = true;

		for (auto & parent_wptr : m_parents) {
			auto parent = parent_wptr.lock ();
			if (parent) {
				deletable = deletable || parent->get_destroy_allowed ();
				mergeable = mergeable || parent->get_merge_allowed ();
			}
		}

		set_deletable (deletable);
		m_mergeable = mergeable;
	}
}