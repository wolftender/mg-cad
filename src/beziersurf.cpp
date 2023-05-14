#include "beziersurf.hpp"

namespace mini {
	const std::vector<point_wptr> & bezier_patch_c0::t_get_points () const {
		return m_points;
	}

	bool bezier_patch_c0::is_showing_polygon () const {
		return m_show_polygon;
	}

	void bezier_patch_c0::set_showing_polygon (bool show) {
		m_show_polygon = true;
	}

	unsigned int bezier_patch_c0::get_patches_x () const {
		return m_patches_x;
	}

	unsigned int bezier_patch_c0::get_patches_y () const {
		return m_patches_y;
	}

	bezier_patch_c0::bezier_patch_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> grid_shader,
		unsigned int patches_x, unsigned int patches_y, const std::vector<point_ptr> & points) :
		scene_obj_t (scene, "bezier_surf_c0", false, false, false), m_patches_x (patches_x), m_patches_y (patches_y) {

		m_shader = shader;
		m_grid_shader = grid_shader;

		m_points.reserve (points.size ());
		for (const auto & point : points) {
			m_points.push_back (point);
		}
	}

	bezier_patch_c0_template::bezier_patch_c0_template (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<shader_t> grid_shader, 
		std::shared_ptr<shader_t> point_shader, std::shared_ptr<texture_t> point_texture, unsigned int patches_x, unsigned int patches_y) : 
		scene_obj_t (scene, "bezier_surf_c0", true, true, true) {

		m_shader = shader;
		m_grid_shader = grid_shader;
		m_point_shader = point_shader;
		m_point_texture = point_texture;

		m_patches_x = patches_x;
		m_patches_y = patches_y;

		m_rebuild_surface ();
	}

	void bezier_patch_c0_template::configure () {
	}

	void bezier_patch_c0_template::integrate (float delta_time) {
	}

	void bezier_patch_c0_template::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		for (const auto & point : m_points) {
			point->render (context, point->get_matrix ());
		}
	}

	void bezier_patch_c0_template::m_rebuild_surface () {
		m_points.clear ();

		// common endpoints, so each patch "owns" 3 points
		// its first point is also the last point of the previous patch
		unsigned int points_x = (m_patches_x * 3) + 1;
		unsigned int points_y = (m_patches_y * 3) + 1;

		m_patch.reset (nullptr);
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

				m_points[index] = std::make_shared<point_object> (get_scene (), m_point_shader, m_point_texture);

				float tx = (static_cast<float> (x) * spacing) + pos.x;
				float ty = (static_cast<float> (y) * spacing) + pos.z;

				m_points[index]->set_translation ({ tx, 0.0f, ty });
				m_points[index]->set_color ({ 0.0f, 0.0f, 1.0f, 1.0f });
				m_points[index]->set_select_color ({ 0.0f, 1.0f, 0.0f, 1.0f });
			}
		}
	}
}