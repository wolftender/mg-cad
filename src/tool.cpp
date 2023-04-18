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

	glm::vec3 tool_base::calculate_mouse_dir () const {
		return get_app ().get_mouse_direction ();
	}

	glm::vec3 tool_base::calculate_mouse_dir (int offset_x, int offset_y) const {
		return get_app ().get_mouse_direction (offset_x, offset_y);
	}

	/**********************
	 *  TRANSLATION TOOL  *
	 **********************/
	translation_tool::translation_tool (application & app) : tool_base (app, "translate") {
		auto selection = get_app ().get_group_selection ();

		if (selection) {
			m_original_transform = selection->get_translation ();
			m_selection = selection;

			glm::vec2 screen_pos = get_app ().world_to_screen (m_original_transform);
			screen_pos = glm::clamp (screen_pos, { -1.0f, -1.0f }, { 1.0f, 1.0f });

			glm::vec2 pixel_pos = get_app ().screen_to_pixels (screen_pos);
			offset_t mouse_offset = get_app ().get_viewport_mouse_offset ();

			m_offset_x = static_cast<int> (pixel_pos.x) - mouse_offset.x;
			m_offset_y = static_cast<int> (pixel_pos.y) - mouse_offset.y;
		} else {
			t_dispose ();
			m_original_transform = glm::vec3 (0.0f);
			m_offset_x = m_offset_y = 0;
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

		if (m_axis_lock == axis_t::none) {
			const auto & camera = get_app ().get_context ().get_camera ();

			glm::vec3 plane_normal = normalize (camera.get_position () - camera.get_target ());
			glm::vec3 direction = calculate_mouse_dir (m_offset_x, m_offset_y);

			float nt = glm::dot ((m_original_transform - camera.get_position ()), plane_normal);
			float dt = glm::dot (direction, plane_normal);

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
			default: break;
		}

		m_selection->set_translation (t);

		return true;
	}


	/**********************
	 *   ROTATION TOOL    *
	 **********************/
	rotation_tool::rotation_tool (application & app) : tool_base (app, "rotate") {
		auto selection = get_app ().get_group_selection ();

		if (selection) {
			m_original_rotation = selection->get_rotation ();
			m_selection = selection;
		} else {
			m_original_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
			t_dispose ();
		}

		m_axis_lock = axis_t::none;
		m_apply = false;
	}

	rotation_tool::~rotation_tool () {
		if (m_selection && !m_apply) {
			m_selection->set_rotation (m_original_rotation);
		}
	}

	bool rotation_tool::on_key_event (int key, int scancode, int action, int mods) {
		if (!m_selection) {
			return false;
		}

		if (action == GLFW_RELEASE) {
			switch (key) {
			case KEY_AXIS_X:
				m_selection->set_rotation (m_original_rotation);
				m_axis_lock = axis_t::x;
				break;

			case KEY_AXIS_Y:
				m_selection->set_rotation (m_original_rotation);
				m_axis_lock = axis_t::y;
				break;

			case KEY_AXIS_Z:
				m_selection->set_rotation (m_original_rotation);
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
		constexpr float pi2 = 2.0f * glm::pi<float> ();

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

		glm::quat delta_rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
		constexpr float ang_speed = (1.0f / 360.0f) * glm::pi<float> ();

		switch (m_axis_lock) {
			case axis_t::x: 
				delta_rotation = delta_rotation * glm::angleAxis (ang_speed * dx, glm::vec3 { 1.0f, 0.0f, 0.0f }); 
				break;

			case axis_t::y:
				delta_rotation = delta_rotation * glm::angleAxis (ang_speed * dx, glm::vec3{ 0.0f, 1.0f, 0.0f });
				break;

			case axis_t::z: 
				delta_rotation = delta_rotation * glm::angleAxis (ang_speed * dx, glm::vec3{ 0.0f, 0.0f, 1.0f });
				break;

			default:
				const auto & camera = get_app ().get_camera ();
				glm::vec3 axis = glm::normalize (camera.get_target () - camera.get_position ());

				delta_rotation = delta_rotation * glm::angleAxis (ang_speed * dx, axis);
				break;
		}

		m_selection->set_rotation (glm::normalize (delta_rotation) * m_selection->get_rotation ());
		return true;
	}


	/**********************
	 *  CAMERA PAN TOOL   *
	 **********************/
	camera_pan_tool::camera_pan_tool (application & app) : tool_base (app, "camera pan") {
		m_original_target = app.get_cam_target ();
		const auto & camera = get_app ().get_context ().get_camera ();

		glm::vec3 plane_normal = glm::normalize (camera.get_position () - m_original_target);
		glm::vec3 direction = calculate_mouse_dir ();

		float nt = glm::dot ((m_original_target - camera.get_position ()), plane_normal);
		float dt = glm::dot (direction, plane_normal);

		m_original_hit = (nt / dt) * direction + camera.get_position ();
		m_original_normal = plane_normal;
	}

	camera_pan_tool::~camera_pan_tool () { }

	bool camera_pan_tool::on_mouse_button (int button, int action, int mods) {
		if (action == GLFW_RELEASE) {
			t_dispose ();
		}

		return true;
	}

	bool camera_pan_tool::on_update (float delta_time) {
		const auto & camera = get_app ().get_context ().get_camera ();

		glm::vec3 plane_normal = m_original_normal;
		glm::vec3 direction = calculate_mouse_dir ();

		float nt = glm::dot ((m_original_target - camera.get_position ()), plane_normal);
		float dt = glm::dot (direction, plane_normal);

		glm::vec3 new_pos = (nt / dt) * direction + camera.get_position ();
		glm::vec3 diff = new_pos - m_original_hit;

		if (glm::length (diff) < 0.01f) {
			return true;
		}

		glm::vec3 d_target = glm::normalize (diff);

		auto target = get_app ().get_cam_target ();
		get_app ().set_cam_target (target + 0.1f * d_target);

		return true;
	}

	/**********************
	 *     SCALE TOOL     *
	 **********************/
	scale_tool::scale_tool (application & app) : tool_base (app, "scale tool") {
		auto selection = get_app ().get_group_selection ();

		if (selection) {
			m_original_transform = selection->get_scale ();
			m_selection = selection;

			glm::vec2 screen_pos = get_app ().world_to_screen (m_selection->get_translation ());
			screen_pos = glm::clamp (screen_pos, { -1.0f, -1.0f }, { 1.0f, 1.0f });

			glm::vec2 pixel_pos = get_app ().screen_to_pixels (screen_pos);
			offset_t mouse_offset = get_app ().get_viewport_mouse_offset ();

			m_start_x = static_cast<float> (mouse_offset.x);
			m_start_y = static_cast<float> (mouse_offset.y);

			m_object_x = pixel_pos.x;
			m_object_y = pixel_pos.y;
		} else {
			m_object_x = m_object_y = m_start_x = m_start_y = 0.0f;
			m_original_transform = { 0.0f, 0.0f, 0.0f };

			t_dispose ();
		}

		m_axis_lock = axis_t::none;
		m_apply = false;
	}

	scale_tool::~scale_tool () {
		if (m_selection && !m_apply) {
			m_selection->set_scale (m_original_transform);
		}
	}

	bool scale_tool::on_key_event (int key, int scancode, int action, int mods) {
		if (!m_selection) {
			return false;
		}

		if (action == GLFW_RELEASE) {
			switch (key) {
			case KEY_AXIS_X:
				m_selection->set_scale (m_original_transform);
				m_axis_lock = axis_t::x;
				break;

			case KEY_AXIS_Y:
				m_selection->set_scale (m_original_transform);
				m_axis_lock = axis_t::y;
				break;

			case KEY_AXIS_Z:
				m_selection->set_scale (m_original_transform);
				m_axis_lock = axis_t::z;
				break;

			case KEY_CANCEL:
				t_dispose ();
			}
		}

		return true;
	}

	bool scale_tool::on_mouse_button (int button, int action, int mods) {
		if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
			if (m_selection) {
				m_apply = true;
				t_dispose ();
			}
		}

		return true;
	}

	bool scale_tool::on_update (float delta_time) {
		if (!m_selection) {
			t_dispose ();
			return false;
		}

		offset_t mouse_offset = get_app ().get_viewport_mouse_offset ();

		float mouse_x = static_cast<float> (mouse_offset.x);
		float mouse_y = static_cast<float> (mouse_offset.y);

		float dx = mouse_x - m_object_x;
		float dy = mouse_y - m_object_y;

		float dsx = m_object_x - m_start_x;
		float dsy = m_object_y - m_start_y;

		float d = glm::sqrt (dx * dx + dy * dy);
		float ds = glm::sqrt (dsx * dsx + dsy * dsy);

		float scale_factor = d / ds;
		if (scale_factor < 0.1f) {
			scale_factor = 0.1f;
		}

		if (scale_factor > 1.0f) {
			scale_factor = scale_factor * scale_factor;
		} else if (scale_factor < 1.0f) {
			scale_factor = glm::sqrt (scale_factor);
		}

		glm::vec3 scale = {
			m_original_transform.x,
			m_original_transform.y,
			m_original_transform.z
		};

		scale_factor = scale_factor - 1.0f;

		switch (m_axis_lock) {
			case axis_t::none:
				scale.x = scale.x + scale_factor;
				scale.y = scale.y + scale_factor;
				scale.z = scale.z + scale_factor;
				break;

			case axis_t::x:
				scale.x = scale.x + scale_factor;
				break;
			
			case axis_t::y:
				scale.y = scale.y + scale_factor;
				break;

			case axis_t::z:
				scale.z = scale.z + scale_factor;
				break;
		}

		m_selection->set_scale (scale);
		return true;
	}
}