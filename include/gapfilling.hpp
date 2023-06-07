#pragma once
#include "object.hpp"
#include "bezier.hpp"
#include "beziersurf.hpp"
#include "gregory.hpp"

namespace mini {
	class gap_filling_controller final {
		private:
			using surface_ptr = std::shared_ptr<bezier_surface_c0>;

			struct surface_gap_t {
				int patch1, patch2, patch3;
				uint64_t point1, point2, point3;

				surface_gap_t (int patch1, int patch2, int patch3, uint64_t point1, uint64_t point2, uint64_t point3) :
					patch1 (patch1), patch2 (patch2), patch3 (patch3), point1 (point1), point2 (point2), point3 (point3) { }
			};

			std::vector<bicubic_surface::surface_patch> m_patches;
			std::vector<surface_gap_t> m_gaps;

			scene_controller_base & m_scene;
			std::shared_ptr<resource_store> m_store;

		public:
			gap_filling_controller (scene_controller_base & scene, std::shared_ptr<resource_store> store);
			~gap_filling_controller ();

			gap_filling_controller (const gap_filling_controller &) = delete;
			gap_filling_controller& operator= (const gap_filling_controller &) = delete;

			void create_surfaces ();

		private:
			void m_get_offsets (bicubic_surface::surface_patch & patch, uint64_t p1, uint64_t p2, patch_offset_t & start, patch_offset_t & end);
			void m_spawn_debug_curve (bicubic_surface::surface_patch & patch, const patch_offset_t & start, const patch_offset_t & end);
	};
}