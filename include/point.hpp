#pragma once
#include "object.hpp"
#include "billboard.hpp"

namespace mini {
	class point_object : public scene_obj_t {
		private:
			billboard_object m_billboard;

		public:
			point_object (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture);
			~point_object () = default;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;
			virtual bool hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const override;

		protected:
			virtual void t_on_selection (bool selected) override;
	};
}