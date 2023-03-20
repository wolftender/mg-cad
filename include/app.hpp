#pragma once
#include <vector>
#include <functional>

#include "window.hpp"
#include "context.hpp"
#include "grid.hpp"
#include "object.hpp"
#include "billboard.hpp"
#include "tool.hpp"
#include "factory.hpp"
#include "store.hpp"

namespace mini {
	class application : public app_window {
		private:
			struct object_wrapper_t {
				std::shared_ptr<scene_obj_t> object;
				std::string name, tmp_name;
				bool selected;

				object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name);
			};

			app_context m_context;

			std::shared_ptr<resource_store> m_store;
			std::shared_ptr<object_factory> m_factory;

			std::vector<std::shared_ptr<object_wrapper_t>> m_objects;

			// gizmos etc
			std::shared_ptr<billboard_object> m_cursor_object;
			std::shared_ptr<grid_object> m_grid_xz, m_grid_xy;

			// textures
			std::shared_ptr<texture_t> m_test_texture;

			std::shared_ptr<object_wrapper_t> m_selected_object;
			std::shared_ptr<tool_base> m_selected_tool;

			// cursor
			glm::vec3 m_cursor_position;

			float m_cam_yaw, m_cam_pitch, m_distance;
			float m_time;
			float m_grid_spacing;
			bool m_grid_enabled;

			// object creation tool
			bool m_show_creator;

			int m_last_vp_width, m_last_vp_height;
			bool m_viewport_focus, m_mouse_in_viewport;

			glm::vec3 m_camera_target;
			offset_t m_vp_mouse_offset;

			// object factories

		public:
			// api for tools
			float get_cam_yaw () const;
			float get_cam_pitch () const;
			float get_cam_distance () const;
			float get_time () const;
			bool is_viewport_focused () const;
			bool is_mouse_in_viewport () const;

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

			void m_show_object_creator (bool enable);

			// render for different ui elements
			void m_draw_main_menu ();
			void m_draw_main_window ();
			void m_draw_view_options ();
			void m_draw_scene_options ();
			void m_draw_object_options ();
			void m_draw_group_options ();
			void m_draw_object_creator ();
			void m_draw_viewport ();

			// object management
			std::string m_get_free_name (const std::string & name, const std::string & self = std::string ()) const;
			void m_add_object (const std::string & name, std::shared_ptr<scene_obj_t> object, bool select);
			void m_reset_selection ();
	};
}