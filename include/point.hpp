#pragma once
#include "object.hpp"
#include "billboard.hpp"

namespace mini {
	class point_object : public scene_obj_t {
		private:
			billboard_object m_billboard;

		public:
			point_object (std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture);
			~point_object () = default;
	};
}