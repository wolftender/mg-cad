#include "beziersurf.hpp"
#include "surfacetpl.hpp"
#include "gui.hpp"
#include "serializer.hpp"

namespace mini {
	std::vector<GLuint> bezier_surface_c0::s_gen_grid_topology (
		unsigned int patches_x,
		unsigned int patches_y,
		const std::vector<GLuint> & topology) {

		std::vector<GLuint> grid_indices;
		unsigned int num_patches = patches_x * patches_y;

		grid_indices.resize (num_patches * 80);
		int i = 0;

		for (unsigned int p = 0; p < num_patches; ++p) {
			constexpr unsigned int patch_size = 4;
			unsigned int i1, i2;

			for (unsigned int y = 0; y < patch_size; ++y) {
				for (unsigned int x = 0; x < patch_size - 1; ++x) {
					i1 = topology[(16 * p) + (y * patch_size) + x];
					i2 = topology[(16 * p) + (y * patch_size) + x + 1];

					grid_indices[i++] = i1;
					grid_indices[i++] = i2;
				}

				if (y == patch_size - 1) {
					break;
				}

				for (unsigned int x = 0; x < patch_size; ++x) {
					i1 = topology[(16 * p) + (y * patch_size) + x];
					i2 = topology[(16 * p) + ((y + 1) * patch_size) + x];

					grid_indices[i++] = i1;
					grid_indices[i++] = i2;
				}
			}
		}

		return grid_indices;
	}

	bezier_surface_c0::bezier_surface_c0 (
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, 
		unsigned int patches_x, 
		unsigned int patches_y, 
		const std::vector<point_ptr> & points,
		bool u_wrapped,
		bool v_wrapped)
		: bicubic_surface (
			scene,
			"bezier_surf_c0",
			shader,
			solid_shader,
			grid_shader,
			patches_x,
			patches_y,
			points
		) {

		m_u_wrapped = u_wrapped;
		m_v_wrapped = v_wrapped;
		
		// validity check
		if ((patches_y * patches_x * 9 + patches_x * 3 + patches_y * 3 + 1) != points.size ()) {
			throw std::runtime_error ("invalid input data for a bicubic surface patch");
		}
	}

	bezier_surface_c0::bezier_surface_c0 (
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> solid_shader, 
		std::shared_ptr<shader_t> grid_shader, 
		unsigned int patches_x, 
		unsigned int patches_y, 
		const std::vector<point_ptr> & points, 
		const std::vector<GLuint> & topology,
		bool u_wrapped,
		bool v_wrapped)
		: bicubic_surface (
			scene,
			"bezier_surf_c0",
			shader,
			solid_shader,
			grid_shader,
			patches_x,
			patches_y,
			points,
			topology,
			s_gen_grid_topology (patches_x, patches_y, topology)
		) {

		m_u_wrapped = u_wrapped;
		m_v_wrapped = v_wrapped;
		
		// topology validity check
		if ((patches_x * patches_y * 16) != topology.size ()) {
			throw std::runtime_error ("invalid topology data for a bezier surface patch");
		}
	}

	const object_serializer_base & bezier_surface_c0::get_serializer () const {
		return generic_object_serializer<bezier_surface_c0>::get_instance ();
	}

	float bezier_surface_c0::get_min_u() const {
		return 0.0f;
	}

	float bezier_surface_c0::get_max_u() const {
		return 1.0f;
	}

	float bezier_surface_c0::get_min_v() const {
		return 0.0f;
	}

	float bezier_surface_c0::get_max_v() const {
		return 1.0f;
	}

	inline glm::vec3 bezier_evaluate(
		const glm::vec3 & b00, 
		const glm::vec3 & b01, 
		const glm::vec3 & b02, 
		const glm::vec3 & b03, 
		float t) {

		float t1 = t;
		float t0 = 1.0 - t;

		glm::vec3 b10, b11, b12;
		glm::vec3 b20, b21;
		glm::vec3 b30;

		b10 = t0 * b00 + t1 * b01;
		b11 = t0 * b01 + t1 * b02;
		b12 = t0 * b02 + t1 * b03;

		b20 = t0 * b10 + t1 * b11;
		b21 = t0 * b11 + t1 * b12;

		b30 = t0 * b20 + t1 * b21;

		return b30;
	}

	inline glm::vec3 bezier_derivative(
		const glm::vec3 & b00,
		const glm::vec3 & b01,
		const glm::vec3 & b02,
		const glm::vec3 & b03,
		float t
	) {
		float t1 = t;
		float t0 = 1.0 - t;

		glm::vec3 d10 = -3.0f * b00 + 3.0f * b01;
		glm::vec3 d11 = -3.0f * b01 + 3.0f * b02;
		glm::vec3 d12 = -3.0f * b02 + 3.0f * b03;

		glm::vec3 d20 = t0 * d10 + t1 * d11;
		glm::vec3 d21 = t0 * d11 + t1 * d12;

		glm::vec3 d30 = t0 * d20 + t1 * d21;
		return d30;
	}

	inline void local_param(int px, int py, float nu, float nv, float & lu, float & lv) {
		lu = nu - static_cast<float>(px);
		lv = nv - static_cast<float>(py);
	}

	glm::vec3 bezier_surface_c0::sample(float u, float v) const {
		float nu = static_cast<float>(get_patches_x()) * glm::clamp(u, 0.0f, 1.0f);
		float nv = static_cast<float>(get_patches_y()) * glm::clamp(v, 0.0f, 1.0f);

		int patch_x = glm::min(static_cast<unsigned int>(floorf(nu)), get_patches_x() - 1);
		int patch_y = glm::min(static_cast<unsigned int>(floorf(nv)), get_patches_y() - 1);

		const auto p = [this, &patch_x, &patch_y](int x, int y) -> const glm::vec3 & {
			return point_at(patch_x, patch_y, x, y);
		};

		float lu, lv;
		local_param(patch_x, patch_y, nu, nv, lu, lv);
		
		auto p0 = bezier_evaluate(p(0, 0), p(0, 1), p(0, 2), p(0, 3), lv);
		auto p1 = bezier_evaluate(p(1, 0), p(1, 1), p(1, 2), p(1, 3), lv);
		auto p2 = bezier_evaluate(p(2, 0), p(2, 1), p(2, 2), p(2, 3), lv);
		auto p3 = bezier_evaluate(p(3, 0), p(3, 1), p(3, 2), p(3, 3), lv);

		return bezier_evaluate(p0, p1, p2, p3, lu);
	}

	glm::vec3 bezier_surface_c0::normal(float u, float v) const {
		auto du = ddu(u, v);
		auto dv = ddv(u, v);

		return glm::normalize(glm::cross(du, dv));
	}

	glm::vec3 bezier_surface_c0::ddu(float u, float v) const {
		float nu = static_cast<float>(get_patches_x()) * glm::clamp(u, 0.0f, 1.0f);
		float nv = static_cast<float>(get_patches_y()) * glm::clamp(v, 0.0f, 1.0f);

		int patch_x = glm::min(static_cast<unsigned int>(floorf(nu)), get_patches_x() - 1);
		int patch_y = glm::min(static_cast<unsigned int>(floorf(nv)), get_patches_y() - 1);

		const auto p = [this, &patch_x, &patch_y](int x, int y) -> const glm::vec3 & {
			return point_at(patch_x, patch_y, x, y);
		};

		float lu, lv;
		local_param(patch_x, patch_y, nu, nv, lu, lv);

		auto p0 = bezier_evaluate(p(0, 0), p(0, 1), p(0, 2), p(0, 3), lv);
		auto p1 = bezier_evaluate(p(1, 0), p(1, 1), p(1, 2), p(1, 3), lv);
		auto p2 = bezier_evaluate(p(2, 0), p(2, 1), p(2, 2), p(2, 3), lv);
		auto p3 = bezier_evaluate(p(3, 0), p(3, 1), p(3, 2), p(3, 3), lv);

		return bezier_derivative(p0, p1, p2, p3, lu);
	}

	glm::vec3 bezier_surface_c0::ddv(float u, float v) const {
		float nu = static_cast<float>(get_patches_x()) * glm::clamp(u, 0.0f, 1.0f);
		float nv = static_cast<float>(get_patches_y()) * glm::clamp(v, 0.0f, 1.0f);

		int patch_x = glm::min(static_cast<unsigned int>(floorf(nu)), get_patches_x() - 1);
		int patch_y = glm::min(static_cast<unsigned int>(floorf(nv)), get_patches_y() - 1);

		const auto p = [this, &patch_x, &patch_y](int x, int y) -> const glm::vec3 & {
			return point_at(patch_x, patch_y, x, y);
		};

		float lu, lv;
		local_param(patch_x, patch_y, nu, nv, lu, lv);

		auto p0 = bezier_evaluate(p(0, 0), p(1, 0), p(2, 0), p(3, 0), lu);
		auto p1 = bezier_evaluate(p(0, 1), p(1, 1), p(2, 1), p(3, 1), lu);
		auto p2 = bezier_evaluate(p(0, 2), p(1, 2), p(2, 2), p(3, 2), lu);
		auto p3 = bezier_evaluate(p(0, 3), p(1, 3), p(2, 3), p(3, 3), lu);

		return bezier_derivative(p0, p1, p2, p3, lv);
	}

	bool bezier_surface_c0::is_u_wrapped() const {
		return m_u_wrapped;
	}

	bool bezier_surface_c0::is_v_wrapped() const {
		return m_v_wrapped;
	}

	void bezier_surface_c0::t_calc_idx_buffer (std::vector<GLuint> & indices, std::vector<GLuint> & grid_indices) {
		// create indices for patches
		unsigned int i = 0;
		unsigned int width = get_patches_x () * 3 + 1;
		unsigned int height = get_patches_y () * 3 + 1;

		for (unsigned int py = 0; py < get_patches_y (); ++py) {
			for (unsigned int px = 0; px < get_patches_x (); ++px) {
				// add all control points to the patch
				unsigned int bx = px * 3;
				unsigned int by = py * 3;

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


	//////////////////////////////////////////////////////////


	template<> void surface_template<bezier_surface_c0>::t_rebuild (surface_template_base::build_mode_t mode) {
		m_points.clear ();

		// common endpoints, so each patch "owns" 3 points
		// its first point is also the last point of the previous patch
		unsigned int points_x = (get_patches_x () * 3) + 1;
		unsigned int points_y = (get_pathces_y () * 3) + 1;

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
					float t = static_cast<float> (x) / static_cast<float> (points_x - 1) * 2.0f * glm::pi<float> ();
					float r = get_radius ();

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

		m_patch = std::make_shared<bezier_surface_c0> (
			get_scene (), 
			m_shader, 
			m_solid_shader, 
			m_grid_shader, 
			get_patches_x (),
			get_pathces_y (), 
			m_points,
			false,
			false
		);
	}

	template<> void surface_template<bezier_surface_c0>::t_add_to_scene () {
		auto & scene = get_scene ();
		scene.add_object (get_name (), m_patch);

		unsigned int points_x = (get_patches_x () * 3) + 1;
		unsigned int points_y = (get_pathces_y () * 3) + 1;

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