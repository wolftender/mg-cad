#pragma once
#include "context.hpp"
#include "object.hpp"

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

	class tool_base {
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

			// virtual methods
			virtual bool on_key_event (int key, int scancode, int action, int mods);
			virtual bool on_character (unsigned int code);
			virtual bool on_cursor_pos (double posx, double posy);
			virtual bool on_mouse_button (int button, int action, int mods);
			virtual bool on_scroll (double offset_x, double offset_y);
			virtual bool on_update (float delta_time);
	};

	class translation_tool : public tool_base {
		private:
			float_vector_t m_original_transform;
			std::shared_ptr<scene_obj_t> m_selection;

			axis_t m_axis_lock;
			bool m_apply;

		public:
			translation_tool (application & app);
			~translation_tool ();

			translation_tool (translation_tool &) = delete;
			translation_tool & operator= (translation_tool &) = delete;

			virtual bool on_key_event (int key, int scancode, int action, int mods) override;
			virtual bool on_mouse_button (int button, int action, int mods) override;
			virtual bool on_update (float delta_time) override;
	};
}