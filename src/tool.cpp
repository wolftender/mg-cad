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

	float_vector_t tool_base::calculate_mouse_dir () const {
		const auto & camera = get_app ().get_context ().get_camera ();
		const auto & mouse_offset = get_app ().get_viewport_mouse_offset ();

		const float mouse_x = static_cast<float> (mouse_offset.x);
		const float mouse_y = static_cast<float> (mouse_offset.y);
		const float vp_width = static_cast<float> (get_app ().get_viewport_width ());
		const float vp_height = static_cast<float> (get_app ().get_viewport_height ());

		const float screen_x = (2.0f * (mouse_x / vp_width)) - 1.0f;
		const float screen_y = (2.0f * (mouse_y / vp_height)) - 1.0f;

		float_vector_t screen = { screen_x, screen_y, 1.0f, 1.0f };
		float_matrix_t view_proj_inv = camera.get_view_inverse () * camera.get_projection_inverse ();
		float_vector_t world = view_proj_inv * screen;

		world[3] = 1.0f / world[3];
		world[0] = world[0] * world[3];
		world[1] = world[1] * world[3];
		world[2] = world[2] * world[3];

		return normalize (world - camera.get_position ());
	}

	/**********************
	 *  TRANSLATION TOOL  *
	 **********************/
	translation_tool::translation_tool (application & app) : tool_base (app, "translate") {
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

		return true;
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

		// get camera
		if (m_axis_lock == axis_t::none) {
			const auto & camera = get_app ().get_context ().get_camera ();

			float_vector_t plane_normal = normalize (camera.get_position () - m_original_transform);
			float_vector_t direction = calculate_mouse_dir ();

			float nt = float_vector_t::dot ((m_original_transform - camera.get_position ()), plane_normal);
			float dt = float_vector_t::dot (direction, plane_normal);

			m_selection->set_translation ((nt / dt) * direction + camera.get_position ());
			return true;
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


	/**********************
	 *   ROTATION TOOL    *
	 **********************/
	rotation_tool::rotation_tool (application & app) : tool_base (app, "rotate") {
		auto selection = get_app ().get_selection ();

		if (selection) {
			m_original_rotation = selection->get_euler_angles ();
			m_selection = selection;
		} else {
			t_dispose ();
		}

		m_axis_lock = axis_t::none;
		m_apply = false;
	}

	rotation_tool::~rotation_tool () {
		if (m_selection && !m_apply) {
			m_selection->set_euler_angles (m_original_rotation);
		}
	}

	bool rotation_tool::on_key_event (int key, int scancode, int action, int mods) {
		if (!m_selection) {
			return false;
		}

		if (action == GLFW_RELEASE) {
			switch (key) {
			case KEY_AXIS_X:
				m_selection->set_euler_angles (m_original_rotation);
				m_axis_lock = axis_t::x;
				break;

			case KEY_AXIS_Y:
				m_selection->set_euler_angles (m_original_rotation);
				m_axis_lock = axis_t::y;
				break;

			case KEY_AXIS_Z:
				m_selection->set_euler_angles (m_original_rotation);
				m_axis_lock = axis_t::z;
				break;

			case KEY_CANCEL:
				t_dispose ();
			}
		}

		return true;
	}

	bool rotation_tool::on_mouse_button (int button, int action, int mods) {
		if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
			if (m_selection) {
				m_apply = true;
				t_dispose ();
			}
		}

		return true;
	}

	inline void loop_angle (float & angle) {
		const float pi2 = 2.0f * pi_f;
		if (angle > pi2) {
			angle = angle - pi2;
		} else if (angle < -pi2) {
			angle = angle + pi2;
		}
	}

	bool rotation_tool::on_update (float delta_time) {
		if (!m_selection) {
			t_dispose ();
			return false;
		}

		const offset_t & last_pos = get_app ().get_last_mouse_offset ();
		const offset_t & curr_pos = get_app ().get_mouse_offset ();

		float dx = static_cast<float>(curr_pos.x) - static_cast<float>(last_pos.x);
		float dy = static_cast<float>(curr_pos.y) - static_cast<float>(last_pos.y);

		auto t = m_selection->get_euler_angles ();
		const float ang_speed = (1 / 360.0f) * pi_f;

		switch (m_axis_lock) {
			case axis_t::x: t[0] += ang_speed * dx; break;
			case axis_t::y: t[1] += ang_speed * dx; break;
			case axis_t::z: t[2] += ang_speed * dx; break;
			default:
				t[0] += ang_speed * dx;
				t[1] -= ang_speed * dy;
				break;
		}

		loop_angle (t[0]);
		loop_angle (t[1]);
		loop_angle (t[2]);

		m_selection->set_euler_angles (t);

		return true;
	}
}