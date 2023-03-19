#pragma once
#include "algebra.hpp"

namespace mini {
	class camera {
		private:
			float m_fov, m_near, m_far, m_aspect;

			glm::vec3 m_position, m_target;
			glm::mat4x4 m_view, m_projection, m_projection_inv, m_view_inv;

		public:
			float get_fov () const;
			float get_near () const;
			float get_far () const;
			float get_aspect () const;

			const glm::mat4x4 & get_view_matrix () const;
			const glm::mat4x4 & get_projection_matrix () const;
			const glm::mat4x4 & get_view_inverse () const;
			const glm::mat4x4 & get_projection_inverse () const;

			const glm::vec3 & get_position () const;
			const glm::vec3 & get_target () const;

			void set_fov (float fov);
			void set_near (float near);
			void set_far (float far);
			void set_aspect (float aspect);

			void set_position (const glm::vec3 & position);
			void set_target (const glm::vec3 & target);

			camera ();
			camera (const glm::vec3 & position, const glm::vec3 & target);

		private:
			void m_recalculate_view ();
			void m_recalculate_projection ();
	};
}