#include "gregory.hpp"

namespace mini {
	patch_offset_t operator+(const patch_offset_t & a, const patch_offset_t & b) {
		return { a.x + b.x, a.y + b.y };
	}

	patch_offset_t operator-(const patch_offset_t & a, const patch_offset_t & b) {
		return { a.x - b.x, a.y - b.y };
	}

	bool operator==(const patch_offset_t & a, const patch_offset_t & b) {
		return (a.x == b.x && a.y == b.y);
	}

	bool operator!=(const patch_offset_t & a, const patch_offset_t & b) {
		return (a.x != b.x || a.y != b.y);
	}

	gregory_surface::gregory_surface (
		scene_controller_base & scene, 
		std::shared_ptr<shader_t> shader, 
		std::shared_ptr<shader_t> line_shader,
		std::shared_ptr<shader_t> bezier_shader,
		const bicubic_surface::surface_patch & patch1, 
		const bicubic_surface::surface_patch & patch2, 
		const bicubic_surface::surface_patch & patch3,
		const patch_indexing_t & index1,
		const patch_indexing_t & index2,
		const patch_indexing_t & index3) :
		scene_obj_t (scene, "gregory_patch", false, false, false) {

		m_shader = shader;
		m_line_shader = line_shader;
		m_patch1 = patch1;
		m_patch2 = patch2;
		m_patch3 = patch3;

		m_index1 = index1;
		m_index2 = index2;
		m_index3 = index3;

		m_calculate_points ();
	}

	constexpr int at (int x, int y) {
		return (y * 4) + x;
	};

	gregory_surface::~gregory_surface () {
	}

	void gregory_surface::configure () {
	}

	void gregory_surface::integrate (float delta_time) {
		
	}

	void gregory_surface::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		
	}

	void gregory_surface::m_calculate_points () {
		std::array<quarter_surface, 2> patch1;
		std::array<quarter_surface, 2> patch2;
		std::array<quarter_surface, 2> patch3;

		// subdivide each adjacent patch in two
		// then we will use those for algorithm from the lecture
		m_calculate_adjacent_surf (patch1[0], patch1[1], m_patch1, m_index1);
		m_calculate_adjacent_surf (patch2[0], patch2[1], m_patch2, m_index2);
		m_calculate_adjacent_surf (patch3[0], patch3[1], m_patch3, m_index3);

		m_calculate_patch (m_gregory_points1, patch1[1], patch2[0]);
		m_calculate_patch (m_gregory_points2, patch2[1], patch3[0]);
		m_calculate_patch (m_gregory_points3, patch3[1], patch1[0]);
	}

	void gregory_surface::m_calculate_adjacent_surf (
		quarter_surface & surface1,
		quarter_surface & surface2,
		const bicubic_surface::surface_patch & patch, 
		const patch_indexing_t idx) {

		constexpr std::array<std::pair<patch_offset_t, patch_offset_t>, 4> clockwise = {
			std::pair<patch_offset_t, patch_offset_t> {{0, 0}, {3, 0}}, {{3, 0}, {3, 3}}, {{3, 3}, {0, 3}}, {{0, 3}, {0, 0}}
		};

		// first reindex the surface so that it is coherent
		// we need to reindex as patches could have been merged on any two sides
		// so we don't really know which way u and v go
		patch_offset_t u_dir = idx.end - idx.start;
		patch_offset_t v_dir;

		bool is_clockwise = false;
		for (const auto & kv : clockwise) {
			if (kv.first == idx.start && kv.second == idx.end) {
				is_clockwise = true;
				break;
			}
		}

		if (is_clockwise) {
			v_dir = { -u_dir.y, u_dir.x };
		} else {
			v_dir = { u_dir.y, -u_dir.x };
		}

		u_dir.x /= 3;
		u_dir.y /= 3;
		v_dir.x /= 3;
		v_dir.y /= 3;

		patch_offset_t u_offset = idx.start, v_offset;
		quarter_surface surface;

		std::cout << std::endl;
		for (int u = 0; u < 4; ++u) {
			v_offset = u_offset;

			for (int v = 0; v < 4; ++v) {
				surface[at (u,v)] = patch.points[v_offset.x][v_offset.y]->get_translation ();
				std::cout << patch.points[v_offset.x][v_offset.y]->get_name () << " ";

				v_offset = v_offset + v_dir;
			}

			u_offset = u_offset + u_dir;
			std::cout << std::endl;
		}
		std::cout << std::endl;

		// now the easy part, just divide the surface in two using decasteljeu algorithm
		for (int v = 0; v < 4; ++v) {
			glm::vec3 b00 = surface[at (0, v)];
			glm::vec3 b01 = surface[at (1, v)];
			glm::vec3 b02 = surface[at (2, v)];
			glm::vec3 b03 = surface[at (3, v)];

			glm::vec3 b10 = (b00 + b01) / 2.0f;
			glm::vec3 b11 = (b01 + b02) / 2.0f;
			glm::vec3 b12 = (b02 + b03) / 2.0f;

			glm::vec3 b20 = (b10 + b11) / 2.0f;
			glm::vec3 b21 = (b11 + b12) / 2.0f;

			glm::vec3 b30 = (b20 + b21) / 2.0f;

			surface1[at (0, v)] = b00;
			surface1[at (1, v)] = b10;
			surface1[at (2, v)] = b20;
			surface1[at (3, v)] = b30;

			surface2[at (0, v)] = b30;
			surface2[at (1, v)] = b21;
			surface2[at (2, v)] = b12;
			surface2[at (3, v)] = b03;
		}
	}

	void gregory_surface::m_calculate_patch (gregory_patch & patch, const quarter_surface & s1, const quarter_surface & s2) {
		// this is the algorithm from the lecture

	}
}