#pragma once
#include <list>

#include "object.hpp"
#include "point.hpp"

namespace mini {
	class bezier_curve_c0 : public scene_obj_t {
		private:
			using point_ptr = std::shared_ptr<point_object>;
			using point_wptr = std::weak_ptr<point_object>;

			class bezier_segment : public graphics_obj_t {
				private:
					GLuint m_vao;
					GLuint m_position_buffer, m_color_buffer;

					std::shared_ptr<shader_t> m_shader;
					std::vector<float> m_positions, m_colors;
					std::array<point_wptr, 4> m_points;

					bool m_ready;

				public:
					bezier_segment (std::shared_ptr<shader_t> shader, point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3);
					~bezier_segment ();

					bezier_segment (const bezier_segment &) = delete;
					bezier_segment & operator= (const bezier_segment &) = delete;

					void integrate (float delta_time);
					virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

				private:
					void m_init_buffers ();
					void m_update_buffers ();
			};
 
			std::vector<std::shared_ptr<bezier_segment>> m_segments;
			std::shared_ptr<shader_t> m_shader;
			std::list<std::weak_ptr<point_object>> m_points;

			bool m_queue_curve_rebuild;
			bool m_auto_extend;
			
		public:
			bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader);
			~bezier_curve_c0 ();

			bezier_curve_c0 (const bezier_curve_c0 &) = delete;
			bezier_curve_c0 & operator= (const bezier_curve_c0 &) = delete;

			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;

		protected:
			virtual void t_on_object_created (std::shared_ptr<scene_obj_t> object) override;
			virtual void t_on_object_deleted (std::shared_ptr<scene_obj_t> object) override;

		private:
			void m_rebuild_curve ();
	};
}