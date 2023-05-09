#include "anaglyph.hpp"
#include "gui.hpp"

namespace mini {
	constexpr const std::string_view screen_vertex_source = R"(
		#version 330
		layout (location = 0) in vec3 a_position;

		out vec2 v_texcoords;

		void main () {
			gl_Position = vec4 (a_position.xyz, 1.0);
			v_texcoords = (1.0 + gl_Position.xy) / 2.0;
		}
	)";

	constexpr const std::string_view screen_fragment_source = R"(
		#version 330
		in vec2 v_texcoords;

		layout (location = 0) out vec4 v_color;

		uniform sampler2D u_texture_right;
		uniform sampler2D u_texture_left;

		void main () {
			vec4 frag_color = texture (u_texture_right, v_texcoords);
			v_color = frag_color;
		}
	)";

	constexpr const int anaglyph_left = 0;
	constexpr const int anaglyph_right = 1;
	constexpr const int anaglyph_front = 2;

	constexpr std::array<float, 18> screen_quad_positions {
		1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,

		1.0f, -1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f
	};

	const anaglyph_camera & anaglyph_controller::get_right_cam () const {
		return (*m_right_cam.get ());
	}

	const anaglyph_camera & anaglyph_controller::get_left_cam () const {
		return (*m_left_cam.get ());
	}

	GLuint anaglyph_controller::get_left_buffer_handle () const {
		return m_texture[anaglyph_left];
	}

	GLuint anaglyph_controller::get_right_buffer_handle () const {
		return m_texture[anaglyph_right];
	}

	GLuint anaglyph_controller::get_buffer_handle () const {
		return m_texture[anaglyph_front];
	}

	anaglyph_controller::anaglyph_controller (const video_mode_t & mode) {
		m_anaglyph_enabled = false;
		m_mode = mode;

		memset (m_buffer, 0, sizeof (m_buffer));
		memset (m_texture, 0, sizeof (m_buffer));

		m_eyes_distance = 0.5f;

		m_left_cam = std::make_unique<anaglyph_camera> ();
		m_right_cam = std::make_unique<anaglyph_camera> ();

		m_left_shader = std::make_unique<shader_t> (std::string (screen_vertex_source), std::string (screen_fragment_source));
		m_right_shader = std::make_unique<shader_t> (std::string (screen_vertex_source), std::string (screen_fragment_source));
		m_front_shader = std::make_unique<shader_t> (std::string (screen_vertex_source), std::string (screen_fragment_source));

		m_left_shader->compile ();
		m_right_shader->compile ();
		m_front_shader->compile ();

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

		// camera adjustments
		float width = static_cast<float> (mode.get_buffer_width ());
		float height = static_cast<float> (mode.get_buffer_height ());

		float aspect = width / height;
		float right = aspect - m_eyes_distance;
		float left = -aspect + m_eyes_distance;

		m_right_cam->set_right (right);
		m_right_cam->set_left (left);

		m_left_cam->set_left (-right);
		m_left_cam->set_right (-left);
	}

	void anaglyph_controller::set_enabled (bool enabled) {
		m_anaglyph_enabled = enabled;
	}

	void anaglyph_controller::render (app_context & context) {
		auto position = context.get_camera ().get_position ();
		auto target = context.get_camera ().get_target ();

		m_left_cam->set_position (position);
		m_right_cam->set_position (position);

		m_left_cam->set_target (target);
		m_right_cam->set_target (target);

		auto original_camera = context.set_camera (std::move (m_left_cam));
		glBindVertexArray (m_vao);
		
		// render with left eye
		auto left_cam = dynamic_cast<anaglyph_camera*> (context.set_camera (std::move (m_right_cam)).release ());
		context.display (false, false);

		glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[anaglyph_right]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, context.get_front_buffer ());

		m_right_shader->bind ();

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray (m_vao);
		glDrawArrays (GL_TRIANGLES, 0, 6);

		// render with right eye
		auto right_cam = dynamic_cast<anaglyph_camera*> (context.set_camera (std::move (original_camera)).release ());
		context.display (false, true);

		glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[anaglyph_left]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, context.get_front_buffer ());

		m_left_shader->bind ();

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray (m_vao);
		glDrawArrays (GL_TRIANGLES, 0, 6);

		m_left_cam.reset (left_cam);
		m_right_cam.reset (right_cam);

		// blend final image
		glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[anaglyph_front]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, m_texture[anaglyph_right]);
		//glActiveTexture (GL_TEXTURE1);
		//glBindTexture (GL_TEXTURE_2D, m_texture[anaglyph_left]);

		m_front_shader->bind ();

		glClearColor (1.0f, 0.0f, 0.0f, 1.0f);
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray (m_vao);
		glDrawArrays (GL_TRIANGLES, 0, 6);

		// set all to none
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, 0);
		glActiveTexture (GL_TEXTURE1);
		glBindTexture (GL_TEXTURE_2D, 0);
		glBindFramebuffer (GL_FRAMEBUFFER, 0);

		glBindVertexArray (0);
	}

	void anaglyph_controller::configure () {
		ImGui::NewLine ();

		if (ImGui::CollapsingHeader ("Anaglyph Options")) {
			gui::prefix_label ("Enabled: ", 100.0f);
			ImGui::Checkbox ("##anaglyphenable", &m_anaglyph_enabled);
		}
	}

	void anaglyph_controller::m_initialize_buffers () {
		int32_t render_width = m_mode.get_buffer_width ();
		int32_t render_height = m_mode.get_buffer_height ();

		const uint32_t position_location = 0;

		glGenTextures (3, m_texture);
		glGenFramebuffers (3, m_buffer);

		for (int i = 0; i <= anaglyph_front; ++i) {
			glBindTexture (GL_TEXTURE_2D, m_texture[i]);
			glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, render_width, render_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[i]);
			glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture[i], 0);

			if (glCheckFramebufferStatus (GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				throw std::runtime_error ("opengl error: framebuffer " + std::to_string (i) + " is not complete");
			}

			glBindTexture (GL_TEXTURE_2D, 0);
			glBindFramebuffer (GL_FRAMEBUFFER, 0);
		}

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_posbuffer);

		glBindVertexArray (m_vao);
		glBindBuffer (GL_ARRAY_BUFFER, m_posbuffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * screen_quad_positions.size (), screen_quad_positions.data (), GL_STATIC_DRAW);
		glVertexAttribPointer (position_location, 3, GL_FLOAT, false, 3 * sizeof (float), 0);
		glEnableVertexAttribArray (position_location);
		
		glBindVertexArray (0);
	}

	void anaglyph_controller::m_destroy_buffers () {
		glDeleteTextures (3, m_texture);
		glDeleteFramebuffers (3, m_buffer);
		glDeleteBuffers (1, &m_posbuffer);
		glDeleteVertexArrays (1, &m_vao);
	}
}