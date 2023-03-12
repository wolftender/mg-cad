#pragma once
#include <glad/glad.h>
#include <memory>

#include "algebra.hpp"
#include "shader.hpp"
#include "camera.hpp"

namespace mini {
	class app_context;

	constexpr uint32_t back = 0;
	constexpr uint32_t front = 1;

	const static std::array<float, 12> quad_vertices = {
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
	};

	const static std::array<float, 8> quad_texcoords = {
		1.0, 1.0,
		1.0, 0.0,
		0.0, 0.0,
		0.0, 1.0
	};

	const static std::array<GLuint, 6> quad_indices = {
		0, 1, 3,
		1, 2, 3
	};

	/// <summary>
	/// This interface represents a visible object on the scene.
	/// </summary>
	class graphics_obj_t {
		public:
			virtual ~graphics_obj_t () { }
			virtual void render (app_context & context, const float_matrix_t & world_matrix) const = 0;
	};

	/// <summary>
	/// This class represents the video mode of the context. Add context settings here.
	/// </summary>
	class video_mode_t {
		private:
			int32_t m_viewport_width, m_viewport_height;
			int32_t m_buffer_width, m_buffer_height;

		public:
			int32_t get_viewport_width () const;
			int32_t get_viewport_height () const;
			int32_t get_buffer_width () const;
			int32_t get_buffer_height () const;

			void set_viewport_width (int32_t vp_width);
			void set_viewport_height (int32_t vp_height);
			void set_buffer_width (int32_t buf_width);
			void set_buffer_height (int32_t buf_height);

			video_mode_t ();
			video_mode_t (int32_t width, int32_t height);
			video_mode_t (int32_t vp_width, int32_t vp_height, int32_t buf_width, int32_t buf_height);

			video_mode_t (const video_mode_t &) = default;
			video_mode_t & operator= (const video_mode_t &) = default;
			~video_mode_t () = default;
	};

	constexpr const uint64_t RENDER_QUEUE_SIZE = 1024;

	/// <summary>
	/// This class represents the graphics context. Because the application is
	/// object oriented and opengl is procedural, we want an object oriented wrapper.
	/// </summary>
	class app_context final {
		private:
			struct enqueued_renderable_t {
				std::weak_ptr<graphics_obj_t> object;
				float_matrix_t world_matrix;
			};

			std::array<enqueued_renderable_t, RENDER_QUEUE_SIZE> m_queue;
			uint64_t m_last_queue_index;

			float_vector_t m_camera_pos;
			float_vector_t m_camera_target;

			float_matrix_t m_view_matrix;
			float_matrix_t m_projection_matrix;

			// opengl framebuffer objects
			GLuint m_framebuffer[2], m_colorbuffer[2];
			GLuint m_renderbuffer;

			// screen quad
			GLuint m_quad_vao, m_quad_buffer[3];
			std::unique_ptr<shader_t> m_screen_shader;

			video_mode_t m_video_mode, m_new_mode;
			bool m_switch_mode;

			camera m_camera;

		public:
			app_context (const video_mode_t & video_mode);
			~app_context ();

			app_context (const app_context &) = delete;
			app_context & operator= (const app_context &) = delete;

			void set_camera_pos (const float_vector_t & position);
			void set_camera_target (const float_vector_t & target);

			void set_video_mode (const video_mode_t & video_mode);

			const video_mode_t & get_video_mode () const;
			const GLuint get_front_buffer () const;

			camera & get_camera ();
			const camera & get_camera () const;

			const float_matrix_t & get_view_matrix () const;
			const float_matrix_t & get_projection_matrix () const;

			void draw (std::weak_ptr<graphics_obj_t> object, float_matrix_t world_matrix);
			void render ();
			void display (bool present);
			void display_scene ();

		private:
			void m_try_switch_mode ();

			void m_init_frame_buffer ();
			void m_init_screen_quad ();

			void m_destroy_frame_buffer ();
			void m_destroy_screen_quad ();
	};
}