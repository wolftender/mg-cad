#include "bezier.hpp"

namespace mini {
	bezier_curve_c0::bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader) : 
		scene_obj_t (scene, "bezier c0", false, false, false) {
	}

	bezier_curve_c0::~bezier_curve_c0 () {
		
	}

	void bezier_curve_c0::integrate (float delta_time) {
		
	}

	void bezier_curve_c0::render (app_context & context, const glm::mat4x4 & world_matrix) const {
		
	}

	void bezier_curve_c0::configure () {
		
	}

	void bezier_curve_c0::t_on_object_created (std::shared_ptr<scene_obj_t> object) {
	}
}