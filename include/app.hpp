#pragma once
#include <vector>

#include "window.hpp"
#include "context.hpp"
#include "grid.hpp"
#include "object.hpp"
#include "billboard.hpp"
#include "tool.hpp"

namespace mini {
	class application : public app_window {
		private:
			struct object_wrapper_t {
				std::shared_ptr<scene_obj_t> object;
				std::string name;
				bool selected;

				object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name);
			};

			app_context m_context;

			std::shared_ptr<shader_t> m_basic_shader, m_grid_xz_shader, m_grid_xy_shader;
			std::shared_ptr<shader_t> m_billboard_shader, m_billboard_shader_s;
			std::shared_ptr<shader_t> m_mesh_shader, m_alt_mesh_shader;
			std::vector<object_wrapper_t> m_objects;

			// gizmos etc
			std::shared_ptr<billboard_object> m_cursor_object;
			std::shared_ptr<grid_object> m_grid_xz, m_grid_xy;

			// textures
			std::shared_ptr<texture_t> m_test_texture;

			std::shared_ptr<scene_obj_t> m_selected_object;
			std::shared_ptr<tool_base> m_selected_tool;

			// cursor
			glm::vec3 m_cursor_position;

			float m_cam_yaw, m_cam_pitch, m_distance;
			float m_time;
			float m_grid_spacing;
			bool m_grid_enabled;

			int m_last_vp_width, m_last_vp_height;
			bool m_viewport_focus;

			glm::vec3 m_camera_target;
			offset_t m_vp_mouse_offset;

		public:
			// api for tools
			float get_cam_yaw () const;
			float get_cam_pitch () const;
			float get_cam_distance () const;
			float get_time () const;
			bool is_viewport_focused () const;

			const glm::vec3 & get_cursor_pos () const;
			const glm::vec2 & get_cursor_screen_pos () const;

			glm::vec3 get_mouse_direction () const;
			glm::vec3 get_screen_direction (float screen_x, float screen_y) const;
			glm::vec2 pixels_to_screen (const glm::vec2 & pos) const;
			glm::vec2 screen_to_pixels (const glm::vec2 & pos) const;
			
			void set_cursor_pos (const glm::vec3 & position);
			void set_cursor_screen_pos (const glm::vec2 & screen_pos);

			int get_viewport_width () const;
			int get_viewport_height () const;
			offset_t get_viewport_mouse_offset () const;

			const glm::vec3 & get_cam_target () const;
			void set_cam_target (const glm::vec3 & target);

			app_context & get_context ();
			std::shared_ptr<scene_obj_t> get_selection ();

			void set_cam_yaw (float yaw);
			void set_cam_pitch (float pitch);
			void set_cam_distance (float distance);

			application ();
			
			application (const application &) = delete;
			application & operator= (const application) = delete;

		protected:
			virtual void t_integrate (float delta_time) override;
			virtual void t_render () override;

			virtual void t_on_character (unsigned int code) override;
			virtual void t_on_cursor_pos (double posx, double posy) override;
			virtual void t_on_mouse_button (int button, int action, int mods) override;
			virtual void t_on_key_event (int key, int scancode, int action, int mods) override;
			virtual void t_on_scroll (double offset_x, double offset_y) override;
			virtual void t_on_resize (int width, int height) override;

		private:
			void m_handle_mouse ();
			void m_snap_cursor_to_mouse ();

			std::string m_read_file_content (const std::string & path) const;
			std::shared_ptr<shader_t> m_load_shader (const std::string & vs_file, const std::string & ps_file) const;

			// render for different ui elements
			void m_draw_main_menu ();
			void m_draw_main_window ();
			void m_draw_view_options ();
			void m_draw_scene_options ();
			void m_draw_object_options ();
			void m_draw_viewport ();

			// object management
			void m_add_object (const std::string & name, std::shared_ptr<scene_obj_t> object);
			void m_reset_selection ();
	};
}