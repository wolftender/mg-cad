#include <fstream>
#include <iostream>
#include <sstream>

#include "gui.hpp"
#include "app.hpp"

// objects
#include "cube.hpp"
#include "torus.hpp"

namespace mini {
	application::object_wrapper_t::object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name) : object (o), name (name), selected (false) {}

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

	int application::get_viewport_width () const {
		return m_last_vp_width;
	}

	int application::get_viewport_height () const {
		return m_last_vp_height;
	}

	offset_t application::get_viewport_mouse_offset () const {
		return m_vp_mouse_offset;
	}

	app_context & application::get_context () {
		return m_context;
	}

	std::shared_ptr<scene_obj_t> application::get_selection () {
		return m_selected_object;
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

		if (action == GLFW_RELEASE) {
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

				default:
					break;
			}
		}
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

			m_cam_yaw = m_cam_yaw + f_d_yaw;
			m_cam_pitch = m_cam_pitch - f_d_pitch;
		}
	}

	std::string application::m_read_file_content (const std::string & path) const {
		std::ifstream stream (path);

		if (stream) {
			std::stringstream ss;
			ss << stream.rdbuf ();

			return ss.str ();
		}

		return std::string ();
	}

	std::shared_ptr<shader_t> application::m_load_shader (const std::string & vs_file, const std::string & ps_file) const {
		const std::string vs_source = m_read_file_content (vs_file);
		const std::string ps_source = m_read_file_content (ps_file);

		return std::make_shared<shader_t> (vs_source, ps_source);
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

		m_last_vp_height = m_last_vp_width = 0;

		m_basic_shader = m_load_shader ("shaders/vs_basic.glsl", "shaders/fs_basic.glsl");
		m_mesh_shader = m_load_shader ("shaders/vs_meshgrid.glsl", "shaders/fs_meshgrid.glsl");
		m_alt_mesh_shader = m_load_shader ("shaders/vs_meshgrid.glsl", "shaders/fs_meshgrid_s.glsl");
		m_grid_xz_shader = m_load_shader ("shaders/vs_grid.glsl", "shaders/fs_grid_xz.glsl");
		m_grid_xy_shader = m_load_shader ("shaders/vs_grid.glsl", "shaders/fs_grid_xy.glsl");

		m_basic_shader->compile ();
		m_mesh_shader->compile ();
		m_alt_mesh_shader->compile ();
		m_grid_xz_shader->compile ();
		m_grid_xy_shader->compile ();

		// initialize the test cube
		m_grid_xz = std::make_shared<grid_object> (m_grid_xz_shader);
		m_grid_xy = std::make_shared<grid_object> (m_grid_xy_shader);

		// start with a cube
		m_add_object ("torus", std::make_shared<torus_object> (m_mesh_shader, m_alt_mesh_shader, 1.0f, 3.0f));
	}

	void application::t_integrate (float delta_time) {
		// if current tool is disposable then simply remove it
		if (m_selected_tool) {
			if (m_selected_tool->is_disposable ()) {
				m_selected_tool = nullptr;
			}
		}

		m_time = m_time + delta_time;

		// if no tool selected then handle mouse events
		// otherwise update tool and only update mouse if allowed
		if (!m_selected_tool || !m_selected_tool->on_update (delta_time)) {
			m_handle_mouse ();
		}

		// clamp pitch to avoid camera "going over" the center
		// the matrices will degenerate then
		gui::clamp (m_cam_pitch, -1.56f, 1.56f);

		constexpr float pi2 = pi_f * 2.0f;
		if (m_cam_yaw > pi2) {
			m_cam_yaw = m_cam_yaw - pi2;
		} else if (m_cam_yaw < -pi2) {
			m_cam_yaw = m_cam_yaw + pi2;
		}

		// clamp distance just in case
		gui::clamp (m_distance, 1.0f, 20.0f);

		gui::clamp (m_grid_spacing, 0.05f, 10.0f);

		// setup camera for the scene
		float_vector_t cam_pos = { 0.0f, 0.0f, -m_distance };
		float_matrix_t cam_rotation = make_rotation_y (m_cam_yaw) * make_rotation_x (m_cam_pitch);

		cam_pos = cam_rotation * cam_pos;
		m_context.get_camera ().set_position (cam_pos);

		app_window::t_integrate (delta_time);
	}

	void application::t_render () {
		for (const auto & object : m_objects) {
			object.object->set_selected (object.selected);
			m_context.draw (object.object, object.object->get_matrix ());
		}
		
		if (m_grid_enabled) {
			m_grid_xz->set_spacing (m_grid_spacing);
			m_grid_xy->set_spacing (m_grid_spacing);
			
			/*if (abs (m_cam_pitch) < 0.001f) {
				m_context.draw (m_grid_xy, make_rotation_x (pi_f / 2.0f));
			}*/

			m_context.draw (m_grid_xz, make_identity ());
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
			ImGui::DockBuilderDockWindow ("Scene Options", dock_id_left_bottom);

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
			ImGui::NewLine ();
		}

		if (ImGui::CollapsingHeader ("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
			gui::prefix_label ("Grid Enabled: ", 250.0f);
			ImGui::Checkbox ("##grid_enable", &m_grid_enabled);
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
		ImGui::SetNextItemWidth (-1);

		if (ImGui::BeginListBox ("##objectlist")) {
			for (auto & object : m_objects) {
				std::string full_name;
				bool was_selected = object.selected;

				if (object.selected) {
					full_name = "*" + object.name + " : (" + object.object->get_type_name () + ")";
				} else {
					full_name = object.name + " : (" + object.object->get_type_name () + ")";
				}

				if (ImGui::Selectable (full_name.c_str (), &object.selected)) {
					for (auto & other : m_objects) {
						if (other.object != object.object) {
							other.selected = false;
						}
					}

					m_selected_object = object.object;
				}

				if (was_selected && !object.selected) {
					m_selected_object = nullptr;
				}
			}

			ImGui::EndListBox ();
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
			m_selected_object->configure ();
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

	void application::m_add_object (const std::string & name, std::shared_ptr<scene_obj_t> object) {
		std::string real_name = name;
		int i = 0;
		bool name_free = true;

		do {
			if (!name_free) {
				real_name = name + "_" + std::to_string (i);
				i++;
				name_free = true;
			}

			for (const auto & object : m_objects) {
				if (object.name == real_name) {
					name_free = false;
				}
			}
		} while (!name_free);

		m_objects.push_back ({object, real_name});
	}

	void application::m_reset_selection () {
		m_selected_tool = nullptr;
		m_selected_object = nullptr;

		for (auto & object : m_objects) {
			object.selected = false;
		}
	}
}