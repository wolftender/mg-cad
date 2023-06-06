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

	gap_filling_controller::gap_filling_controller (scene_controller_base & scene) : m_scene (scene) {
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
		std::vector<int> degV (num_vertices);
		std::vector<edge_data> E (num_vertices * num_vertices);
		std::unordered_map<uint64_t, int> vertex_map;
		std::list<std::pair<int, int>> edge_list;

		std::fill (degV.begin (), degV.end (), 0);

		auto index = [num_vertices](int v, int u) constexpr -> int {
			return v * num_vertices + u;
		};

		auto set_edge = [&E, &index, &edge_list, &degV](int v, int u, unsigned int p) -> void {
			if (E[index (v, u)].is_edge) {
				return;
			}

			degV[v]++;
			degV[u]++;

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
		}

		// graph is constructed, now we can do the big scary algorithm
		std::set<surface_triangle> triangles;

		for (const auto & edge : edge_list) {
			auto u = edge.first, v = edge.second;

			for (int w = 0; w < V.size (); ++w) {
				if (w != u && w != v && u != v && 
					E[index(v, w)].is_edge && E[index(w, u)].is_edge && 
					(degV[v] < 4 || degV[u] < 4 || degV[w] < 4)) {
					triangles.insert(surface_triangle {u, v, w});
				}
			}
		}

		for (const auto & tri : triangles) {
			int p1 = E[index (tri.u, tri.v)].patch_id;
			int p2 = E[index (tri.u, tri.v)].patch_id;
			int p3 = E[index (tri.u, tri.v)].patch_id;

			m_gaps.push_back (surface_gap_t {p1, p2, p3});
		}
	}

	gap_filling_controller::~gap_filling_controller () { }

	void gap_filling_controller::create_surfaces () {
		
	}
}