#include "beziersurf.hpp"
#include "gui.hpp"
#include "serializer.hpp"

namespace mini {
	bezier_surface_c0::bezier_surface_c0 (
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, 
		unsigned int patches_x, 
		unsigned int patches_y, 
		const std::vector<point_ptr> & points)
		: bicubic_surface (
			"bezier_surf_c0",
			scene,
			shader,
			solid_shader,
			grid_shader,
			patches_x,
			patches_y,
			points
		) { }

	bezier_surface_c0::bezier_surface_c0 (
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, 
		unsigned int patches_x, 
		unsigned int patches_y, 
		const std::vector<point_ptr> & points, 
		const std::vector<GLuint> topology)
		: bicubic_surface (
			"bezier_surf_c0",
			scene,
			shader,
			solid_shader,
			grid_shader,
			patches_x,
			patches_y,
			points,
			topology
		) { }

	const object_serializer_base & bezier_surface_c0::get_serializer () const {
		return generic_object_serializer<bezier_surface_c0>::get_instance ();
	}


	//////////////////////////////////////////////////////////


	bezier_patch_c0_template::bezier_patch_c0_template (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, 
		unsigned int patches_x, unsigned int patches_y) : 
		scene_obj_t (scene, "bezier_surf_c0", false, false, false) {

		m_shader = shader;
		m_solid_shader = solid_shader;
		m_grid_shader = grid_shader;
		m_point_shader = point_shader;
		m_point_texture = point_texture;

		m_patches_x = patches_x;
		m_patches_y = patches_y;
		m_rebuild = false;

		m_radius = 2.0f;

		m_combo_item = 0;
		m_rebuild_surface (build_mode_t::mode_default);
	}

	void bezier_patch_c0_template::configure () {
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
				m_add_to_scene ();
			}
			ImGui::NewLine ();
		}

		if (m_patch) {
			m_patch->configure ();
		}
	}

	void bezier_patch_c0_template::integrate (float delta_time) {
		if (m_rebuild) {
			m_rebuild_surface (m_build_mode);
			m_rebuild = false;
		}
	}

	void bezier_patch_c0_template::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_patch) {
			m_patch->render (context, world_matrix);
		}

		for (const auto & point : m_points) {
			point->render (context, point->get_matrix ());
		}
	}

	void bezier_patch_c0_template::m_add_to_scene () {
		auto & scene = get_scene ();
		scene.add_object (get_name (), m_patch);

		unsigned int points_x = (m_patches_x * 3) + 1;
		unsigned int points_y = (m_patches_y * 3) + 1;

		for (unsigned int x = 0; x < points_x; ++x) {
			for (unsigned int y = 0; y < points_y; ++y) {
				unsigned int index = (y * points_x) + x;

				m_points[index]->set_color (point_object::s_color_default);
				m_points[index]->set_select_color (point_object::s_select_default);

				scene.add_object ("point", m_points[index]);
			}
		}

		scene.clear_selection ();
		for (const auto & point : m_points) {
			scene.select_by_id (point->get_id ());
		}

		scene.select_by_id (m_patch->get_id ());
		dispose ();
	}

	void bezier_patch_c0_template::m_rebuild_surface (bezier_patch_c0_template::build_mode_t mode) {
		m_build_mode = mode;
		m_points.clear ();

		// common endpoints, so each patch "owns" 3 points
		// its first point is also the last point of the previous patch
		unsigned int points_x = (m_patches_x * 3) + 1;
		unsigned int points_y = (m_patches_y * 3) + 1;

		m_patch = nullptr;
		m_points.clear ();

		m_points.resize (points_x * points_y);

		constexpr float spacing = 0.75f;

		const auto & center = get_translation ();
		const glm::vec3 pos{
			center.x - (static_cast<float>(points_x - 1) * spacing / 2.0f),
			center.y,
			center.z - (static_cast<float>(points_y - 1) * spacing / 2.0f)
		};

		for (unsigned int x = 0; x < points_x; ++x) {
			for (unsigned int y = 0; y < points_y; ++y) {
				unsigned int index = (y * points_x) + x;

				if (mode == build_mode_t::mode_cylinder && x == points_x - 1) {
					m_points[index] = m_points[y * points_x];
					continue;
				}

				m_points[index] = std::make_shared<point_object> (get_scene (), m_point_shader, m_point_texture);

				float tx = 0.0f, ty = 0.0f, tz = 0.0f;

				switch (mode) {
					case build_mode_t::mode_default:
						tx = (static_cast<float> (x) * spacing) + pos.x;
						ty = 0.0f;
						tz = (static_cast<float> (y) * spacing) + pos.z;
						break;

					case build_mode_t::mode_cylinder:
						{
							float t = static_cast<float> (x) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float>();
							float r = m_radius;

							int xmod = x % 3;
							if (xmod == 0) {
								tx = r * glm::cos (t);
								ty = r * glm::sin (t);
								tz = (static_cast<float> (y) * spacing) + pos.z;
							} else {
								float tp;
								if (xmod == 1) {
									tp = static_cast<float> (x - 1) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float> ();
								} else {
									tp = static_cast<float> (x + 1) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float> ();
								}

								float xp = r * glm::cos (tp);
								float yp = r * glm::sin (tp);

								float dx = -glm::sin (tp);
								float dy = glm::cos (tp);

								if (xmod != 1) {
									dx = -dx;
									dy = -dy;
								}

								float dang = (2.0f * glm::pi<float> () / static_cast<float> (points_x));
								float len = r * glm::tan (dang);

								tx = xp + len * dx;
								ty = yp + len * dy;
								tz = (static_cast<float> (y) * spacing) + pos.z;
							}
						}
						break;

					case build_mode_t::mode_hat:
						{
							tx = (static_cast<float> (x) * spacing) + pos.x;
							tz = (static_cast<float> (y) * spacing) + pos.z;

							float dx = tx - center.x;
							float dy = tz - center.z;

							ty = -glm::exp (1.0f / (1.0f +  0.25f * (dx * dx + dy * dy)));
						}

						break;

					default: break;
				}

				m_points[index]->set_translation ({ tx, ty, tz });
				m_points[index]->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				m_points[index]->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
			}
		}

		m_patch = std::make_shared<bezier_surface_c0> (get_scene (), m_shader, m_solid_shader, m_grid_shader, m_patches_x, m_patches_y, m_points);
	}
}