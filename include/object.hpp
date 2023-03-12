#pragma once
#include "../include/context.hpp"

namespace mini {
	class scene_obj_t : public graphics_obj_t {
		public:
			virtual const std::string& get_type_name () const = 0;
			virtual void configure () = 0;

			virtual ~scene_obj_t () { }
	};
}