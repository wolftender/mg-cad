#include <fstream>
#include <iostream>
#include <sstream>

#include "gui.hpp"
#include "app.hpp"

namespace mini {
	application::object_wrapper_t::object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name) : object (o), name (name), selected (false) {
		tmp_name = name;
		destroy = false;
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

		glm::vec3 plane_normal = glm::normalize (plane_center - cam_pos);
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

		if (action == GLFW_RELEASE && !ImGui::GetIO ().WantCaptureKeyboard) {
			switch (key) {
				case KEY_TRANSLATE:
					m_selected_tool = std::make_shared<translation_tool> (*this);
					break;

				case KEY_ROTATE:
					m_selected_tool = std::make_shared<rotation_tool> (*this);
					break;
					
				case GLFW_KEY_ESCAPE:
					m_reset_selection ();
					break;

				case GLFW_KEY_DELETE:
					m_destroy_object ();
					break;

				default:
					break;
			}
		}

		app_window::t_on_key_event (key, scancode, action, mods);
	}

	void application::t_on_scroll (double offset_x, double offset_y) {
		if (m_viewport_focus) {
			m_distance = m_distance - (offset_y / 2.0f);
		}
	}

	void application::t_on_resize (int width, int height) {
		if (width != 0 && height != 0) {
			video_mode_t new_vm = m_context.get_video_mode ();

			new_vm.set_viewport_width (width);
			new_vm.set_viewport_width (height);

			m_context.set_video_mode (new_vm);
		}

		app_window::t_on_resize (width, height);
	}

	void application::m_handle_mouse () {
		// mouse input etc
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
			m_selected_object->destroy = true;
		}
	}

	void application::m_show_object_creator (bool enable) {
		m_show_creator = enable;
	}

	application::application () : 
		app_window (1200, 800, "modelowanie geometryczne 1"), 
		m_context (video_mode_t (1200, 800)) {

		m_cam_pitch = 0.0f;
		m_cam_yaw = 0.0f;
		m_distance = 10.0f;
		m_time = 0.0f;
		m_grid_spacing = 1.0f;
		m_grid_enabled = true;
		m_viewport_focus = false;
		m_mouse_in_viewport = false;

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
		m_selected_group = std::make_shared<group_logic_object> ();

		// m_add_object ("torus", std::make_shared<torus_object> (m_mesh_shader, m_alt_mesh_shader, 1.0f, 3.0f));
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
			if ((*iter)->destroy) {
				if (m_selected_object && m_selected_object == *iter) {
					m_selected_object = m_selected_group->group_pop ();
				}

				iter = m_objects.erase (iter);
			}

			if (iter != m_objects.end ()) {
				++iter;
			}
		}

		// if no tool selected then handle mouse events
		// otherwise update tool and only update mouse if allowed
		if (!m_selected_tool || !m_selected_tool->on_update (delta_time)) {
			m_handle_mouse ();
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

		cam_rotation = glm::translate (cam_rotation, m_camera_target);
		cam_rotation = glm::rotate (cam_rotation, m_cam_yaw, { 0.0f, 1.0f, 0.0f });
		cam_rotation = glm::rotate (cam_rotation, m_cam_pitch, { 1.0f, 0.0f, 0.0f });
		
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
		
		if (m_selected_group) {
			m_context.draw (m_origin_object, make_translation (m_selected_group->get_origin ()));
		}

		m_context.display (false);

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

	void application::t_on_character (unsigned int code) {
		if (m_selected_tool) {
			if (m_selected_tool->on_character (code)) {
				return app_window::t_on_character (code);
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

		app_window::t_on_cursor_pos (posx, posy);
	}

	void application::t_on_mouse_button (int button, int action, int mods) {
		if (m_selected_tool) {
			if (m_selected_tool->on_mouse_button (button, action, mods)) {
				return app_window::t_on_mouse_button (button, action, mods);
			}
		} else if (action == GLFW_PRESS) {
			if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
				m_selected_tool = std::make_shared<camera_pan_tool> (*this);
			} else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
				m_snap_cursor_to_mouse ();
			}
		}

		app_window::t_on_mouse_button (button, action, mods);
	}

	void application::m_draw_main_menu () {
		if (ImGui::BeginMenuBar ()) {
			if (ImGui::BeginMenu ("File")) {
				ImGui::MenuItem ("Open");
				ImGui::EndMenu ();
			}
		}

		ImGui::EndMenuBar ();
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
			ImGuiWindowFlags_NoDocking);

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

		if (ImGui::CollapsingHeader ("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label ("Grid Enabled: ", 250.0f);
			ImGui::Checkbox ("##grid_enable", &m_grid_enabled);
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
		ImGui::Button ("Delete Object", ImVec2 (-1.0f, 24.0f));

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
		
		auto ptr = m_factory->configure ();

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

		int width = max.x - min.x;
		int height = max.y - min.y;

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
			m_context.set_video_mode (video_mode_t (width, height));

			m_last_vp_width = width;
			m_last_vp_height = height;
		}

		ImGui::ImageButton (reinterpret_cast<ImTextureID> (m_context.get_front_buffer ()), ImVec2 (width, height), ImVec2 (0, 0), ImVec2 (1, 1), 0);

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
		auto real_name = m_get_free_name (name);

		std::shared_ptr<object_wrapper_t> wrapper = std::shared_ptr<object_wrapper_t> (new object_wrapper_t (object, real_name));
		m_objects.push_back (wrapper);

		if (select) {
			m_reset_selection ();
			m_select_object (wrapper);
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
}