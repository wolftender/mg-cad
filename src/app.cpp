#include <fstream>
#include <iostream>
#include <sstream>

#include <nfd.h>

#include "gui.hpp"
#include "app.hpp"
#include "point.hpp"
#include "serializer.hpp"
#include "gapfilling.hpp"

namespace mini {
	constexpr const std::string_view app_title = "modelowanie geometryczne 1";

	application::object_wrapper_t::object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name) : object (o), name (name), selected (false) {
		tmp_name = name;
		destroy = false;

		object->set_name (name);
	}

	float application::get_cam_yaw () const {
		return m_cam_yaw;
	}

	float application::get_cam_pitch () const {
		return m_cam_pitch;
	}

	float application::get_cam_distance () const {
		return m_distance;
	}

	float application::get_time () const {
		return m_time;
	}

	bool application::is_viewport_focused () const {
		return m_viewport_focus;
	}

	bool application::is_mouse_in_viewport () const {
		return m_mouse_in_viewport;
	}

	const glm::vec3 & application::get_cursor_pos () const {
		return m_cursor_position;
	}

	glm::vec2 application::get_cursor_screen_pos () const {
		const auto & camera = m_context.get_camera ();

		glm::vec4 cursor_pos = { m_cursor_position, 1.0f };
		glm::vec4 screen_pos = camera.get_projection_matrix () * camera.get_view_matrix () * cursor_pos;

		screen_pos /= screen_pos.w;

		return static_cast<glm::vec2> (screen_pos);
	}

	hit_test_data_t application::get_hit_test_data () const {
		auto & camera = m_context.get_camera ();

		glm::vec2 mouse_screen = glm::vec2 (
			static_cast<float> (m_vp_mouse_offset.x),
			static_cast<float> (m_vp_mouse_offset.y)
		);

		glm::vec2 screen_res = glm::vec2 (
			static_cast<float> (m_last_vp_width),
			static_cast<float> (m_last_vp_height)
		);

		hit_test_data_t hit_data (
			camera,
			mouse_screen,
			screen_res,
			get_mouse_direction ()
		);
		
		hit_data.valid = is_mouse_in_viewport ();
		return hit_data;
	}

	glm::vec3 application::get_mouse_direction () const {
		return get_mouse_direction (0, 0);
	}

	glm::vec3 application::get_mouse_direction (int offset_x, int offset_y) const {
		const auto & mouse_offset = get_viewport_mouse_offset ();

		const float mouse_x = static_cast<float> (mouse_offset.x + offset_x);
		const float mouse_y = static_cast<float> (mouse_offset.y + offset_y);

		const auto screen_pos = pixels_to_screen ({ mouse_x, mouse_y });

		return get_screen_direction (screen_pos.x, screen_pos.y);
	}

	glm::vec3 application::get_screen_direction (float screen_x, float screen_y) const {
		const auto & camera = m_context.get_camera ();
		
		glm::vec4 screen = { screen_x, screen_y, 1.0f, 1.0f };
		glm::mat4x4 view_proj_inv = camera.get_view_inverse () * camera.get_projection_inverse ();
		glm::vec4 world = view_proj_inv * screen;

		world[3] = 1.0f / world[3];
		world[0] = world[0] * world[3];
		world[1] = world[1] * world[3];
		world[2] = world[2] * world[3];
		world[3] = 1.0f;

		glm::vec3 world_pos = world;
		return glm::normalize (world_pos - camera.get_position ());
	}

	glm::vec2 application::pixels_to_screen (const glm::vec2 & pos) const {
		const auto & camera = m_context.get_camera ();
		const auto & mouse_offset = get_viewport_mouse_offset ();

		const float vp_width = static_cast<float> (get_viewport_width ());
		const float vp_height = static_cast<float> (get_viewport_height ());

		const float screen_x = (2.0f * (pos.x / vp_width)) - 1.0f;
		const float screen_y = (2.0f * (pos.y / vp_height)) - 1.0f;

		return { screen_x, screen_y };
	}

	glm::vec2 application::screen_to_pixels (const glm::vec2 & pos) const {
		const auto & camera = m_context.get_camera ();
		const auto & mouse_offset = get_viewport_mouse_offset ();

		const float vp_width = static_cast<float> (get_viewport_width ());
		const float vp_height = static_cast<float> (get_viewport_height ());

		const float pixel_x = ((pos.x + 1.0f) / 2.0f) * vp_width;
		const float pixel_y = ((pos.y + 1.0f) / 2.0f) * vp_height;

		return { pixel_x, pixel_y };
	}

	glm::vec2 application::world_to_screen (const glm::vec3 & world_pos) const {
		const auto & camera = m_context.get_camera ();

		glm::vec4 world_pos_affine = { world_pos, 1.0f };
		glm::vec4 screen_pos = camera.get_projection_matrix () * camera.get_view_matrix () * world_pos_affine;

		screen_pos /= screen_pos.w;

		return glm::vec2 (screen_pos.x, screen_pos.y);
	}

	scene_controller_base::selected_object_iter_ptr application::get_selected_objects () {
		if (!m_selected_group) {
			throw std::runtime_error ("selection group was not initialized");
		}

		return m_selected_group->get_iterator ();
	}

	void application::select_by_id (uint64_t id) {
		auto iter = m_id_cache.find (id);

		if (iter != m_id_cache.end ()) {
			auto object_ptr = iter->second.lock ();
			if (object_ptr) {
				m_group_select_add (object_ptr);
			}
		}
	}

	void application::clear_selection () {
		m_reset_selection ();
	}

	void application::set_cursor_pos (const glm::vec3 & position) {
		m_cursor_position = position;
	}

	void application::set_cursor_screen_pos (const glm::vec2 & screen_pos) {
		const auto & camera = m_context.get_camera ();
		const auto & cam_pos = camera.get_position ();
		
		glm::vec2 cur_screen_pos = get_cursor_screen_pos ();
		glm::vec3 plane_center = get_cursor_pos ();

		glm::vec2 new_pos = glm::clamp (screen_pos, {-1.0f, -1.0f}, {1.0f, 1.0f});

		if (cur_screen_pos.x > 1.0f || cur_screen_pos.y > 1.0f || cur_screen_pos.x < -1.0f || cur_screen_pos.y < -1.0f) {
			plane_center = m_camera_target;
		}

		glm::vec3 plane_normal = glm::normalize (get_cam_target () - cam_pos);
		glm::vec3 direction = get_screen_direction (new_pos.x, new_pos.y);

		float nt = glm::dot ((plane_center - camera.get_position ()), plane_normal);
		float dt = glm::dot (direction, plane_normal);

		set_cursor_pos ((nt / dt) * direction + camera.get_position ());
	}

	int application::get_viewport_width () const {
		return m_last_vp_width;
	}

	int application::get_viewport_height () const {
		return m_last_vp_height;
	}

	offset_t application::get_viewport_mouse_offset () const {
		return m_vp_mouse_offset;
	}

	const glm::vec3 & application::get_cam_target () const {
		return m_camera_target;
	}

	void application::set_cam_target (const glm::vec3 & target) {
		m_camera_target = target;
	}

	const camera & application::get_camera () const {
		return m_context.get_camera ();
	}

	const video_mode_t & application::get_video_mode () const {
		return m_context.get_video_mode ();
	}

	bool application::get_show_points () const {
		return m_points_enabled;
	}

	app_context & application::get_context () {
		return m_context;
	}

	std::shared_ptr<scene_obj_t> application::get_selection () {
		if (m_selected_object) {
			return m_selected_object->object;
		}

		return nullptr;
	}

	std::shared_ptr<scene_obj_t> application::get_group_selection () {
		if (m_selected_group->group_size () >= 2) {
			return m_selected_group;
		}

		return get_selection ();
	}

	void application::set_cam_yaw (float yaw) {
		m_cam_yaw = yaw;
	}

	void application::set_cam_pitch (float pitch) {
		m_cam_pitch = pitch;
	}

	void application::set_cam_distance (float distance) {
		m_distance = distance;
	}

	void application::t_on_key_event (int key, int scancode, int action, int mods) {
		if (m_selected_tool) {
			if (m_selected_tool->on_key_event (key, scancode, action, mods)) {
				return;
			}
		}

		if (m_selected_object && m_selected_group->group_size () == 1) {
			if (m_selected_object->object->on_key_event (key, scancode, action, mods)) {
				return;
			}
		}

		if (action == GLFW_RELEASE && !ImGui::GetIO ().WantCaptureKeyboard) {
			switch (key) {
				case KEY_TRANSLATE:
					m_selected_tool = std::make_shared<translation_tool> (*this, axis_t::none, false);
					break;

				case KEY_ROTATE:
					m_selected_tool = std::make_shared<rotation_tool> (*this, axis_t::none);
					break;

				case KEY_SCALE:
					m_selected_tool = std::make_shared<scale_tool> (*this, axis_t::none);
					break;

				case KEY_MERGE:
					m_merge_selection ();
					break;

				case KEY_FILLIN:
					m_fillin_selection ();
					break;
					
				case GLFW_KEY_ESCAPE:
					m_reset_selection ();
					break;

				case GLFW_KEY_DELETE:
					m_destroy_object ();
					break;

				case KEY_SELECT_ALL:
					m_select_all ();
					break;

				default:
					break;
			}
		}

		app_window::t_on_key_event (key, scancode, action, mods);
	}

	void application::t_on_scroll (double offset_x, double offset_y) {
		if (m_selected_tool) {
			if (m_selected_tool->on_scroll (offset_x, offset_y)) {
				return;
			}
		}

		if (m_selected_object && m_selected_group->group_size () == 1) {
			if (m_selected_object->object->on_scroll (offset_x, offset_y)) {
				return;
			}
		}

		if (m_viewport_focus && m_mouse_in_viewport) {
			m_distance = m_distance - (static_cast<float> (offset_y) / 2.0f);
		}
	}

	void application::t_on_character (unsigned int code) {
		if (m_selected_tool) {
			if (m_selected_tool->on_character (code)) {
				return app_window::t_on_character (code);
			}
		}

		if (m_selected_object && m_selected_group->group_size () == 1) {
			if (m_selected_object->object->on_character (code)) {
				return;
			}
		}

		app_window::t_on_character (code);
	}

	void application::t_on_cursor_pos (double posx, double posy) {
		if (m_selected_tool) {
			if (m_selected_tool->on_cursor_pos (posx, posy)) {
				return app_window::t_on_cursor_pos (posx, posy);
			}
		}

		if (m_selected_object && m_selected_group->group_size () == 1) {
			if (m_selected_object->object->on_cursor_pos (posx, posy)) {
				return;
			}
		}

		app_window::t_on_cursor_pos (posx, posy);
	}

	void application::t_on_mouse_button (int button, int action, int mods) {
		if (m_selected_tool) {
			if (m_selected_tool->on_mouse_button (button, action, mods)) {
				return app_window::t_on_mouse_button (button, action, mods);
			}
		}

		if (m_selected_object && m_selected_group->group_size () == 1) {
			if (m_selected_object->object->on_mouse_button (button, action, mods)) {
				return app_window::t_on_mouse_button (button, action, mods);
			}
		}
		
		if (action == GLFW_PRESS) {
			if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
				m_selected_tool = std::make_shared<camera_pan_tool> (*this);
			} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
				m_snap_cursor_to_mouse ();
			} else if (button == GLFW_MOUSE_BUTTON_LEFT) {
				bool is_alt_down = is_key_down (GLFW_KEY_LEFT_ALT) || is_key_down (GLFW_KEY_RIGHT_ALT);

				if (!is_alt_down) {
					if (!m_handle_gizmo_action ()) {
						m_handle_mouse_select ();
					}
				} else {
					m_begin_box_select ();
				}
			}
		}

		app_window::t_on_mouse_button (button, action, mods);
	}

	void application::t_on_resize (int width, int height) {
		if (width != 0 && height != 0) {
			video_mode_t new_vm = m_context.get_video_mode ();

			new_vm.set_viewport_width (width);
			new_vm.set_viewport_width (height);

			m_context.set_video_mode (new_vm);
			m_anaglyph.set_video_mode (new_vm);
		}

		app_window::t_on_resize (width, height);
	}

	bool application::m_handle_gizmo_action () {
		auto group_size = m_selected_group->group_size ();
		if (group_size == 0) {
			return false;
		}

		hit_test_data_t data = get_hit_test_data ();
		
		if (!data.valid) {
			return false;
		}

		gizmo::gizmo_action_t action = gizmo::gizmo_action_t::none;

		if (group_size == 1) {
			action = m_gizmo->get_action (data, m_selected_object->object->get_translation ());
		} else {
			action = m_gizmo->get_action (data, m_selected_group->get_origin ());
		}

		switch (action) {
			case gizmo::gizmo_action_t::translate_x:
				m_selected_tool = std::make_shared<translation_tool> (*this, axis_t::x, true);
				return true;

			case gizmo::gizmo_action_t::translate_y:
				m_selected_tool = std::make_shared<translation_tool> (*this, axis_t::y, true);
				return true;
			
			case gizmo::gizmo_action_t::translate_z:
				m_selected_tool = std::make_shared<translation_tool> (*this, axis_t::z, true);
				return true;

			default: break;
		} 

		return false;
	}

	void application::m_handle_mouse_select () {
		// todo: probably rework this code
		auto hit_data = get_hit_test_data ();

		if (!hit_data.valid) {
			return;
		}

		// pass all of this to the hit detect functions
		std::shared_ptr<object_wrapper_t> selection = nullptr;
		glm::vec3 pos;
		float best_dist = 1000000.0f, dist;

		for (auto & object : m_objects) {
			if (object->object->hit_test (hit_data, pos)) {
				// hit was detected, compare hit vector with "best" hit vector
				// i.e. select the object closer to the camera

				dist = glm::distance (get_camera ().get_position (), pos);
				if (dist < best_dist) {
					selection = object;
					best_dist = dist;
				}
			}
		}

		if (selection) {
			m_mark_object (selection);
		}
	}

	void application::m_handle_mouse () {
		// mouse input etc
		if (m_box_select) {
			bool is_alt_down = is_key_down (GLFW_KEY_LEFT_ALT) || is_key_down (GLFW_KEY_RIGHT_ALT);

			if (is_left_click () && m_viewport_focus && is_alt_down) {
				const offset_t & curr_pos = get_viewport_mouse_offset ();

				float xs = m_bs_start.x;
				float xc = curr_pos.x;

				float ys = m_bs_start.y;
				float yc = curr_pos.y;

				float x1 = glm::min (xs, xc);
				float x2 = glm::max (xs, xc);

				float y1 = glm::min (ys, yc);
				float y2 = glm::max (ys, yc);

				m_bs_top_left = { x1, y1 };
				m_bs_bottom_right = { x2, y2 };

				float sx = x2 - x1;
				float sy = y2 - y1;

				m_bs_sprite->set_size ({sx, sy});
			} else {
				m_end_box_select ();
			}

			return;
		}

		if (is_left_click () && m_viewport_focus) {
			const offset_t & last_pos = get_last_mouse_offset ();
			const offset_t & curr_pos = get_mouse_offset ();

			int d_yaw = curr_pos.x - last_pos.x;
			int d_pitch = curr_pos.y - last_pos.y;

			float f_d_yaw = static_cast<float> (d_yaw);
			float f_d_pitch = static_cast<float> (d_pitch);

			f_d_yaw = f_d_yaw * 15.0f / static_cast<float> (get_width ());
			f_d_pitch = f_d_pitch * 15.0f / static_cast<float> (get_height ());

			m_cam_yaw = m_cam_yaw - f_d_yaw;
			m_cam_pitch = m_cam_pitch + f_d_pitch;
		}
	}

	void application::m_snap_cursor_to_mouse () {
		if (!m_mouse_in_viewport) {
			return;
		}

		const auto & mouse_offset = get_viewport_mouse_offset ();

		const float mouse_x = static_cast<float> (mouse_offset.x);
		const float mouse_y = static_cast<float> (mouse_offset.y);

		const auto screen_pos = pixels_to_screen ({ mouse_x, mouse_y });

		set_cursor_screen_pos (screen_pos);
	}

	void application::m_destroy_object () {
		if (m_selected_object) {
			if (m_selected_group->group_size () > 1) {
				m_selected_group->group_destroy_all ();
				m_reset_selection ();
			} else {
				m_selected_object->destroy = true;
			}
		}
	}

	void application::m_show_object_creator (bool enable) {
		m_show_creator = enable;
	}

	application::application () : 
		app_window (1200, 800, std::string (app_title)),
		m_context (video_mode_t (1200, 800)),
		m_anaglyph (m_context.get_video_mode ()) {

		// render hooks
		m_context.set_post_render (std::bind (&application::m_post_render, this, std::placeholders::_1));

		m_cam_pitch = 0.0f;
		m_cam_yaw = 0.0f;
		m_distance = 10.0f;
		m_time = 0.0f;
		m_grid_spacing = 1.0f;
		m_grid_enabled = true;
		m_viewport_focus = false;
		m_mouse_in_viewport = false;
		m_points_enabled = true;

		m_last_vp_height = m_last_vp_width = 0;
		m_test_texture = texture_t::load_from_file ("assets/test.png");

		m_store = std::make_shared<resource_store> ();
		m_factory = std::make_shared<object_factory> (m_store);

		// initialize gizmos
		m_cursor_object = std::make_shared<billboard_object> (m_store->get_billboard_s_shader (), m_store->get_cursor_texture ());
		m_origin_object = std::make_shared<billboard_object> (m_store->get_billboard_s_shader (), m_store->get_cursor_texture ());
		m_cursor_object->set_size ({ 35.0f, 35.0f });
		m_origin_object->set_size ({45.0f, 45.0f});

		m_grid_xz = std::make_shared<grid_object> (m_store->get_grid_xz_shader ());
		m_grid_xy = std::make_shared<grid_object> (m_store->get_grid_xy_shader ());

		// selection
		m_selected_object = nullptr;
		m_selected_group = std::make_shared<group_logic_object> (*this);

		// default is not saved
		m_is_saved = false;

		// box select data
		m_box_select = false;
		m_bs_bottom_right = { 0.0f, 0.0f };
		m_bs_top_left = { 0.0f, 0.0f };
		m_bs_start = { 0.0f, 0.0f };

		m_bs_sprite = std::make_shared<sprite> (m_store->get_box_select_shader (), nullptr);

		// gizmo
		m_gizmo = std::make_shared<gizmo> (m_store->get_gizmo_shader (), m_store->get_line_shader ());
	}

	void application::t_integrate (float delta_time) {
		// if current tool is disposable then simply remove it
		if (m_selected_tool) {
			if (m_selected_tool->is_disposable ()) {
				m_selected_tool = nullptr;
			}
		}

		m_time = m_time + delta_time;

		// update the current group
		m_selected_group->update ();

		// delete objects
		for (auto iter = m_objects.begin (); iter != m_objects.end (); ) {
			if ((*iter)->object->is_disposed ()) {
				(*iter)->destroy = true;
			} else if (!(*iter)->object->is_deletabe ()) {
				(*iter)->destroy = false;
			}

			if ((*iter)->destroy) {
				if (m_selected_object && m_selected_object == *iter) {
					m_selected_object = m_selected_group->group_pop ();
				}

				for (auto & listener : m_objects) {
					listener->object->notify_object_deleted ((*iter)->object);
				}

				// unparent
				auto old_id = t_unparent_object (*(*iter)->object);
				m_id_cache.erase (old_id);

				iter = m_objects.erase (iter);
			}

			if (iter != m_objects.end ()) {
				++iter;
			}
		}

		for (auto & obj : m_objects) {
			obj->object->integrate (delta_time);
		}

		// if no tool selected then handle mouse events
		// otherwise update tool and only update mouse if allowed
		if (!m_selected_tool || !m_selected_tool->on_update (delta_time)) {
			if (!m_selected_object || m_selected_group->group_size () != 1 || !m_selected_object->object->is_mouse_lock ()) {
				m_handle_mouse ();
			}
		}

		// clamp pitch to avoid camera "going over" the center
		// the matrices will degenerate then
		gui::clamp (m_cam_pitch, -1.56f, 1.56f);

		constexpr float pi2 = glm::pi<float> () * 2.0f;

		if (m_cam_yaw > pi2) {
			m_cam_yaw = m_cam_yaw - pi2;
		} else if (m_cam_yaw < -pi2) {
			m_cam_yaw = m_cam_yaw + pi2;
		}

		// clamp distance just in case
		gui::clamp (m_distance, 1.0f, 20.0f);

		gui::clamp (m_grid_spacing, 0.05f, 10.0f);

		// setup camera for the scene
		glm::vec4 cam_pos = { 0.0f, 0.0f, -m_distance, 1.0f };
		glm::mat4x4 cam_rotation (1.0f);

		//cam_rotation = glm::translate (cam_rotation, m_camera_target);
		//cam_rotation = glm::rotate (cam_rotation, m_cam_yaw, { 0.0f, 1.0f, 0.0f });
		//cam_rotation = glm::rotate (cam_rotation, m_cam_pitch, { 1.0f, 0.0f, 0.0f });
		cam_rotation = cam_rotation * make_translation (m_camera_target);
		cam_rotation = cam_rotation * make_rotation_y (m_cam_yaw);
		cam_rotation = cam_rotation * make_rotation_x (m_cam_pitch);
		
		cam_pos = cam_rotation * cam_pos;

		m_context.get_camera ().set_position (cam_pos);
		m_context.get_camera ().set_target (m_camera_target);

		app_window::t_integrate (delta_time);
	}

	void application::t_render () {
		for (const auto & object : m_objects) {
			object->object->set_selected (object->selected);
			m_context.draw (object->object, object->object->get_matrix ());
		}
		
		if (m_grid_enabled) {
			m_grid_xz->set_spacing (m_grid_spacing);
			m_grid_xy->set_spacing (m_grid_spacing);
			
			/*if (abs (m_cam_pitch) < 0.001f) {
				m_context.draw (m_grid_xy, make_rotation_x (pi_f / 2.0f));
			}*/

			m_context.draw (m_grid_xz, glm::mat4x4 (1.0f));
		}

		
		m_context.draw (m_cursor_object, make_translation (m_cursor_position));

		if (m_box_select) {
			float bs_width = m_bs_sprite->get_size ().x;
			float bs_height = m_bs_sprite->get_size ().y;

			glm::mat4x4 bs_world (1.0f);
			bs_world = glm::translate (bs_world, { 0.5f * bs_width + m_bs_top_left.x, 0.5f * bs_height + m_bs_top_left.y, 0 });

			m_context.draw (m_bs_sprite, bs_world);
		}

		if (m_anaglyph.is_enabled ()) {
			m_anaglyph.render (m_context);
		} else {
			m_context.display (false, true);
		}

		// rendering above is done to a buffer
		// now we can render ui to the window

		m_draw_main_window ();
		m_draw_viewport ();
		m_draw_view_options ();
		m_draw_scene_options ();

		if (m_selected_object != nullptr) {
			m_draw_object_options ();
		}

		if (m_selected_group->group_size () > 1) {
			m_draw_group_options ();
		}

		if (m_show_creator) {
			m_draw_object_creator ();
		}
	}

	void application::m_post_render (app_context & context) {
		glClear (GL_DEPTH_BUFFER_BIT);
		glEnable (GL_DEPTH_TEST);

		if (m_selected_group && m_selected_group->group_size () >= 1) {
			if (m_selected_group->group_size () == 1) {
				m_gizmo->render (context, make_translation (m_selected_object->object->get_translation ()));
			} else {
				m_gizmo->render (context, make_translation (m_selected_group->get_origin ()));
			}
		}
	}

	void application::m_draw_main_menu () {
		ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding, ImVec2 (10.0f, 10.0f));

		if (ImGui::BeginMenuBar ()) {
			if (ImGui::BeginMenu ("File")) {
				if (ImGui::MenuItem ("New", "Ctrl + N", nullptr, true)) {
					m_new_project ();
				}

				ImGui::Separator ();

				if (ImGui::MenuItem ("Open", "Ctrl + O", nullptr, true)) {
					m_load_scene ();
				}

				ImGui::Separator ();

				if (ImGui::MenuItem ("Save As...", "Ctrl + Shift + S", nullptr, true)) {
					m_save_scene_as ();
				}

				if (ImGui::MenuItem ("Save", "Ctrl + S", nullptr, m_is_saved)) {
					m_save_scene ();
				}

				ImGui::EndMenu ();
			}

			if (ImGui::BeginMenu ("Edit")) {
				if (ImGui::MenuItem ("Clear Selection", "Esc", nullptr, true)) {
					m_reset_selection ();
				}

				bool selected_objects = (m_selected_group->group_size () > 0);

				if (ImGui::MenuItem ("Delete Objects", "Delete", nullptr, selected_objects)) {
					m_destroy_object ();
				}

				if (ImGui::MenuItem ("Merge Points", "M", nullptr, selected_objects)) {
					m_merge_selection ();
				}

				if (ImGui::MenuItem ("Fill-in Surface", "G", nullptr, selected_objects)) {
					m_fillin_selection ();
				}

				ImGui::Separator ();

				if (ImGui::MenuItem ("Translate", "T", nullptr, selected_objects)) {
					m_selected_tool = std::make_shared<translation_tool> (*this, axis_t::none, false);
				}

				if (ImGui::MenuItem ("Rotate", "R", nullptr, selected_objects)) {
					m_selected_tool = std::make_shared<rotation_tool> (*this, axis_t::none);
				}

				if (ImGui::MenuItem ("Scale", "S", nullptr, selected_objects)) {
					m_selected_tool = std::make_shared<scale_tool> (*this, axis_t::none);
				}

				ImGui::EndMenu ();
			}
		}

		ImGui::EndMenuBar ();
		ImGui::PopStyleVar (1);
	}

	void application::m_draw_main_window () {
		static bool first_time = true;

		ImGuiViewport * viewport = ImGui::GetMainViewport ();
		float vp_width = static_cast<float> (get_width ());
		float vp_height = static_cast<float> (get_height ());

		ImGui::SetNextWindowViewport (viewport->ID);
		ImGui::PushStyleVar (ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar (ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar (ImGuiStyleVar_WindowPadding, ImVec2 (0.0f, 0.0f));

		ImGui::Begin ("Dockspace", nullptr, 
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_MenuBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoResize);

		ImGui::SetWindowPos (ImVec2 (0, 0));
		ImGui::SetWindowSize (ImVec2 (vp_width, vp_height));

		ImGuiID dockspace_id = ImGui::GetID ("g_dockspace");
		ImGui::DockSpace (dockspace_id, ImVec2 (0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

		auto min = ImGui::GetWindowContentRegionMin ();
		auto max = ImGui::GetWindowContentRegionMax ();

		if (first_time) {
			first_time = false;

			ImGui::DockBuilderRemoveNode (dockspace_id);
			ImGui::DockBuilderAddNode (dockspace_id, ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize (dockspace_id, ImVec2 (vp_width, vp_height));

			auto dock_id_left = ImGui::DockBuilderSplitNode (dockspace_id, ImGuiDir_Left, 0.25f, nullptr, &dockspace_id);
			auto dock_id_left_bottom = ImGui::DockBuilderSplitNode (dock_id_left, ImGuiDir_Down, 0.45f, nullptr, &dock_id_left);
			
			ImGui::DockBuilderDockWindow ("Viewport", dockspace_id);
			ImGui::DockBuilderDockWindow ("View Options", dock_id_left);
			ImGui::DockBuilderDockWindow ("Object Options", dock_id_left);
			ImGui::DockBuilderDockWindow ("Group Options", dock_id_left);
			ImGui::DockBuilderDockWindow ("Scene Options", dock_id_left_bottom);
			ImGui::DockBuilderDockWindow ("Object Creator", dock_id_left_bottom);

			ImGui::DockBuilderFinish (dockspace_id);
		}

		ImGui::PopStyleVar (3);
		m_draw_main_menu ();

		ImGui::End ();
	}

	void application::m_draw_view_options () {
		ImGui::PushStyleVar (ImGuiStyleVar_WindowMinSize, ImVec2 (270, 450));
		ImGui::Begin ("View Options", NULL);
		ImGui::SetWindowPos (ImVec2 (30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize (ImVec2 (270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::CollapsingHeader ("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label ("Cam. Pitch: ", 250.0f);
			ImGui::InputFloat ("##pitch", &m_cam_pitch);

			gui::prefix_label ("Cam. Yaw: ", 250.0f);
			ImGui::InputFloat ("##yaw", &m_cam_yaw);

			gui::prefix_label ("Cam. Dist: ", 250.0f);
			ImGui::InputFloat ("##distance", &m_distance);

			gui::vector_editor ("Cam. Target", m_camera_target);
			ImGui::NewLine ();
		}

		if (ImGui::CollapsingHeader ("Display", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label ("Grid Enabled: ", 250.0f);
			ImGui::Checkbox ("##grid_enable", &m_grid_enabled);

			gui::prefix_label ("Points Enabled: ", 250.0f);
			ImGui::Checkbox ("##points_enable", &m_points_enabled);
			ImGui::NewLine ();
		}

		if (ImGui::CollapsingHeader ("Cursor", ImGuiTreeNodeFlags_DefaultOpen)) {
			glm::vec2 cursor_screen_pos = get_cursor_screen_pos ();

			gui::vector_editor ("World Pos. :", m_cursor_position);

			if (gui::vector_editor ("Screen Pos. :", cursor_screen_pos)) {
				set_cursor_screen_pos (cursor_screen_pos);
			}

			ImGui::NewLine ();

			if (ImGui::Button ("Center camera", ImVec2 (0.0f, 24.0f))) {
				set_cam_target (get_cursor_pos ());
			}

			ImGui::SameLine ();

			if (ImGui::Button ("Center cursor", ImVec2 (0.0f, 24.0f))) {
				set_cursor_pos (get_cam_target ());
			}

			ImGui::SameLine ();

			if (ImGui::Button ("Reset cursor", ImVec2 (-1.0f, 24.0f))) {
				set_cursor_pos ({ 0.0f, 0.0f, 0.0f });
			}
		}

		m_anaglyph.configure ();

		ImGui::End ();
		ImGui::PopStyleVar (1);
	}

	void application::m_draw_scene_options () {
		ImGui::PushStyleVar (ImGuiStyleVar_WindowMinSize, ImVec2 (270, 450));
		ImGui::Begin ("Scene Options", NULL);
		ImGui::PopStyleVar (1);
		ImGui::SetWindowPos (ImVec2 (30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize (ImVec2 (270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::BeginListBox ("##objectlist", ImVec2 (-1.0f, ImGui::GetWindowHeight () - 110.0f))) {
			for (auto & object : m_objects) {
				std::string full_name;
				bool selected = object->selected;

				if (selected) {
					full_name = "*" + object->name + " : (" + object->object->get_type_name () + ")";
				} else {
					full_name = object->name + " : (" + object->object->get_type_name () + ")";
				}

				if (ImGui::Selectable (full_name.c_str (), &selected)) {
					m_mark_object (object);
				}
			}

			ImGui::EndListBox ();
		}

		ImGui::NewLine ();

		if (ImGui::Button ("Add Object", ImVec2 (ImGui::GetWindowWidth () * 0.45f, 24.0f))) {
			m_show_object_creator (true);
		}

		ImGui::SameLine ();
		
		if (ImGui::Button ("Delete Object", ImVec2 (-1.0f, 24.0f))) {
			m_destroy_object ();
		}

		ImGui::End ();
	}

	void application::m_draw_object_options () {
		ImGui::PushStyleVar (ImGuiStyleVar_WindowMinSize, ImVec2 (270, 450));
		ImGui::Begin ("Object Options", NULL);
		ImGui::PopStyleVar (1);
		ImGui::SetWindowPos (ImVec2 (30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize (ImVec2 (270, 450), ImGuiCond_Once);

		if (m_selected_object) {
			if (ImGui::CollapsingHeader ("Object Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
				gui::prefix_label ("Name", 250.0f);
				ImGui::InputText ("##obj_name", &m_selected_object->tmp_name);

				if (ImGui::IsItemDeactivatedAfterEdit ()) {
					if (m_selected_object->tmp_name.length () > 0) {
						auto real_name = m_get_free_name (m_selected_object->tmp_name, m_selected_object->name);
						m_selected_object->name = real_name;
						m_selected_object->tmp_name = real_name;
						m_selected_object->object->set_name (real_name);
					} else {
						m_selected_object->tmp_name = m_selected_object->name;
					}
				}
				
				ImGui::NewLine ();
			}

			m_selected_object->object->configure ();
		}

		ImGui::End ();
	}

	void application::m_draw_group_options () {
		ImGui::Begin ("Group Options", NULL);
		m_selected_group->configure ();
		ImGui::End ();
	}

	void application::m_draw_object_creator () {
		ImGui::Begin ("Object Creator", NULL);
		
		auto ptr = m_factory->configure (*this);

		if (ptr) {
			// an object was created, so stop showing the ui
			m_show_object_creator (false);

			// insert the created object into the object tree
			// at the position of the cursor
			ptr->set_translation (get_cursor_pos ());
			m_add_object (ptr->get_type_name (), ptr, true);
		}

		ImGui::End ();
	}

	void application::m_draw_viewport () {
		ImGui::PushStyleVar (ImGuiStyleVar_WindowMinSize, ImVec2 (320, 240));
		ImGui::Begin ("Viewport", NULL);
		ImGui::SetWindowPos (ImVec2 (30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize (ImVec2 (640, 480), ImGuiCond_Once);

		if (ImGui::IsWindowFocused ()) {
			m_viewport_focus = true;
		} else {
			m_viewport_focus = false;
		}

		auto min = ImGui::GetWindowContentRegionMin ();
		auto max = ImGui::GetWindowContentRegionMax ();
		auto window_pos = ImGui::GetWindowPos ();

		int width = static_cast<int> (max.x - min.x);
		int height = static_cast<int> (max.y - min.y);

		const offset_t & mouse_offset = get_mouse_offset ();
		m_vp_mouse_offset.x = mouse_offset.x - static_cast<int> (min.x + window_pos.x);
		m_vp_mouse_offset.y = mouse_offset.y - static_cast<int> (min.y + window_pos.y);

		if (m_vp_mouse_offset.x < 0 || m_vp_mouse_offset.x > width || m_vp_mouse_offset.y < 0 || m_vp_mouse_offset.y > height) {
			m_mouse_in_viewport = false;
		} else {
			m_mouse_in_viewport = true;
		}

		// if mouse is out of viewport then pretend its at the border
		gui::clamp (m_vp_mouse_offset.x, 0, width);
		gui::clamp (m_vp_mouse_offset.y, 0, height);

		if ((width != m_last_vp_width || height != m_last_vp_height) && width > 8 && height > 8) {
			video_mode_t mode (width, height);

			m_context.set_video_mode (mode);
			m_anaglyph.set_video_mode (mode);

			m_last_vp_width = width;
			m_last_vp_height = height;
		}

		if (m_anaglyph.is_enabled ()) {
			ImGui::ImageButton (reinterpret_cast<ImTextureID> (m_anaglyph.get_buffer_handle ()), ImVec2 (width, height), ImVec2 (0, 0), ImVec2 (1, 1), 0);
		} else {
			ImGui::ImageButton (reinterpret_cast<ImTextureID> (m_context.get_front_buffer ()), ImVec2 (width, height), ImVec2 (0, 0), ImVec2 (1, 1), 0);
		}

		ImGui::End ();
		ImGui::PopStyleVar (1);
	}

	std::string application::m_get_free_name (const std::string & name, const std::string & self) const {
		std::string real_name = name;
		int i = 0;
		bool name_free = true;

		do {
			if (!name_free) {
				real_name = name + "_" + std::to_string (i);
				i++;
				name_free = true;
			}
			
			if (real_name == self) {
				return self;
			}

			for (const auto & object : m_objects) {
				if (object->name == real_name) {
					name_free = false;
				}
			}
		} while (!name_free);

		return real_name;
	}

	void application::m_add_object (const std::string & name, std::shared_ptr<scene_obj_t> object, bool select) {
		// dont allow duplicate objects
		if (object->get_id () != 0UL) {
			return;
		}

		auto real_name = m_get_free_name (name);

		std::shared_ptr<object_wrapper_t> wrapper = std::shared_ptr<object_wrapper_t> (new object_wrapper_t (object, real_name));
		m_objects.push_back (wrapper);

		auto id = t_parent_object (*object);
		m_id_cache.insert ({ id, wrapper });

		if (select) {
			m_reset_selection ();
			m_select_object (wrapper);
		}

		for (auto & o : m_objects) {
			o->object->notify_object_created (object);
		}
	}

	void application::m_merge_selection () {
		std::list<point_ptr> points;
		glm::vec3 sum = { 0.0f, 0.0f, 0.0f };

		for (auto iter = m_selected_group->get_iterator (); iter->has (); iter->next ()) {
			auto object = std::dynamic_pointer_cast<point_object> (iter->get_object ());

			if (object && object->is_mergeable ()) {
				points.push_back (object);
				sum += object->get_translation ();
			}
		}

		if (points.size () < 2) {
			return;
		}

		sum = sum / static_cast<float> (points.size ());
		auto center = std::make_shared<point_object> (
			*this,
			m_store->get_billboard_s_shader (),
			m_store->get_point_texture ()
		);

		center->set_translation (sum);
		m_add_object ("point", center, true);

		for (const auto & point : points) {
			point->merge (center);
		}
	}

	void application::m_fillin_selection () {
		gap_filling_controller algorithm (*this);
	}

	void application::m_begin_box_select () {
		m_box_select = true;
		m_bs_start = {
			static_cast<float> (m_vp_mouse_offset.x),
			static_cast<float> (m_vp_mouse_offset.y)
		};
	}

	void application::m_end_box_select () {
		m_box_select = false;

		glm::vec2 screen_res = glm::vec2 (
			static_cast<float> (m_last_vp_width),
			static_cast<float> (m_last_vp_height)
		);

		box_test_data_t box_test (get_camera (), m_bs_top_left, m_bs_bottom_right, screen_res);

		for (auto & object : m_objects) {
			if (object->object->box_test (box_test)) {
				m_group_select_add (object);
			}
		}
	}

	// this function should be the main entry point for the selection
	// say you were to make a new selection mechanism e.g. based on keyboard
	// and want it to be consistent with the others
	// then use this method
	void application::m_mark_object (std::shared_ptr<object_wrapper_t> object_wrapper) {
		bool is_control_down = is_key_down (GLFW_KEY_RIGHT_CONTROL) || is_key_down (GLFW_KEY_LEFT_CONTROL);

		if (is_control_down) {
			m_group_select_add (object_wrapper);
		} else {
			m_select_object (object_wrapper);
		}
	}

	void application::m_select_object (std::shared_ptr<object_wrapper_t> object) {
		if (object->selected) {
			if (m_selected_group->group_size () > 1) {
				// if object was selected as a part of a group then clear selection from all of the group
				// and select only the object
				m_reset_selection ();
				m_group_select_add (object);
			} else {
				// if object was selected by itself then just clear the selection
				m_reset_selection ();
			}
		} else {
			// if the object is not selected then just clear selection of the current object and select another one
			m_reset_selection ();
			m_group_select_add (object);
		}
	}

	void application::m_group_select_add (std::shared_ptr<object_wrapper_t> object) {
		auto prev_select = m_selected_object;

		if (object->selected) {
			if (m_selected_group->group_size () > 1) {
				if (object == m_selected_object) {
					m_selected_group->group_remove (object);
					m_selected_object = m_selected_group->group_pop ();
				} else {
					m_selected_group->group_remove (object);
				}
			} else {
				m_reset_selection ();
			}
		} else {
			m_selected_object = object;
			m_selected_object->selected = true;

			m_selected_group->group_add (object);
		}

		if (prev_select != m_selected_object && m_selected_object) {
			for (auto & o : m_objects) {
				o->object->notify_object_selected (m_selected_object->object);
			}
		}
	}

	void application::m_select_all () {
		m_reset_selection ();

		for (auto & object : m_objects) {
			m_group_select_add (object);
		}
	}

	void application::m_reset_selection () {
		m_selected_tool = nullptr;
		m_selected_object = nullptr;

		// clear selection group
		m_selected_group->group_clear ();

		for (auto & object : m_objects) {
			object->selected = false;
		}
	}

	bool application::m_serialize_scene (std::string & serialized) const {
		scene_serializer serializer;

		for (int i = 0; i < m_objects.size (); ++i) {
			serializer.add_object (m_objects[i]->object);
		}

		serialized = serializer.get_data ();
		return true;
	}

	bool application::m_deserialize_scene (const std::string & data) {
		return false;
	}

	void application::m_new_project () {
		m_reset_selection ();
		m_selected_tool = nullptr;
		
		m_objects.clear ();
		m_id_cache.clear ();
		m_project_path.clear ();

		set_cursor_pos ({ 0.0f, 0.0f, 0.0f });

		m_is_saved = false;
		m_cam_pitch = 0.0f;
		m_cam_yaw = 0.0f;
		m_distance = 10.0f;
		m_grid_spacing = 1.0f;
		m_grid_enabled = true;

		set_title (std::string (app_title));
	}

	void application::m_save_scene_as () {
		constexpr const nfdchar_t* filters = "json";
		nfdchar_t * out_path = nullptr;

		nfdresult_t result = NFD_SaveDialog (filters, nullptr, &out_path);

		if (result == NFD_OKAY) {
			std::string path = std::string (out_path, strlen(out_path));
			
			m_is_saved = true;
			m_project_path = path;

			set_title (std::string (app_title) + " - " + m_project_path);

			m_save_scene ();
			free (out_path);
		}
	}

	void application::m_save_scene () {
		if (m_is_saved) {
			std::string serialized;
			
			if (m_serialize_scene (serialized)) {
				std::ofstream fs (m_project_path);

				if (fs) {
					fs << serialized;
				} else {
					std::cerr << "failed to open file " << m_project_path << std::endl;
				}
			} else {
				std::cerr << "failed to serialize scene" << std::endl;
			}
		}
	}

	void application::m_load_scene () {
		constexpr const nfdchar_t * filters = "json";
		nfdchar_t * in_path = nullptr;

		nfdresult_t result = NFD_OpenDialog (filters, nullptr, &in_path);

		if (result == NFD_OKAY) {
			std::string path = std::string (in_path, strlen (in_path));
			std::ifstream stream (path);

			if (!stream) {
				std::cerr << "failed to open file " << path << std::endl;
				return;
			}

			std::stringstream ss;
			ss << stream.rdbuf ();
			stream.close ();

			scene_deserializer deserializer (*this, m_store);

			try {
				deserializer.load (ss.str ());
			} catch (const std::exception & error) {
				std::cerr << error.what () << std::endl;
				return;
			}

			// clear all objects
			m_new_project ();

			while (deserializer.has_next ()) {
				auto object = deserializer.get_next ();
				add_object (object->get_name (), object);
			}

			m_is_saved = true;
			m_project_path = path;
			set_title (std::string (app_title) + " - " + m_project_path);
		}
	}

	void application::add_object (const std::string & name, std::shared_ptr<scene_obj_t> object) {
		m_add_object (name, object, true);
	}

	std::shared_ptr<scene_obj_t> application::get_object (uint64_t id) {
		auto iter = m_id_cache.find (id);

		if (iter != m_id_cache.end ()) {
			auto object = iter->second.lock ();
			if (object) {
				return object->object;
			}
		}

		return nullptr;
	}
}