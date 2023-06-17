#include <unordered_set>

#include "gapfilling.hpp"

namespace mini {
	struct edge_data {
		unsigned int patch_id;
		bool is_edge;
	};

	struct surface_triangle {
		int u, v, w;

		surface_triangle (int _u, int _v, int _w) {
			u = _u;
			v = _v;
			w = _w;

			if (u > w) {
				std::swap (u, w);
			}

			if (u > v) {
				std::swap (u, v);
			}

			if (v > w) {
				std::swap (v, w);
			}
		}
	};

	bool operator< (const surface_triangle & a, const surface_triangle & t) {
		return a.u < t.u || (a.u == t.u && (a.v < t.v || (a.v == t.v && a.w < t.w)));
	}

	gap_filling_controller::gap_filling_controller (scene_controller_base & scene, std::shared_ptr<resource_store> store) : 
		m_scene (scene), m_store (store) {
		std::unordered_set<uint64_t> vertex_set;

		for (auto iter = m_scene.get_selected_objects (); iter->has (); iter->next ()) {
			auto surface = std::dynamic_pointer_cast<bezier_surface_c0> (iter->get_object ());

			if (surface) {
				unsigned int patches_x = surface->get_patches_x ();
				unsigned int patches_y = surface->get_patches_y ();

				for (unsigned int i = 0; i < patches_x * patches_y; ++i) {
					int px = i % patches_x;
					int py = i / patches_x;

					auto patch = surface->get_patch (px, py);
					m_patches.push_back (patch);

					vertex_set.insert (patch.points[0][0]->get_id ());
					vertex_set.insert (patch.points[0][3]->get_id ());
					vertex_set.insert (patch.points[3][0]->get_id ());
					vertex_set.insert (patch.points[3][3]->get_id ());
				}
			}
		}

		unsigned int num_patches = m_patches.size ();
		unsigned int num_vertices = vertex_set.size ();

		std::vector<uint64_t> V (num_vertices);
		std::vector<int> adjV (num_vertices);
		std::vector<edge_data> E (num_vertices * num_vertices);
		std::unordered_map<uint64_t, int> vertex_map;
		std::list<std::pair<int, int>> edge_list;

		std::fill (adjV.begin (), adjV.end (), 0);

		auto index = [num_vertices](int v, int u) constexpr -> int {
			return v * num_vertices + u;
		};

		auto set_edge = [&E, &index, &edge_list](int v, int u, unsigned int p) -> void {
			if (E[index (v, u)].is_edge) {
				return;
			}

			E[index (v, u)] = E[index (u, v)] = { p, true }; 
			edge_list.push_back ({ u, v });
		};

		int idx = 0;
		for (auto v : vertex_set) {
			V[idx] = v;
			vertex_map.insert ({v, idx});
			idx++;
		}

		vertex_set.clear ();
		for (unsigned int patch_id = 0; patch_id < m_patches.size (); ++patch_id) {
			auto & patch = m_patches[patch_id];
			int p00 = vertex_map[patch.points[0][0]->get_id ()];
			int p03 = vertex_map[patch.points[0][3]->get_id ()];
			int p30 = vertex_map[patch.points[3][0]->get_id ()];
			int p33 = vertex_map[patch.points[3][3]->get_id ()];

			set_edge (p00, p03, patch_id);
			set_edge (p03, p33, patch_id);
			set_edge (p33, p30, patch_id);
			set_edge (p30, p00, patch_id);

			adjV[p00]++;
			adjV[p03]++;
			adjV[p30]++;
			adjV[p33]++;
		}

		// graph is constructed, now we can do the big scary algorithm
		std::set<surface_triangle> triangles;

		for (const auto & edge : edge_list) {
			auto u = edge.first, v = edge.second;

			for (int w = 0; w < V.size (); ++w) {
				if (w != u && w != v && u != v && 
					E[index(v, w)].is_edge && E[index(w, u)].is_edge &&
					(adjV[v] < 4 || adjV[u] < 4 || adjV[w] < 4)) {
					triangles.insert(surface_triangle {u, v, w});
				}
			}
		}

		for (const auto & tri : triangles) {
			int p1 = E[index (tri.u, tri.v)].patch_id;
			int p2 = E[index (tri.v, tri.w)].patch_id;
			int p3 = E[index (tri.w, tri.u)].patch_id;

			m_gaps.push_back (surface_gap_t {p1, p2, p3, V[tri.u], V[tri.v], V[tri.w]});
		}
	}

	gap_filling_controller::~gap_filling_controller () { }

	void gap_filling_controller::m_get_offsets (
		bicubic_surface::surface_patch & patch, 
		uint64_t p1, 
		uint64_t p2, 
		patch_offset_t & start, 
		patch_offset_t & end) {

		constexpr std::array<patch_offset_t, 4> offsets = { patch_offset_t {0,0}, {3,0}, {0,3}, {3,3} };

		for (const auto & o : offsets) {
			if (patch.points[o.x][o.y]->get_id () == p1) {
				start = o;
			}
			
			if (patch.points[o.x][o.y]->get_id () == p2) {
				end = o;
			}
		}
	}

	void gap_filling_controller::m_spawn_debug_curve (bicubic_surface::surface_patch & patch, const patch_offset_t & start, const patch_offset_t & end) {
		point_list control_points;
		for (int x = start.x, y = start.y, i = 0; i < 4; x += (end.x - start.x) / 3, y += (end.y - start.y) / 3, i++) {
			control_points.push_back (patch.points[x][y]);
			std::cout << patch.points[x][y]->get_name () << ", ";
		}
		std::cout << std::endl;

		auto curve = std::make_shared<bezier_curve_c0> (
			m_scene,
			m_store->get_bezier_shader (),
			m_store->get_line_shader (),
			control_points
		);

		curve->set_color ({ 1.0f, 0.0f, 0.0f, 1.0f });
		m_scene.add_object ("test_curve", curve);
	}

	void gap_filling_controller::create_surfaces () {
		for (const auto & gap : m_gaps) {
			patch_offset_t patch1_start, patch1_end;
			patch_offset_t patch2_start, patch2_end;
			patch_offset_t patch3_start, patch3_end;

			/*std::cout <<
				m_scene.get_object (gap.point1)->get_name () << " " <<
				m_scene.get_object (gap.point2)->get_name () << " " <<
				m_scene.get_object (gap.point3)->get_name () << std::endl;*/

			m_get_offsets (m_patches[gap.patch1], gap.point1, gap.point2, patch1_start, patch1_end);
			m_get_offsets (m_patches[gap.patch2], gap.point2, gap.point3, patch2_start, patch2_end);
			m_get_offsets (m_patches[gap.patch3], gap.point3, gap.point1, patch3_start, patch3_end);

			// m_spawn_debug_curve (m_patches[gap.patch1], patch1_start, patch1_end);
			// m_spawn_debug_curve (m_patches[gap.patch2], patch2_start, patch2_end);
			// m_spawn_debug_curve (m_patches[gap.patch3], patch3_start, patch3_end);

			auto gregory_surf = std::make_shared<gregory_surface> (
				m_scene,
				m_store->get_gregory_surf_shader (),
				m_store->get_gregory_surf_solid_shader (),
				m_store->get_line_shader (),
				m_store->get_bezier_shader (),
				m_patches[gap.patch1],
				m_patches[gap.patch2],
				m_patches[gap.patch3],
				patch_indexing_t { patch1_start, patch1_end },
				patch_indexing_t { patch2_start, patch2_end },
				patch_indexing_t { patch3_start, patch3_end }
			);

			m_scene.add_object ("gregory_surface", gregory_surf);
		}
	}
}