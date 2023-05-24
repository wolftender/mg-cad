#include "surfacetpl.hpp"
#include "gui.hpp"

namespace mini {
	int surface_template_base::get_patches_x () const {
		return m_patches_x;
	}

	int surface_template_base::get_pathces_y () const {
		return m_patches_y;
	}

	float surface_template_base::get_radius () const {
		return m_radius;
	}

	bool surface_template_base::is_rebuild_needed () const {
		return m_rebuild;
	}

	bool surface_template_base::is_added () const {
		return m_added;
	}

	surface_template_base::build_mode_t surface_template_base::get_build_mode () const {
		return m_build_mode;
	}

	void surface_template_base::set_patches_x (int patches_x) {
		m_patches_x = patches_x;
		m_rebuild = true;
	}

	void surface_template_base::set_patches_y (int patches_y) {
		m_patches_y = patches_y;
		m_rebuild = true;
	}

	void surface_template_base::set_radius (float radius) {
		m_radius = radius;
		m_rebuild = true;
	}

	void surface_template_base::set_rebuild_needed (bool rebuild) {
		m_rebuild = rebuild;
	}

	void surface_template_base::set_build_mode (build_mode_t mode) {
		m_build_mode = mode;
		m_rebuild = true;
	}

	surface_template_base::surface_template_base (
		const std::string & type_name,
		scene_controller_base & scene, 
		unsigned int patches_x, 
		unsigned int patches_y) : 
		scene_obj_t (scene, type_name, false, false, false) {

		m_patches_x = patches_x;
		m_patches_y = patches_y;
		m_build_mode = build_mode_t::mode_default;
		m_added = false;
		m_rebuild = true;

		m_radius = 2.0f;

		m_combo_item = 0;
	}

	void surface_template_base::configure () {
		if (ImGui::CollapsingHeader ("Builder Options", ImGuiTreeNodeFlags_DefaultOpen)) {
			constexpr const char * items[] = { "default", "cylinder", "cowboy" };
			gui::prefix_label ("Shape: ", 250.0f);

			if (ImGui::Combo ("##surf_builder_shape", &m_combo_item, items, 3)) {
				build_mode_t new_mode;
				switch (m_combo_item) {
				case 1: new_mode = build_mode_t::mode_cylinder; break;
				case 2: new_mode = build_mode_t::mode_hat; break;
				default: new_mode = build_mode_t::mode_default; break;
				}

				if (new_mode != m_build_mode) {
					m_rebuild = true;
					m_build_mode = new_mode;
				}
			}

			gui::prefix_label ("Patches X: ", 250.0f);
			if (ImGui::InputInt ("##surf_patches_x", &m_patches_x)) {
				m_rebuild = true;
			}

			gui::prefix_label ("Patches Y: ", 250.0f);
			if (ImGui::InputInt ("##surf_patches_y", &m_patches_y)) {
				m_rebuild = true;
			}

			if (m_build_mode == build_mode_t::mode_cylinder) {
				gui::prefix_label ("Radius: ", 250.0f);
				if (ImGui::InputFloat ("##surf_radius", &m_radius)) {
					m_rebuild = true;
				}
			}

			if (m_build_mode == build_mode_t::mode_cylinder) {
				gui::clamp (m_patches_x, 3, 15);
			} else {
				gui::clamp (m_patches_x, 1, 15);
			}

			gui::clamp (m_patches_y, 1, 15);
			gui::clamp (m_radius, 0.5f, 4.0f);

			ImGui::NewLine ();
			if (ImGui::Button ("Create Surface", ImVec2 (ImGui::GetWindowWidth (), 24.0f))) {
				t_add_to_scene ();
			}
			ImGui::NewLine ();
		}
	}

	void surface_template_base::integrate (float delta_time) {
		if (m_rebuild) {
			m_rebuild_surface (m_build_mode);
			m_rebuild = false;
		}
	}

	void surface_template_base::m_rebuild_surface (build_mode_t mode) {
		m_build_mode = mode;
		t_rebuild (mode);
	}
}