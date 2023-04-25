#include "interpolate.hpp"

namespace mini {
	interpolating_curve::interpolating_curve (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2)
		: curve_base (scene, "interpolating c2") {

		m_shader1 = shader1;
		m_shader2 = shader2;

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
		
	}

	void interpolating_curve::t_rebuild_curve () {
		/*float_array_t w1 = {0, 1, 2, 3, 4};
		float_array_t w2 = { 10, 20, 30, 40, 50 };
		float_array_t w3 = { 33, 44, 55, 66, 0 };
		float_array_t w4 = { 77, 88, 99, 111, 222 };
		auto w5 = m_solve_tridiag (w1, w2, w3, w4);*/

		const auto & points = t_get_points ();
		m_bezier_points.clear ();

		if (points.size () < 2) {
			return;
		}

		using spline_segment_t = std::array<glm::vec3, 4>;

		// at least four bspline points
		const unsigned int n = points.size () - 2;

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
			d[i] = glm::length (P[i + 1] - P[i]);
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
			for (int i = 0; i < n - 1; ++i) {
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
		for (int i = 0; i < n; ++i) {
			power[i][0] = P[i];

		}
	}

	interpolating_curve::float_array_t interpolating_curve::m_solve_tridiag (
		const float_array_t & a, 
		const float_array_t & b, 
		const float_array_t & c,
		const float_array_t & d) {

		const int n = b.size ();
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
}