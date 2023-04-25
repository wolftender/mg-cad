#include "interpolate.hpp"

namespace mini {
	interpolating_curve::interpolating_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2)
		: curve_base (scene, "interpolating c2") {

		m_shader1 = shader1;
		m_shader2 = shader2;

		m_vao_poly = 0;
		m_vao = 0;
		m_pos_buffer_poly = 0;
		m_pos_buffer = 0;
		m_ready = false;

		rebuild_curve ();
	}

	interpolating_curve::~interpolating_curve () { }

	void interpolating_curve::configure () {
		curve_base::configure ();
	}

	void interpolating_curve::integrate (float delta_time) {
		if (is_rebuild_queued ()) {
			rebuild_curve ();
		}
	}

	void interpolating_curve::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		if (m_ready) {
			glBindVertexArray (m_vao);
			m_bind_shader (context, m_shader1, world_matrix);

			m_shader1->set_uniform ("u_start_t", 0.0f);
			m_shader1->set_uniform ("u_end_t", 0.5f);
			glDrawArrays (GL_LINES_ADJACENCY, 0, static_cast<GLsizei>(m_bezier_buffer.size ()));

			m_shader1->set_uniform ("u_start_t", 0.5f);
			m_shader1->set_uniform ("u_end_t", 1.0f);
			glDrawArrays (GL_LINES_ADJACENCY, 0, static_cast<GLsizei>(m_bezier_buffer.size ()));

			if (is_show_polygon ()) {
				glBindVertexArray (m_vao_poly);
				m_bind_shader (context, m_shader2, world_matrix);
				glDrawArrays (GL_LINES, 0, static_cast<GLsizei> (m_bezier_buffer_poly.size ()));
			}

			glBindVertexArray (0);
		}
	}

	void interpolating_curve::t_rebuild_curve () {
		const auto & points = t_get_points ();

		m_bezier_buffer_poly.clear ();
		m_bezier_buffer.clear ();

		m_destroy_buffers ();
		m_ready = false;

		if (points.size () < 4) {
			return;
		}

		using spline_segment_t = std::array<glm::vec3, 4>;

		// at least four bspline points
		const int n = static_cast<int> (points.size () - 2);

		// n+2 points so n+1 segments
		std::vector<spline_segment_t> power (n + 1);
		std::vector<spline_segment_t> bernstein (n + 1);

		// fill all with zeroes
		for (int i = 0; i < n + 1; ++i) {
			for (int j = 0; j < 4; ++j) {
				power[i][j] = { 0.0f, 0.0f, 0.0f };
				bernstein[i][j] = { 0.0f, 0.0f, 0.0f };
			}
		}

		// interpolation will be solving the equation using thomas algorithm
		float_array_t a (n); // [0 .. n] = [1 .. N-1]
		float_array_t b (n);
		float_array_t m (n);
		float_array_t R (n); 
		float_array_t d (n + 1); // [0 .. n] = [0 .. N-2]

		std::vector<glm::vec3> P(n + 2); // [0 .. n+1] = [0 .. N-1]

		for (int i = 0; i < n + 2; ++i) {
			const auto ptr = points[i].point.lock ();
			if (!ptr) {
				return;
			}

			P[i] = ptr->get_translation ();
		}

		// calculate d coefficients
		for (int i = 0; i < n + 1; ++i) {
			//d[i] = glm::length (P[i + 1] - P[i]);
			d[i] = 1.0f;
		}

		// remember that now d_i from lecture is actually d[i+1] in the code!! :D
		// alphas and betas
		for (int i = 0; i < n - 1; ++i) {
			a[i + 1] = d[i + 1] / (d[i + 1] + d[i + 2]);
			b[i] = d[i + 1] / (d[i] + d[i + 1]);
		}

		// middle diagonal with just twos
		std::fill (m.begin (), m.end (), 2.0f);

		// the d coefficients are vector equation
		for (int dim = 0; dim < 3; ++dim) {
			for (int i = 0; i < n; ++i) {
				float num1 = (P[i + 2][dim] - P[i + 1][dim]) / d[i + 1];
				float num2 = (P[i + 1][dim] - P[i + 0][dim]) / d[i + 0];
				float den = d[i] + d[i + 1];
				R[i] = 3.0f * (num1 - num2) / den;
			}

			auto c = m_solve_tridiag (a, m, b, R);
			for (int i = 0; i < n; ++i) {
				power[i + 1][2][dim] = c[i];
			}
		}

		// calculate remaining power basis coordinates
		for (int i = 0; i < n + 1; ++i) {
			power[i][0] = P[i];
		}

		int i = 0;
		for (;i < n; ++i) {
			float di = d[i];
			power[i][3] = (power[i + 1][2] - power[i][2]) / (3.0f * di);
			power[i][1] = ((power[i+1][0] - power[i][0]) / di) - (power[i][2] * di) - (power[i][3] * di * di);
		}

		float di = d[i];
		power[i][3] = (0.0f - power[i][2]) / (3.0f * di);
		power[i][1] = ((P[i+1] - power[i][0]) / di) - (power[i][2] * di) - (power[i][3] * di * di);

		// convert to bernstein basis and we're done
		constexpr const glm::mat4x4 POWER_TO_BERNSTEIN = {
			1.0f, 1.0f, 1.0f, 1.0f, 
			0.0f, 1.0f / 3.0f, 2.0f / 3.0f, 1.0f, 
			0.0f, 0.0f, 1.0f / 3.0f, 1.0f, 
			0.0f, 0.0f, 0.0f, 1.0f
		};

		for (int i = 0; i < n + 1; ++i) {
			for (int dim = 0; dim < 3; ++dim) {
				glm::vec4 pow_w = { power[i][0][dim], power[i][1][dim], power[i][2][dim], power[i][3][dim] };
				glm::vec4 bern_w = POWER_TO_BERNSTEIN * pow_w;

				bernstein[i][0][dim] = bern_w[0];
				bernstein[i][1][dim] = bern_w[1];
				bernstein[i][2][dim] = bern_w[2];
				bernstein[i][3][dim] = bern_w[3];
			}
		}

		m_bezier_buffer.resize ((n + 1) * 12);
		m_bezier_buffer_poly.resize ((n + 1) * 18);

		for (int i = 0; i < n + 1; ++i) {
			m_bezier_buffer[i * 12 + 0] = bernstein[i][0].x;
			m_bezier_buffer[i * 12 + 1] = bernstein[i][0].y;
			m_bezier_buffer[i * 12 + 2] = bernstein[i][0].z;

			m_bezier_buffer[i * 12 + 3] = bernstein[i][1].x;
			m_bezier_buffer[i * 12 + 4] = bernstein[i][1].y;
			m_bezier_buffer[i * 12 + 5] = bernstein[i][1].z;

			m_bezier_buffer[i * 12 + 6] = bernstein[i][2].x;
			m_bezier_buffer[i * 12 + 7] = bernstein[i][2].y;
			m_bezier_buffer[i * 12 + 8] = bernstein[i][2].z;

			m_bezier_buffer[i * 12 + 9] = bernstein[i][3].x;
			m_bezier_buffer[i * 12 + 10] = bernstein[i][3].y;
			m_bezier_buffer[i * 12 + 11] = bernstein[i][3].z;

			// now for the polygon
			m_bezier_buffer_poly[i * 18 + 0] = bernstein[i][0].x;
			m_bezier_buffer_poly[i * 18 + 1] = bernstein[i][0].y;
			m_bezier_buffer_poly[i * 18 + 2] = bernstein[i][0].z;

			m_bezier_buffer_poly[i * 18 + 3] = bernstein[i][1].x;
			m_bezier_buffer_poly[i * 18 + 4] = bernstein[i][1].y;
			m_bezier_buffer_poly[i * 18 + 5] = bernstein[i][1].z;

			m_bezier_buffer_poly[i * 18 + 6] = bernstein[i][1].x;
			m_bezier_buffer_poly[i * 18 + 7] = bernstein[i][1].y;
			m_bezier_buffer_poly[i * 18 + 8] = bernstein[i][1].z;

			m_bezier_buffer_poly[i * 18 + 9] = bernstein[i][2].x;
			m_bezier_buffer_poly[i * 18 + 10] = bernstein[i][2].y;
			m_bezier_buffer_poly[i * 18 + 11] = bernstein[i][2].z;

			m_bezier_buffer_poly[i * 18 + 12] = bernstein[i][2].x;
			m_bezier_buffer_poly[i * 18 + 13] = bernstein[i][2].y;
			m_bezier_buffer_poly[i * 18 + 14] = bernstein[i][2].z;

			m_bezier_buffer_poly[i * 18 + 15] = bernstein[i][3].x;
			m_bezier_buffer_poly[i * 18 + 16] = bernstein[i][3].y;
			m_bezier_buffer_poly[i * 18 + 17] = bernstein[i][3].z;
		}

		m_init_buffers ();
	}

	interpolating_curve::float_array_t interpolating_curve::m_solve_tridiag (
		const float_array_t & a, 
		const float_array_t & b, 
		const float_array_t & c,
		const float_array_t & d) {

		const int n = static_cast<int> (b.size ());
		float_array_t cp (n);
		float_array_t dp (n);

		cp[0] = c[0] / b[0];
		dp[0] = d[0] / b[0];

		for (int i = 1; i < n - 1; ++i) {
			cp[i] = c[i] / (b[i] - a[i] * cp[i - 1]);
		}

		for (int i = 1; i < n; ++i) {
			dp[i] = (d[i] - a[i] * dp[i - 1]) / (b[i] - a[i] * cp[i - 1]);
		}

		float_array_t x (n);

		x [n - 1] = dp[n - 1];
		for (int i = n - 2; i >= 0; --i) {
			x[i] = dp[i] - cp[i] * x[i + 1];
		}

		return x;
	}

	void interpolating_curve::m_init_buffers () {
		constexpr GLuint a_position = 0;

		glGenVertexArrays (1, &m_vao);
		glGenBuffers (1, &m_pos_buffer);

		glBindVertexArray (m_vao);

		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_bezier_buffer.size (), reinterpret_cast<void *> (m_bezier_buffer.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glGenVertexArrays (1, &m_vao_poly);
		glBindVertexArray (m_vao_poly);

		glGenBuffers (1, &m_pos_buffer_poly);
		glBindBuffer (GL_ARRAY_BUFFER, m_pos_buffer_poly);
		glBufferData (GL_ARRAY_BUFFER, sizeof (float) * m_bezier_buffer_poly.size (), reinterpret_cast<void *> (m_bezier_buffer_poly.data ()), GL_DYNAMIC_DRAW);
		glVertexAttribPointer (a_position, 3, GL_FLOAT, false, sizeof (float) * 3, (void *)0);
		glEnableVertexAttribArray (a_position);

		glBindVertexArray (static_cast<GLuint> (NULL));

		m_ready = true;
	}

	void interpolating_curve::m_destroy_buffers () {
		if (m_pos_buffer_poly != 0) {
			glDeleteVertexArrays (1, &m_pos_buffer_poly);
		}

		if (m_pos_buffer != 0) {
			glDeleteVertexArrays (1, &m_pos_buffer);
		}

		if (m_vao_poly != 0) {
			glDeleteVertexArrays (1, &m_vao_poly);
		}

		if (m_vao != 0) {
			glDeleteVertexArrays (1, &m_vao);
		}

		m_vao_poly = 0;
		m_vao = 0;
		m_pos_buffer_poly = 0;
		m_pos_buffer = 0;

		m_ready = false;
	}

	void interpolating_curve::m_bind_shader (app_context & context, std::shared_ptr<shader_t> shader, const glm::mat4x4 & world_matrix) const {
		shader->bind ();

		const auto & view_matrix = context.get_view_matrix ();
		const auto & proj_matrix = context.get_projection_matrix ();

		const auto & video_mode = context.get_video_mode ();

		glm::vec2 resolution = {
			static_cast<float> (video_mode.get_buffer_width ()),
			static_cast<float> (video_mode.get_buffer_height ())
		};

		shader->set_uniform ("u_world", world_matrix);
		shader->set_uniform ("u_view", view_matrix);
		shader->set_uniform ("u_projection", proj_matrix);
		shader->set_uniform ("u_resolution", resolution);
		shader->set_uniform ("u_line_width", 2.0f);
	}
}
