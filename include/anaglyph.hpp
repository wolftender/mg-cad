#pragma once
#include "scamera.hpp"
#include "context.hpp"

namespace mini {
	class anaglyph_controller final {
		private:
			std::unique_ptr<anaglyph_camera> m_right_cam;
			std::unique_ptr<anaglyph_camera> m_left_cam;
			video_mode_t m_mode;
			bool m_anaglyph_enabled;

			GLuint m_buffer[3];
			GLuint m_texture[3];
			GLuint m_vao, m_posbuffer;

			float m_eyes_distance;
			float m_near, m_far;
			float m_gamma, m_cutoff;

			std::unique_ptr<shader_t> m_left_shader;
			std::unique_ptr<shader_t> m_right_shader;
			std::unique_ptr<shader_t> m_front_shader;

		public:
			const anaglyph_camera & get_right_cam () const;
			const anaglyph_camera & get_left_cam () const;

			GLuint get_left_buffer_handle () const;
			GLuint get_right_buffer_handle () const;
			GLuint get_buffer_handle () const;
			
			anaglyph_controller (const video_mode_t & mode);
			~anaglyph_controller ();

			anaglyph_controller (const anaglyph_controller &) = delete;
			anaglyph_controller & operator= (const anaglyph_controller &) = delete;

			const video_mode_t & get_video_mode () const;
			bool is_enabled () const;

			void set_video_mode (const video_mode_t & mode);
			void set_enabled (bool enabled);

			void render (app_context & context);
			void configure ();

		private:
			void m_adjust_camera ();
			void m_initialize_buffers ();
			void m_destroy_buffers ();
	};
}