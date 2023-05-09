#include "anaglyph.hpp"
#include "gui.hpp"

namespace mini {
	const anaglyph_camera & anaglyph_controller::get_right_cam () const {
		return (*m_right_cam.get ());
	}

	const anaglyph_camera & anaglyph_controller::get_left_cam () const {
		return (*m_left_cam.get ());
	}

	GLuint anaglyph_controller::get_left_buffer_handle () const {
		return m_left_buffer;
	}

	GLuint anaglyph_controller::get_right_buffer_handle () const {
		return m_right_buffer;
	}

	GLuint anaglyph_controller::get_buffer_handle () const {
		return m_buffer;
	}

	anaglyph_controller::anaglyph_controller (const video_mode_t & mode) {
		m_anaglyph_enabled = false;
		m_mode = mode;

		m_left_buffer = 0;
		m_right_buffer = 0;
		m_buffer = 0;

		m_initialize_buffers ();
	}

	anaglyph_controller::~anaglyph_controller () {
		m_destroy_buffers ();
	}

	const video_mode_t & anaglyph_controller::get_video_mode () const {
		return m_mode;
	}

	bool anaglyph_controller::is_enabled () const {
		return m_anaglyph_enabled;
	}

	void anaglyph_controller::set_video_mode (const video_mode_t & mode) {
		m_mode = mode;

		m_destroy_buffers ();
		m_initialize_buffers ();
	}

	void anaglyph_controller::set_enabled (bool enabled) {
		m_anaglyph_enabled = enabled;
	}

	void anaglyph_controller::render (app_context & context) {
		auto original_camera = context.set_camera (std::move (m_left_cam));
		
		// todo: render with left eye

		auto left_cam = dynamic_cast<anaglyph_camera*> (context.set_camera (std::move (m_right_cam)).release ());

		// todo: render with right eye

		auto right_cam = dynamic_cast<anaglyph_camera*> (context.set_camera (std::move (original_camera)).release ());

		m_left_cam.reset (left_cam);
		m_right_cam.reset (right_cam);

		// todo: blend final image
	}

	void anaglyph_controller::configure () {
	}

	void anaglyph_controller::m_initialize_buffers () {
	}

	void anaglyph_controller::m_destroy_buffers () {
		
	}
}