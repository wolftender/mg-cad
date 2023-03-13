#include "tool.hpp"
#include "app.hpp"
#include "object.hpp"

namespace mini {
	void tool_base::t_dispose () {
		m_disposable = true;
	}

	const std::string & tool_base::get_name () const {
		return m_name;
	}

	application & tool_base::get_app () const {
		return m_app;
	}

	bool tool_base::is_disposable () const {
		return m_disposable;
	}

	tool_base::tool_base (application & app, const std::string & name) : m_app (app) {
		m_name = name;
		m_disposable = false;
	}

	tool_base::~tool_base () { }

	bool tool_base::on_key_event (int key, int scancode, int action, int mods) { 
		if (action == GLFW_RELEASE && key == KEY_CANCEL) {
			t_dispose ();
		}

		return false;
	}

	bool tool_base::on_character (unsigned int code) { return false; }
	bool tool_base::on_cursor_pos (double posx, double posy) { return false; }
	bool tool_base::on_mouse_button (int button, int action, int mods) { return false; }
	bool tool_base::on_scroll (double offset_x, double offset_y) { return false; }
	bool tool_base::on_update (float delta_time) { return false; }


	/**********************
	 *  TRANSLATION TOOL  *
	 **********************/
	translation_tool::translation_tool (application & app) : tool_base (app, "translation") {
		auto selection = get_app ().get_selection ();

		if (selection) {
			m_original_transform = selection->get_translation ();
			m_selection = selection;
		} else {
			t_dispose ();
		}

		m_axis_lock = axis_t::none;
		m_apply = false;
	}
		
	translation_tool::~translation_tool () { 
		if (m_selection && !m_apply) {
			m_selection->set_translation (m_original_transform);
		}
	}

	bool translation_tool::on_key_event (int key, int scancode, int action, int mods) {
		if (!m_selection) {
			return false;
		}
		
		if (action == GLFW_RELEASE) {
			switch (key) {
				case KEY_AXIS_X:
					m_selection->set_translation (m_original_transform);
					m_axis_lock = axis_t::x;
					break;

				case KEY_AXIS_Y:
					m_selection->set_translation (m_original_transform);
					m_axis_lock = axis_t::y;
					break;

				case KEY_AXIS_Z:
					m_selection->set_translation (m_original_transform);
					m_axis_lock = axis_t::z;
					break;

				case KEY_CANCEL:
					t_dispose ();
			}
		}

		return false;
	}

	bool translation_tool::on_mouse_button (int button, int action, int mods) {
		if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
			if (m_selection) {
				m_apply = true;
				t_dispose ();
			}
		}

		return true;
	}

	bool translation_tool::on_update (float delta_time) {
		if (!m_selection) {
			t_dispose ();
			return false;
		}

		const offset_t & last_pos = get_app ().get_last_mouse_offset ();
		const offset_t & curr_pos = get_app ().get_mouse_offset ();

		float dx = static_cast<float>(curr_pos.x) - static_cast<float>(last_pos.x);
		float dy = static_cast<float>(curr_pos.y) - static_cast<float>(last_pos.y);

		auto t = m_selection->get_translation ();

		switch (m_axis_lock) {
			case axis_t::x: t[0] += 0.1f * dx; break;
			case axis_t::y: t[1] += 0.1f * dy; break;
			case axis_t::z: t[2] += 0.1f * dx; break;
			default:
				t[0] += 0.1f * dx;
				t[1] -= 0.1f * dy;
				break;
		}

		m_selection->set_translation (t);

		return true;
	}
}