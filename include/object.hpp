#pragma once
#include "context.hpp"
#include "algebra.hpp"

namespace mini {
	class scene_obj_t : public graphics_obj_t, std::enable_shared_from_this<scene_obj_t> {
		private:
			std::string m_type_name;

			float_vector_t m_translation;
			float_vector_t m_euler_angles;
			float_vector_t m_scale;

			bool m_selected;

		public:
			const std::string & get_type_name () const;

			const float_vector_t & get_translation () const;
			const float_vector_t & get_euler_angles () const;
			const float_vector_t & get_scale () const;
			bool is_selected () const;

			void set_translation (const float_vector_t & translation);
			void set_euler_angles (const float_vector_t & euler_angles);
			void set_scale (const float_vector_t & scale);
			void set_selected (bool selected);

			float_matrix_t get_matrix () const;

			scene_obj_t (const std::string & type_name);
			virtual ~scene_obj_t ();

			// virtual methods
			virtual void configure ();
	};
}