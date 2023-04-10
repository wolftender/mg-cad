#pragma once
#include "context.hpp"
#include "object.hpp"
#include "event.hpp"

#include <string>
#include <GLFW/glfw3.h>

namespace mini {
	constexpr const int KEY_CANCEL = GLFW_KEY_ESCAPE;
	constexpr const int KEY_TRANSLATE = GLFW_KEY_T;
	constexpr const int KEY_ROTATE = GLFW_KEY_R;
	constexpr const int KEY_SCALE = GLFW_KEY_S;
	constexpr const int KEY_AXIS_X = GLFW_KEY_X;
	constexpr const int KEY_AXIS_Y = GLFW_KEY_Y;
	constexpr const int KEY_AXIS_Z = GLFW_KEY_Z;

	enum class axis_t {
		none, x, y, z
	};

	class application;

	class tool_base : public event_listener_base {
		private:
			std::string m_name;
			application & m_app;
			bool m_disposable;

		protected:
			void t_dispose ();

		public:
			const std::string & get_name () const;
			application & get_app () const;
			bool is_disposable () const;

			tool_base (application & app, const std::string & name);
			virtual ~tool_base ();

			tool_base (const tool_base &) = delete;
			tool_base & operator= (const tool_base &) = delete;

			virtual bool on_key_event (int key, int scancode, int action, int mods) override;

		public:
			glm::vec3 calculate_mouse_dir () const;
			glm::vec3 calculate_mouse_dir (int offset_x, int offset_y) const;
	};

	class camera_pan_tool : public tool_base {
		private:
			glm::vec3 m_original_target, m_original_hit, m_original_normal;
			
		public:
			camera_pan_tool (application & app);
			~camera_pan_tool ();

			camera_pan_tool (const camera_pan_tool &) = delete;
			camera_pan_tool & operator= (const camera_pan_tool &) = delete;

			virtual bool on_mouse_button (int button, int action, int mods) override;
			virtual bool on_update (float delta_time) override;
	};

	class translation_tool : public tool_base {
		private:
			glm::vec3 m_original_transform;
			int m_offset_x, m_offset_y;

			std::shared_ptr<scene_obj_t> m_selection;

			axis_t m_axis_lock;
			bool m_apply;

		public:
			translation_tool (application & app);
			~translation_tool ();

			translation_tool (const translation_tool &) = delete;
			translation_tool & operator= (const translation_tool &) = delete;

			virtual bool on_key_event (int key, int scancode, int action, int mods) override;
			virtual bool on_mouse_button (int button, int action, int mods) override;
			virtual bool on_update (float delta_time) override;
	};

	class rotation_tool : public tool_base {
		private:
			glm::vec3 m_original_rotation;
			std::shared_ptr<scene_obj_t> m_selection;

			axis_t m_axis_lock;
			bool m_apply;

		public:
			rotation_tool (application & app);
			~rotation_tool ();

			rotation_tool (const rotation_tool &) = delete;
			rotation_tool & operator= (const rotation_tool &) = delete;

			virtual bool on_key_event (int key, int scancode, int action, int mods) override;
			virtual bool on_mouse_button (int button, int action, int mods) override;
			virtual bool on_update (float delta_time) override;
	};

	class scale_tool : public tool_base {
		private:
			glm::vec3 m_original_transform;
			float m_start_x, m_start_y, m_object_x, m_object_y;

			std::shared_ptr<scene_obj_t> m_selection;

			axis_t m_axis_lock;
			bool m_apply;

		public:
			scale_tool (application & app);
			~scale_tool ();

			scale_tool (const scale_tool &) = delete;
			scale_tool & operator= (const scale_tool &) = delete;

			virtual bool on_key_event (int key, int scancode, int action, int mods) override;
			virtual bool on_mouse_button (int button, int action, int mods) override;
			virtual bool on_update (float delta_time) override;
	};
}