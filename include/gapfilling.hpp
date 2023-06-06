#pragma once
#include "object.hpp"
#include "bezier.hpp"
#include "beziersurf.hpp"

namespace mini {
	class gap_filling_controller final {
		private:
			using surface_ptr = std::shared_ptr<bezier_surface_c0>;

			struct surface_gap_t {
				int pid1, pid2, pid3;

				surface_gap_t (uint64_t pid1, uint64_t pid2, uint64_t pid3) :
					pid1(pid1), pid2(pid2), pid3(pid3) { }
			};

			std::vector<bicubic_surface::surface_patch> m_patches;
			std::vector<surface_gap_t> m_gaps;

			scene_controller_base & m_scene;

		public:
			gap_filling_controller (scene_controller_base & scene);
			~gap_filling_controller ();

			gap_filling_controller (const gap_filling_controller &) = delete;
			gap_filling_controller& operator= (const gap_filling_controller &) = delete;

			void create_surfaces ();
	};
}