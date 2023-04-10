#pragma once

namespace mini {
	class event_listener_base {
		public:
			event_listener_base () = default;
			virtual ~event_listener_base () = default;

			virtual bool on_key_event (int key, int scancode, int action, int mods) { return false; }
			virtual bool on_character (unsigned int code) { return false; }
			virtual bool on_cursor_pos (double posx, double posy) { return false; }
			virtual bool on_mouse_button (int button, int action, int mods) { return false; }
			virtual bool on_scroll (double offset_x, double offset_y) { return false; }
			virtual bool on_update (float delta_time) { return false; }
	};
}