#include "bsplinesurf.hpp"
#include "surfacetpl.hpp"
#include "serializer.hpp"

namespace mini {
	// this is extremally sub-optimal for b-spline surfaces because
	// it generates many suplicate edges!
	// todo: fix this in the future

	std::vector<GLuint> bspline_surface::s_gen_grid_topology (
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

	std::vector<float> bspline_surface::s_gen_uv(
		unsigned int patches_x, 
		unsigned int patches_y, 
		unsigned int num_points,
		const std::vector<GLuint>& topology) {

		std::vector<float> uv;
		uv.resize(2 * num_points);

		float u_step = 1.0f / static_cast<float>(patches_x);
		float v_step = 1.0f / static_cast<float>(patches_y);

		for (unsigned int py = 0; py < patches_y; ++py) {
			for (unsigned int px = 0; px < patches_x; ++px) {
				unsigned int patch_idx = py * patches_x + px;
				unsigned int base_idx = patch_idx * 16;

				for (unsigned int y = 0; y < 4; ++y) {
					for (unsigned int x = 0; x < 4; ++x) {
						float fpx = static_cast<float>(px);
						float fpy = static_cast<float>(py);
						float fx = static_cast<float>(x);
						float fy = static_cast<float>(y);

						unsigned int local_idx = (4 * y) + x;

						float u = (fpx * u_step) + (fx * u_step) - u_step;
						float v = (fpy * v_step) + (fy * v_step) - v_step;

						unsigned int base = 2 * topology[base_idx + local_idx];

						uv[base + 0] = u;
						uv[base + 1] = v;
					}
				}
			}
		}

		return uv;
	}

	bspline_surface::bspline_surface (
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
			"bezier_surf_c2",
			shader,
			solid_shader,
			grid_shader,
			patches_x,
			patches_y,
			points
		) { 
		
		// validity check
		if ((3 + patches_x) * (3 + patches_y) != points.size ()) {
			throw std::runtime_error ("invalid input data for a bicubic surface patch");
		}

		m_u_wrapped = u_wrapped;
		m_v_wrapped = v_wrapped;
	}

	bspline_surface::bspline_surface (
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
			"bezier_surf_c2",
			shader,
			solid_shader,
			grid_shader,
			patches_x,
			patches_y,
			points,
			s_gen_uv(patches_x, patches_y, points.size(), topology),
			topology,
			s_gen_grid_topology (patches_x, patches_y, topology)
		) { 
		
		// topology validity check
		if ((patches_x * patches_y * 16) != topology.size ()) {
			throw std::runtime_error ("invalid topology data for a bspline surface patch");
		}

		m_u_wrapped = u_wrapped;
		m_v_wrapped = v_wrapped;
	}

	const object_serializer_base & bspline_surface::get_serializer () const {
		return generic_object_serializer<bspline_surface>::get_instance ();
	}

	float bspline_surface::get_min_u() const {
		return 0.0f;
	}

	float bspline_surface::get_max_u() const {
		return 1.0f;
	}

	float bspline_surface::get_min_v() const {
		return 0.0f;
	}

	float bspline_surface::get_max_v() const {
		return 1.0f;
	}

	glm::vec3 bspline_evaluate(glm::vec3 b00, glm::vec3 b01, glm::vec3 b02, glm::vec3 b03, float t) {
		float N00 = 1.0f;
		float N10 = (1.0f - t) * N00;
		float N11 = t * N00;
		float N20 = ((1.0f - t) * N10) / 2.0f;
		float N21 = ((1.0f + t) * N10 + (2.0f - t) * N11) / 2.0f;
		float N22 = (t * N11) / 2.0f;
		float N30 = ((1.0f - t) * N20) / 3.0f;
		float N31 = ((2.0f + t) * N20 + (2.0f - t) * N21) / 3.0f;
		float N32 = ((1.0f + t) * N21 + (3.0f - t) * N22) / 3.0f;
		float N33 = (t * N22) / 3.0f;

		return b00 * N30 + b01 * N31 + b02 * N32 + b03 * N33;
	}

	glm::vec3 bspline_derivative(glm::vec3 b00, glm::vec3 b01, glm::vec3 b02, glm::vec3 b03, float t) {
		float N00 = 1.0f;
		float N10 = (1.0f - t) * N00;
		float N11 = t * N00;
		float N20 = ((1.0f - t) * N10) / 2.0f;
		float N21 = ((1.0f + t) * N10 + (2.0f - t) * N11) / 2.0f;
		float N22 = (t * N11) / 2.0f;

		float dN30 = -N20;
		float dN31 = N20 - N21;
		float dN32 = N21 - N22;
		float dN33 = N22;

		return b00 * dN30 + b01 * dN31 + b02 * dN32 + b03 * dN33;
	}

	inline void local_param(int px, int py, float nu, float nv, float& lu, float& lv) {
		lu = nu - static_cast<float>(px);
		lv = nv - static_cast<float>(py);
	}

	glm::vec3 bspline_surface::sample(float u, float v) const {
		float nu = static_cast<float>(get_patches_x()) * glm::clamp(u, 0.0f, 1.0f);
		float nv = static_cast<float>(get_patches_y()) * glm::clamp(v, 0.0f, 1.0f);

		int patch_x = glm::min(static_cast<unsigned int>(floorf(nu)), get_patches_x() - 1);
		int patch_y = glm::min(static_cast<unsigned int>(floorf(nv)), get_patches_y() - 1);

		const auto p = [this, &patch_x, &patch_y](int x, int y) -> const glm::vec3& {
			return point_at(patch_x, patch_y, x, y);
			};

		float lu, lv;
		local_param(patch_x, patch_y, nu, nv, lu, lv);

		auto p0 = bspline_evaluate(p(0, 0), p(0, 1), p(0, 2), p(0, 3), lv);
		auto p1 = bspline_evaluate(p(1, 0), p(1, 1), p(1, 2), p(1, 3), lv);
		auto p2 = bspline_evaluate(p(2, 0), p(2, 1), p(2, 2), p(2, 3), lv);
		auto p3 = bspline_evaluate(p(3, 0), p(3, 1), p(3, 2), p(3, 3), lv);

		return bspline_evaluate(p0, p1, p2, p3, lu);
	}

	glm::vec3 bspline_surface::normal(float u, float v) const {
		auto du = ddu(u, v);
		auto dv = ddv(u, v);

		return glm::normalize(glm::cross(du, dv));
	}

	glm::vec3 bspline_surface::ddu(float u, float v) const {
		float nu = static_cast<float>(get_patches_x()) * glm::clamp(u, 0.0f, 1.0f);
		float nv = static_cast<float>(get_patches_y()) * glm::clamp(v, 0.0f, 1.0f);

		int patch_x = glm::min(static_cast<unsigned int>(floorf(nu)), get_patches_x() - 1);
		int patch_y = glm::min(static_cast<unsigned int>(floorf(nv)), get_patches_y() - 1);

		const auto p = [this, &patch_x, &patch_y](int x, int y) -> const glm::vec3& {
			return point_at(patch_x, patch_y, x, y);
			};

		float lu, lv;
		local_param(patch_x, patch_y, nu, nv, lu, lv);

		auto p0 = bspline_evaluate(p(0, 0), p(0, 1), p(0, 2), p(0, 3), lv);
		auto p1 = bspline_evaluate(p(1, 0), p(1, 1), p(1, 2), p(1, 3), lv);
		auto p2 = bspline_evaluate(p(2, 0), p(2, 1), p(2, 2), p(2, 3), lv);
		auto p3 = bspline_evaluate(p(3, 0), p(3, 1), p(3, 2), p(3, 3), lv);

		return bspline_derivative(p0, p1, p2, p3, lu);
	}

	glm::vec3 bspline_surface::ddv(float u, float v) const {
		float nu = static_cast<float>(get_patches_x()) * glm::clamp(u, 0.0f, 1.0f);
		float nv = static_cast<float>(get_patches_y()) * glm::clamp(v, 0.0f, 1.0f);

		int patch_x = glm::min(static_cast<unsigned int>(floorf(nu)), get_patches_x() - 1);
		int patch_y = glm::min(static_cast<unsigned int>(floorf(nv)), get_patches_y() - 1);

		const auto p = [this, &patch_x, &patch_y](int x, int y) -> const glm::vec3& {
			return point_at(patch_x, patch_y, x, y);
			};

		float lu, lv;
		local_param(patch_x, patch_y, nu, nv, lu, lv);

		auto p0 = bspline_evaluate(p(0, 0), p(1, 0), p(2, 0), p(3, 0), lu);
		auto p1 = bspline_evaluate(p(0, 1), p(1, 1), p(2, 1), p(3, 1), lu);
		auto p2 = bspline_evaluate(p(0, 2), p(1, 2), p(2, 2), p(3, 2), lu);
		auto p3 = bspline_evaluate(p(0, 3), p(1, 3), p(2, 3), p(3, 3), lu);

		return bspline_derivative(p0, p1, p2, p3, lv);
	}

	bool bspline_surface::is_u_wrapped() const {
		return m_u_wrapped;
	}

	bool bspline_surface::is_v_wrapped() const {
		return m_v_wrapped;
	}

	bool bspline_surface::is_trimmable() const {
		return true;
	}

	trimmable_surface_domain& bspline_surface::get_trimmable_domain() {
		return get_domain();
	}

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

	void bspline_surface::t_calc_uv_buffer(std::vector<float>& uv, const std::vector<GLuint>& indices) {
		 uv = s_gen_uv(get_patches_x(), get_patches_y(), get_num_points(), indices);
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

		bool u_wrapped = false;
		bool v_wrapped = false;

		if (mode == build_mode_t::mode_cylinder) {
			u_wrapped = true;
		}

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
						//float r = get_radius () * (3.0f - glm::cos (2.0f * glm::pi<float> () / static_cast<float> (points_x - 3))) / 2.0f;
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
			m_points,
			u_wrapped,
			v_wrapped
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