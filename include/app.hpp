#pragma once
#include <vector>
#include <functional>
#include <list>

#include "window.hpp"
#include "context.hpp"
#include "grid.hpp"
#include "object.hpp"
#include "billboard.hpp"
#include "tool.hpp"
#include "factory.hpp"
#include "store.hpp"

namespace mini {
	class application : public app_window, public scene_controller_base {
		private:
			struct object_wrapper_t {
				std::shared_ptr<scene_obj_t> object;
				std::string name, tmp_name;
				bool selected;
				bool destroy;

				object_wrapper_t (std::shared_ptr<scene_obj_t> o, const std::string & name);
			};

			// the way that group transforms are implemented is as follows:
			//  1) multiple objects can be selected
			//  2) if 1 object is selected then simply this object is the selection
			//  3) if more objects are selected the selection is a "fake" object that can only be transformed
			class group_logic_object : public scene_obj_t {
				private:
					struct grouped_object_wrapper {
						std::shared_ptr<object_wrapper_t> ptr;
						glm::mat4x4 os_transform;

						grouped_object_wrapper (std::shared_ptr<object_wrapper_t> _ptr);
					};

				public:
					class object_collection : public selected_object_collection {
						private:
							std::list<grouped_object_wrapper> & m_list;
							std::list<grouped_object_wrapper>::iterator m_iter;

						public:
							object_collection (std::list<grouped_object_wrapper> & list);

							virtual bool next () override;
							virtual bool has () override;
							virtual std::shared_ptr<scene_obj_t> get_object () override;
					};

				private:
					std::list<grouped_object_wrapper> m_group;
					glm::vec3 m_origin;

				public:
					group_logic_object (scene_controller_base & scene);
					~group_logic_object () = default;

					group_logic_object (const group_logic_object &) = delete;
					group_logic_object& operator= (const group_logic_object &) = delete;

					void group_add (std::shared_ptr<object_wrapper_t> object);
					void group_remove (std::shared_ptr<object_wrapper_t> object);
					void group_clear ();
					void group_destroy_all ();

					selected_object_iter_ptr get_iterator ();

					uint32_t group_size () const;

					glm::vec3 get_origin () const;
					std::shared_ptr<object_wrapper_t> group_pop ();

					void update ();

					virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
					virtual void configure () override;

				private:
					void m_reset_group_transforms ();
					void m_apply_all ();
					void m_calc_origin ();
					void m_apply_group_transform (grouped_object_wrapper & item, const glm::mat4x4 & group_transform);
			};

			app_context m_context;

			std::shared_ptr<resource_store> m_store;
			std::shared_ptr<object_factory> m_factory;

			std::vector<std::shared_ptr<object_wrapper_t>> m_objects;

			// gizmos etc
			std::shared_ptr<billboard_object> m_cursor_object, m_origin_object;
			std::shared_ptr<grid_object> m_grid_xz, m_grid_xy;

			// textures
			std::shared_ptr<texture_t> m_test_texture;

			std::shared_ptr<object_wrapper_t> m_selected_object;
			std::shared_ptr<tool_base> m_selected_tool;

			// group select
			std::shared_ptr<group_logic_object> m_selected_group;

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

		public:
			// api for tools
			float get_cam_yaw () const;
			float get_cam_pitch () const;
			float get_cam_distance () const;
			float get_time () const;
			virtual bool is_viewport_focused () const override;
			virtual bool is_mouse_in_viewport () const override;

			virtual void add_object (const std::string & name, std::shared_ptr<scene_obj_t> object) override;

			virtual const glm::vec3 & get_cursor_pos () const override;
			glm::vec2 get_cursor_screen_pos () const;

			virtual hit_test_data_t get_hit_test_data () const override;

			virtual glm::vec3 get_mouse_direction () const override;
			virtual glm::vec3 get_mouse_direction (int offset_x, int offset_y) const override;
			virtual glm::vec3 get_screen_direction (float screen_x, float screen_y) const override;
			virtual glm::vec2 pixels_to_screen (const glm::vec2 & pos) const override;
			virtual glm::vec2 screen_to_pixels (const glm::vec2 & pos) const override;
			virtual glm::vec2 world_to_screen (const glm::vec3 & world_pos) const override;

			virtual selected_object_iter_ptr get_selected_objects () override;
			
			virtual void set_cursor_pos (const glm::vec3 & position) override;
			void set_cursor_screen_pos (const glm::vec2 & screen_pos);

			int get_viewport_width () const;
			int get_viewport_height () const;
			offset_t get_viewport_mouse_offset () const;

			virtual const glm::vec3 & get_cam_target () const override;
			void set_cam_target (const glm::vec3 & target);

			virtual const camera & get_camera () const override;
			virtual const video_mode_t & get_video_mode () const override;

			app_context & get_context ();
			std::shared_ptr<scene_obj_t> get_selection ();
			std::shared_ptr<scene_obj_t> get_group_selection ();

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
			void m_handle_mouse_select ();
			void m_handle_mouse ();
			void m_snap_cursor_to_mouse ();
			void m_destroy_object ();

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

			// selection methods
			void m_mark_object (std::shared_ptr<object_wrapper_t> object_wrapper);
			void m_select_object (std::shared_ptr<object_wrapper_t> object_wrapper);
			void m_group_select_add (std::shared_ptr<object_wrapper_t> object_wrapper);
			void m_reset_selection ();

};
}