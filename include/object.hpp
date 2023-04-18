#pragma once
#include <functional>
#include <array>
#include <set>
#include <list>

#include "context.hpp"
#include "algebra.hpp"
#include "event.hpp"
#include "serializer.hpp"

namespace mini {
	struct hit_test_data_t {
		const mini::camera & camera;
		glm::vec2 mouse_screen;
		glm::vec2 screen_res;
		glm::vec3 mouse_ray;
		bool valid;

		hit_test_data_t (const mini::camera & cam, const glm::vec2 & mouse_screen, 
			const glm::vec2 & screen_res, const glm::vec3 & mouse_ray);
	};

	class scene_obj_t;

	class scene_controller_base {
		public:
			class selected_object_collection {
				public:
					selected_object_collection () { }
					virtual ~selected_object_collection () { }

					virtual bool has () = 0;
					virtual bool next () = 0;
					virtual std::shared_ptr<scene_obj_t> get_object () = 0;
			};

			using selected_object_iter_ptr = std::unique_ptr<selected_object_collection>;

		public:
			virtual ~scene_controller_base () { };

			// virtual methods that a "scene" has
			virtual void add_object (const std::string & name, std::shared_ptr<scene_obj_t> object) = 0;
			virtual void set_cursor_pos (const glm::vec3 & position) = 0;

			virtual bool is_viewport_focused () const = 0;
			virtual bool is_mouse_in_viewport () const = 0;

			virtual glm::vec3 get_mouse_direction () const = 0;
			virtual glm::vec3 get_mouse_direction (int offset_x, int offset_y) const = 0;
			virtual glm::vec3 get_screen_direction (float screen_x, float screen_y) const = 0;
			virtual glm::vec2 pixels_to_screen (const glm::vec2 & pos) const = 0;
			virtual glm::vec2 screen_to_pixels (const glm::vec2 & pos) const = 0;
			virtual glm::vec2 world_to_screen (const glm::vec3 & world_pos) const = 0;

			virtual const glm::vec3 & get_cursor_pos () const = 0;
			virtual const glm::vec3 & get_cam_target () const = 0;

			virtual hit_test_data_t get_hit_test_data () const = 0;

			virtual const camera & get_camera () const = 0;
			virtual const video_mode_t & get_video_mode () const = 0;

			virtual selected_object_iter_ptr get_selected_objects () = 0;
	};

	class scene_obj_t : 
		public graphics_obj_t, public event_listener_base, public std::enable_shared_from_this<scene_obj_t> {
		public:
			enum class signal_event_t {
				moved		= 0,
				rotated		= 1,
				scaled		= 2,
				selected	= 3,
				renamed		= 4,
				MAX
			};

			using signal_handler_t = std::function<void(signal_event_t, scene_obj_t &)>;

		private:
			scene_controller_base & m_scene;
			std::string m_type_name;
			std::string m_name;

			glm::vec3 m_translation;
			glm::vec3 m_euler_angles;
			glm::vec3 m_scale;

			bool m_rotatable, m_movable, m_scalable;
			bool m_selected, m_disposed;
			bool m_mouse_lock;

			std::array<std::list<std::weak_ptr<scene_obj_t>>, static_cast<int> (signal_event_t::MAX)> m_listeners;
			std::array<signal_handler_t, static_cast<int> (signal_event_t::MAX) + 1> m_handlers;

		private:
			void m_notify (signal_event_t sig);
			void m_receive (signal_event_t sig, scene_obj_t & emitter);
			void m_listen (signal_event_t sig, std::shared_ptr<scene_obj_t> listener);
			void m_ignore (signal_event_t sig, std::shared_ptr<scene_obj_t> listener);

		protected:
			void t_listen (signal_event_t sig, scene_obj_t & target);
			void t_ignore (signal_event_t sig, scene_obj_t & target);
			void t_set_handler (signal_event_t sig, signal_handler_t handler);

			void t_set_mouse_lock (bool lock);

		public:
			const std::string & get_type_name () const;
			const std::string & get_name () const;

			const glm::vec3 & get_translation () const;
			const glm::vec3 & get_euler_angles () const;
			const glm::vec3 & get_scale () const;

			bool is_selected () const;
			bool is_rotatable () const;
			bool is_scalable () const;
			bool is_movable () const;
			bool is_disposed () const;

			bool is_mouse_lock () const;

			scene_controller_base & get_scene () const;

			void dispose ();

			void set_translation (const glm::vec3 & translation);
			void set_euler_angles (const glm::vec3 & euler_angles);
			void set_scale (const glm::vec3 & scale);
			void set_selected (bool selected);
			void set_name (const std::string & name);

			glm::mat4x4 compose_matrix (const glm::vec3 & translation, const glm::vec3 & euler_angles, const glm::vec3 & scale) const;
			glm::mat4x4 get_matrix () const;

			scene_obj_t (scene_controller_base & scene, const std::string & type_name, bool movable = true, bool rotatable = true, bool scalable = true);
			virtual ~scene_obj_t ();

			// notify methods
			void notify_object_created (std::shared_ptr<scene_obj_t> object);
			void notify_object_selected (std::shared_ptr<scene_obj_t> object);
			void notify_object_deleted (std::shared_ptr<scene_obj_t> object);

			// virtual methods
			virtual void integrate (float delta_time);
			virtual void configure ();
			virtual bool hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const;
			
			// object serialization
			virtual const object_serializer_base & get_serializer () const;

		protected:
			virtual void t_on_selection (bool select) { }
			virtual void t_on_object_created (std::shared_ptr<scene_obj_t> object) { }
			virtual void t_on_object_selected (std::shared_ptr<scene_obj_t> object) {}
			virtual void t_on_object_deleted (std::shared_ptr<scene_obj_t> object) { }
	};
}