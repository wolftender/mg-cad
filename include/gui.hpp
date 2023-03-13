#pragma once
#include <string>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include "algebra.hpp"

namespace mini {
	namespace gui {
		void prefix_label (const std::string & label, float min_width = 0.0f);
		void vector_editor (const std::string & label, float_vector_t & vector);
		
		template<typename T> void clamp (T & value, T min, T max) {
			if (value < min) {
				value = min;
			} else if (value > max) {
				value = max;
			}
		}
	}
}