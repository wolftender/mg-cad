#include "bsplinesurf.hpp"
#include "surfacetpl.hpp"

namespace mini {
	bspline_surface::bspline_surface (
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

	bspline_surface::bspline_surface (
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

	void bspline_surface::t_calc_idx_buffer (std::vector<GLuint> & indices, std::vector<GLuint> & grid_indices) {
		// create indices for patches
		unsigned int i = 0;
		unsigned int width = 4 + get_patches_x () - 1;
		unsigned int height = 4 + get_patches_y () - 1;

		for (unsigned int py = 0; py < get_patches_y (); ++py) {
			for (unsigned int px = 0; px < get_patches_x (); ++px) {
				// add all control points to the patch
				unsigned int bx = px;
				unsigned int by = py;

				for (unsigned int y = 0; y < 4; ++y) {
					for (unsigned int x = 0; x < 4; ++x) {
						unsigned int cx = bx + x;
						unsigned int cy = by + y;

						unsigned int index = (cy * width) + cx;
						indices[i++] = index;
					}
				}
			}
		}

		// for the grid we will need a different procedure
		grid_indices.resize (2 * (((width - 1) * height) + (width * (height - 1))));
		i = 0;

		for (unsigned int cy = 0; cy < height; ++cy) {
			unsigned int i1, i2;

			for (unsigned int cx = 0; cx < width - 1; ++cx) {
				i1 = (cy * width) + cx;
				i2 = (cy * width) + cx + 1;

				grid_indices[i++] = i1;
				grid_indices[i++] = i2;
			}

			if (cy == height - 1) {
				break;
			}

			for (unsigned int cx = 0; cx < width; ++cx) {
				i1 = (cy * width) + cx;
				i2 = ((cy + 1) * width) + cx;

				grid_indices[i++] = i1;
				grid_indices[i++] = i2;
			}
		}
	}

	////////////////////////////////////////////////////////


	template<> void surface_template<bspline_surface>::t_rebuild (surface_template_base::build_mode_t mode) {
		m_points.clear ();

		// common endpoints, so each patch "owns" 3 points
		// its first point is also the last point of the previous patch
		unsigned int points_x = 4 + get_patches_x () - 1;
		unsigned int points_y = 4 + get_pathces_y () - 1;

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

				if (mode == build_mode_t::mode_cylinder) {
					if (x >= points_x - 3) {
						int sx = points_x - x;
						m_points[index] = m_points[y * points_x + 3 - sx];
						continue;
					}
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
						float t = static_cast<float> (x) / static_cast<float> (points_x - 3) * 2.0f * glm::pi<float> ();
						float r = get_radius ();

						// just using this actually looks pretty nice for bspline surfaces
						tx = r * glm::cos (t);
						ty = r * glm::sin (t);
						tz = (static_cast<float> (y) * spacing) + pos.z;
					}
					break;

					case build_mode_t::mode_hat:
					{
						tx = (static_cast<float> (x) * spacing) + pos.x;
						tz = (static_cast<float> (y) * spacing) + pos.z;

						float dx = tx - center.x;
						float dy = tz - center.z;

						ty = -glm::exp (1.0f / (1.0f + 0.25f * (dx * dx + dy * dy)));
					}

					break;

					default: break;
				}

				m_points[index]->set_translation ({ tx, ty, tz });
				m_points[index]->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				m_points[index]->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
			}
		}

		m_patch = std::make_shared<bspline_surface> (
			get_scene (),
			m_shader,
			m_solid_shader,
			m_grid_shader,
			get_patches_x (),
			get_pathces_y (),
			m_points
		);
	}

	template<> void surface_template<bspline_surface>::t_add_to_scene () {
		auto & scene = get_scene ();
		scene.add_object (get_name (), m_patch);

		unsigned int points_x = 4 + get_patches_x () - 1;
		unsigned int points_y = 4 + get_pathces_y () - 1;

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
}