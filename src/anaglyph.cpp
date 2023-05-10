#include "anaglyph.hpp"
#include "gui.hpp"

namespace mini {
	constexpr const std::string_view camera_vertex_source = R"(
		#version 330
		layout (location = 0) in vec3 a_position;

		out vec2 v_texcoords;

		void main () {
			gl_Position = vec4 (a_position.xyz, 1.0);
			v_texcoords = (1.0 + gl_Position.xy) / 2.0;
		}
	)";

	constexpr const std::string_view eye_fragment_source = R"(
		#version 330
		in vec2 v_texcoords;

		layout (location = 0) out vec4 v_color;

		uniform float u_gamma;
		uniform float u_cutoff;
		uniform sampler2D u_texture;

		void main () {
			vec4 frag_color = texture (u_texture, v_texcoords);
			//float gs = 0.299 * frag_color.x + 0.587 * frag_color.y + 0.114 * frag_color.z;

			//gs = pow(gs, u_gamma);
			//v_color = vec4 (gs, gs, gs, 1.0);
			v_color = frag_color;
			v_color.x = pow(v_color.x, u_gamma);
			v_color.y = pow(v_color.y, u_gamma);
			v_color.z = pow(v_color.z, u_gamma);
		}
	)";

	constexpr const std::string_view front_fragment_source = R"(
		#version 330
		in vec2 v_texcoords;

		layout (location = 0) out vec4 v_color;

		uniform sampler2D u_left_texture;
		uniform sampler2D u_right_texture;

		void main () {
			vec4 left_color = texture (u_left_texture, v_texcoords);
			vec4 right_color = texture (u_right_texture, v_texcoords);

			float Lr = left_color.x;
			float Lg = left_color.y;
			float Lb = left_color.z;

			float Rr = right_color.x;
			float Rg = right_color.y;
			float Rb = right_color.z;

			float Ar = 0.4561 * Lr + 0.500484 * Lg + 0.176381 * Lb - 0.0434706 * Rr - 0.0879388 * Rg - 0.00155529 * Rb;
			float Ag = -0.0400822 * Lr - 0.0378246 * Lg - 0.0157589 * Lb + 0.378476 * Rr + 0.73364 * Rg - 0.0184503 * Rb;
			float Ab = -0.0152161 * Lr - 0.0205971 * Lg - 0.00546856 * Lb - 0.0721527 * Rr - 0.112961 * Rg + 1.2264 * Rb;

			v_color = vec4 (Ar, Ag, Ab, 1.0);
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

		m_eyes_distance = 0.15f;

		m_left_cam = std::make_unique<anaglyph_camera> ();
		m_right_cam = std::make_unique<anaglyph_camera> ();

		m_left_shader = std::make_unique<shader_t> (std::string (camera_vertex_source), std::string (eye_fragment_source));
		m_right_shader = std::make_unique<shader_t> (std::string (camera_vertex_source), std::string (eye_fragment_source));
		m_front_shader = std::make_unique<shader_t> (std::string (camera_vertex_source), std::string (front_fragment_source));

		m_left_shader->compile ();
		m_right_shader->compile ();
		m_front_shader->compile ();
		
		m_near = 0.1f;
		m_far = 100.0f;

		m_gamma = 1.41f;
		m_cutoff = 0.25f;

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

		m_adjust_camera ();
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
		context.display (false, false);

		glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[anaglyph_right]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, context.get_front_buffer ());

		m_right_shader->bind ();
		m_right_shader->set_uniform ("u_gamma", m_gamma);
		m_right_shader->set_uniform ("u_cutoff", m_cutoff);

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray (m_vao);
		glDrawArrays (GL_TRIANGLES, 0, 6);

		// render with right eye
		auto left_cam = dynamic_cast<anaglyph_camera *> (context.set_camera (std::move (m_right_cam)).release ());
		context.display (false, true);

		glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[anaglyph_left]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, context.get_front_buffer ());

		m_left_shader->bind ();
		m_left_shader->set_uniform ("u_gamma", m_gamma);
		m_left_shader->set_uniform ("u_cutoff", m_cutoff);

		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray (m_vao);
		glDrawArrays (GL_TRIANGLES, 0, 6);

		auto right_cam = dynamic_cast<anaglyph_camera *> (context.set_camera (std::move (original_camera)).release ());
		m_left_cam.reset (left_cam);
		m_right_cam.reset (right_cam);

		// blend final image
		glBindFramebuffer (GL_FRAMEBUFFER, m_buffer[anaglyph_front]);
		glActiveTexture (GL_TEXTURE0);
		glBindTexture (GL_TEXTURE_2D, m_texture[anaglyph_right]);
		glActiveTexture (GL_TEXTURE1);
		glBindTexture (GL_TEXTURE_2D, m_texture[anaglyph_left]);

		m_front_shader->bind ();
		m_front_shader->set_uniform_sampler ("u_right_texture", 0U);
		m_front_shader->set_uniform_sampler ("u_left_texture", 1U);

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
		bool adjust_cam = false;

		if (ImGui::CollapsingHeader ("Anaglyph Options")) {
			gui::prefix_label ("Enabled: ", 100.0f);
			ImGui::Checkbox ("##anaglyphenable", &m_anaglyph_enabled);
			
			gui::prefix_label ("Near Z: ", 100.0f);
			if (ImGui::InputFloat ("##nearz", &m_near)) {
				adjust_cam = true;
			}

			gui::prefix_label ("Far Z: ", 100.0f);
			if (ImGui::InputFloat ("##farz", &m_far)) {
				adjust_cam = true;
			}

			gui::prefix_label ("Eye Dist.: ", 100.0f);
			if (ImGui::InputFloat ("##eyedist", &m_eyes_distance)) {
				adjust_cam = true;
			}

			gui::prefix_label ("Gamma: ", 100.0f);
			ImGui::InputFloat ("##gamma", &m_gamma);

			gui::prefix_label ("Cutoff: ", 100.0f);
			ImGui::InputFloat ("##cutoff", &m_cutoff);
		}

		if (adjust_cam) {
			m_adjust_camera ();
		}
	}

	void anaglyph_controller::m_adjust_camera () {
		// camera adjustments
		constexpr const float fovy = glm::pi<float> () / 3.0f;

		float width = static_cast<float> (m_mode.get_buffer_width ());
		float height = static_cast<float> (m_mode.get_buffer_height ());

		float aspect = width / height;
		float h = m_near * glm::tan (fovy / 2.0f);
		float w = aspect * h;

		float right = w * (1.0f - m_eyes_distance);
		float left = -w * (1.0f + m_eyes_distance);
		float top = h;
		float bottom = -h;

		m_right_cam->set_right (right);
		m_right_cam->set_left (left);
		m_right_cam->set_top (top);
		m_right_cam->set_bottom (bottom);
		m_right_cam->set_near (m_near);
		m_right_cam->set_far (m_far);

		m_left_cam->set_left (-right);
		m_left_cam->set_right (-left);
		m_left_cam->set_top (top);
		m_left_cam->set_bottom (bottom);
		m_left_cam->set_near (m_near);
		m_left_cam->set_far (m_far);
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