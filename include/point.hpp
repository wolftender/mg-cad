#pragma once
#include "object.hpp"
#include "billboard.hpp"

namespace mini {
	class point_object : public scene_obj_t {
		public:
			static constexpr glm::vec4 s_color_default = { 1.0f, 1.0f, 1.0f, 1.0f };
			static constexpr glm::vec4 s_select_default = { 0.960f, 0.646f, 0.0192f, 1.0f };

		private:
			billboard_object m_billboard;
			glm::vec4 m_color, m_selected_color;

		public:
			const glm::vec4 & get_color () const;
			const glm::vec4 & get_select_color () const;

			void set_color (const glm::vec4 & color);
			void set_select_color (const glm::vec4 & color);

			point_object (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture);
			~point_object () = default;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;
			virtual bool hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const override;
			virtual bool box_test (const box_test_data_t & data) const override;

		protected:
			virtual void t_on_selection (bool selected) override;
	};

	using point_ptr = std::shared_ptr<point_object>;
	using point_wptr = std::weak_ptr<point_object>;
}