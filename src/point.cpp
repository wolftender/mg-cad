#include "point.hpp"

namespace mini {
	const glm::vec4 & point_object::get_color () const {
		return m_color;
	}

	const glm::vec4 & point_object::get_select_color () const {
		return m_selected_color;
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

		m_billboard.set_size ({ 16.0f, 16.0f });
	}

	void point_object::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		glDisable (GL_DEPTH_TEST);
		m_billboard.render (context, world_matrix);
		glEnable (GL_DEPTH_TEST);
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

	void point_object::t_on_selection (bool selected) {
		if (selected) {
			m_billboard.set_color_tint (m_selected_color);
		} else {
			m_billboard.set_color_tint (m_color);
		}
	}
}