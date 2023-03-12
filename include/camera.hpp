#pragma once
#include "algebra.hpp"

namespace mini {
	class camera {
		private:
			float m_fov, m_near, m_far, m_aspect;

			float_vector_t m_position, m_target;
			float_matrix_t m_view, m_projection;

		public:
			float get_fov () const;
			float get_near () const;
			float get_far () const;
			float get_aspect () const;

			const float_matrix_t & get_view_matrix () const;
			const float_matrix_t & get_projection_matrix () const;

			const float_vector_t & get_position () const;
			const float_vector_t & get_target () const;

			void set_fov (float fov);
			void set_near (float near);
			void set_far (float far);
			void set_aspect (float aspect);

			void set_position (const float_vector_t & position);
			void set_target (const float_vector_t & target);

			camera ();
			camera (const float_vector_t & position, const float_vector_t & target);

		private:
			void m_recalculate_view ();
			void m_recalculate_projection ();
	};
}