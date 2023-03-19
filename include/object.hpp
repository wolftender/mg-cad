#pragma once
#include "context.hpp"
#include "algebra.hpp"

namespace mini {
	class scene_obj_t : public graphics_obj_t, std::enable_shared_from_this<scene_obj_t> {
		private:
			std::string m_type_name;

			glm::vec3 m_translation;
			glm::vec3 m_euler_angles;
			glm::vec3 m_scale;

			bool m_selected;

		public:
			const std::string & get_type_name () const;

			const glm::vec3 & get_translation () const;
			const glm::vec3 & get_euler_angles () const;
			const glm::vec3 & get_scale () const;
			bool is_selected () const;

			void set_translation (const glm::vec3 & translation);
			void set_euler_angles (const glm::vec3 & euler_angles);
			void set_scale (const glm::vec3 & scale);
			void set_selected (bool selected);

			glm::mat4x4 get_matrix () const;

			scene_obj_t (const std::string & type_name);
			virtual ~scene_obj_t ();

			// virtual methods
			virtual void configure ();
	};
}