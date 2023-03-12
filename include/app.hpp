#pragma once
#include "../include/window.hpp"
#include "../include/context.hpp"
#include "../include/cube.hpp"
#include "../include/grid.hpp"
#include "../include/object.hpp"

namespace mini {
	class application : public app_window {
		private:
			app_context m_context;

			std::shared_ptr<shader_t> m_basic_shader, m_grid_shader;
			std::shared_ptr<cube_object> m_test_cube;
			std::shared_ptr<grid_object> m_grid;

			float m_cam_yaw, m_cam_pitch, m_distance;
			float m_time;
			float m_grid_spacing;
			bool m_grid_enabled;

			int m_last_vp_width, m_last_vp_height;
			bool m_viewport_focus;

		public:
			application ();
			
			application (const application &) = delete;
			application & operator= (const application) = delete;

		protected:
			virtual void t_integrate (float delta_time) override;
			virtual void t_render () override;

			/*virtual void t_on_key_event (int key, int scancode, int action, int mods) override;
			virtual void t_on_character (unsigned int code) override;
			virtual void t_on_cursor_pos (double posx, double posy) override;
			virtual void t_on_mouse_button (int button, int action, int mods) override;*/

			virtual void t_on_scroll (double offset_x, double offset_y) override;
			virtual void t_on_resize (int width, int height) override;

		private:
			std::string m_read_file_content (const std::string & path) const;
			std::shared_ptr<shader_t> m_load_shader (const std::string & vs_file, const std::string & ps_file) const;

			// render for different ui elements
			void m_draw_main_menu ();
			void m_draw_main_window ();
			void m_draw_view_options ();
			void m_draw_scene_options ();
			void m_draw_viewport ();
	};
}