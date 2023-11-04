#pragma once
#include "surface.hpp"
#include "beziersurf.hpp"
#include "bsplinesurf.hpp"

namespace mini {
	class application;

	class padlock_milling_script final {
		private:
			std::shared_ptr<bezier_surface_c0> m_padlock_keyhole;
			std::shared_ptr<bspline_surface> m_padlock_body;
			std::shared_ptr<bspline_surface> m_padlock_shackle;
			std::shared_ptr<bezier_surface_c0> m_model_base;

			application& m_app;

		public:
			padlock_milling_script(application& app);
			~padlock_milling_script();

			padlock_milling_script(const padlock_milling_script&) = delete;
			padlock_milling_script& operator=(const padlock_milling_script&) = delete;

			void run();

		private:
			void m_prepare_base();
			void m_identify_model();
	};
}