#include <fstream>
#include <iostream>
#include <sstream>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include "app.hpp"

namespace mini {
	application::object_wrapper_t::object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name) : object (o), name (name), selected (false) {}

	void application::t_on_key_event (int key, int scancode, int action, int mods) {
		if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
			m_reset_selection ();
		}
	}

	void application::t_on_scroll (double offset_x, double offset_y) {
		if (m_viewport_focus) {
			m_distance = m_distance - (offset_y / 2.0f);
		}
	}

	void application::t_on_resize (int width, int height) {
		video_mode_t new_vm = m_context.get_video_mode ();

		new_vm.set_viewport_width (width);
		new_vm.set_viewport_width (height);

		m_context.set_video_mode (new_vm);

		app_window::t_on_resize (width, height);
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
		m_grid_shader = m_load_shader ("shaders/vs_grid.glsl", "shaders/fs_grid.glsl");

		m_basic_shader->compile ();
		m_grid_shader->compile ();

		// initialize the test cube
		m_grid = std::make_shared<grid_object> (m_grid_shader);

		// start with a cube
		m_add_object ("cube", std::make_shared<cube_object> (m_basic_shader));
		m_add_object ("cube", std::make_shared<cube_object> (m_basic_shader));
		m_add_object ("cube", std::make_shared<cube_object> (m_basic_shader));
		m_add_object ("cube", std::make_shared<cube_object> (m_basic_shader));
	}

	void application::t_integrate (float delta_time) {
		m_time = m_time + delta_time;

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

		// clamp pitch to avoid camera "going over" the center
		// the matrices will degenerate then
		if (m_cam_pitch > 1.56f) {
			m_cam_pitch = 1.56f;
		} else if (m_cam_pitch < -1.56f) {
			m_cam_pitch = -1.56f;
		}

		// clamp distance just in case
		if (m_distance < 1.0f) {
			m_distance = 1.0f;
		} else if (m_distance > 20.0f) {
			m_distance = 20.0f;
		}

		if (m_grid_spacing < 0.05f) {
			m_grid_spacing = 0.05f;
		} else if (m_grid_spacing > 10.0f) {
			m_grid_spacing = 10.0f;
		}

		// setup camera for the scene
		float_vector_t cam_pos = { 0.0f, 0.0f, -m_distance };
		float_matrix_t cam_rotation = make_rotation_y (m_cam_yaw) * make_rotation_x (m_cam_pitch);

		cam_pos = cam_rotation * cam_pos;
		m_context.get_camera ().set_position (cam_pos);

		app_window::t_integrate (delta_time);
	}

	void application::t_render () {
		for (const auto & object : m_objects) {
			m_context.draw (object.object, make_identity ());
		}
		
		if (m_grid_enabled) {
			m_grid->set_spacing (m_grid_spacing);
			m_context.draw (m_grid, make_identity ());
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

	void prefix_label (const std::string & label, float min_width = 0.0f) {
		float width = ImGui::CalcItemWidth ();

		if (width < min_width) {
			width = min_width;
		}

		float x = ImGui::GetCursorPosX ();
		ImGui::Text (label.c_str ());
		ImGui::SameLine ();
		ImGui::SetCursorPosX (x + width * 0.5f + ImGui::GetStyle ().ItemInnerSpacing.x);
		ImGui::SetNextItemWidth (-1);
	}

	void application::m_draw_view_options () {
		ImGui::PushStyleVar (ImGuiStyleVar_WindowMinSize, ImVec2 (270, 450));
		ImGui::Begin ("View Options", NULL);
		ImGui::SetWindowPos (ImVec2 (30, 30), ImGuiCond_Once);
		ImGui::SetWindowSize (ImVec2 (270, 450), ImGuiCond_Once);

		// render controls
		if (ImGui::CollapsingHeader ("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
			prefix_label ("Cam. Pitch: ", 250.0f);
			ImGui::InputFloat ("##pitch", &m_cam_pitch);

			prefix_label ("Cam. Yaw: ", 250.0f);
			ImGui::InputFloat ("##yaw", &m_cam_yaw);

			prefix_label ("Cam. Dist: ", 250.0f);
			ImGui::InputFloat ("##distance", &m_distance);
			ImGui::NewLine ();
		}

		if (ImGui::CollapsingHeader ("Grid", ImGuiTreeNodeFlags_DefaultOpen)) {
			prefix_label ("Grid Enabled: ", 250.0f);
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

		int width = max.x - min.x;
		int height = max.y - min.y;

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
		m_selected_object = nullptr;

		for (auto & object : m_objects) {
			object.selected = false;
		}
	}
}