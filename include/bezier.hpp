#pragma once
#include <list>

#include "object.hpp"
#include "point.hpp"

namespace mini {
	using point_ptr = std::shared_ptr<point_object>;
	using point_wptr = std::weak_ptr<point_object>;

	class bezier_segment_base : public graphics_obj_t {
		private:
			std::array<point_wptr, 4> m_points;
			glm::vec4 m_color;

			bool m_show_polygon;

		protected:
			const std::array<point_wptr, 4> t_get_points () const;

		public:
			bool is_showing_polygon () const;
			void set_showing_polygon (bool show);

			const glm::vec4 & get_color () const;
			void set_color (const glm::vec4 & color);

			bezier_segment_base (point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3);
			virtual ~bezier_segment_base () {}

			bezier_segment_base (const bezier_segment_base &) = delete;
			bezier_segment_base & operator= (const bezier_segment_base &) = delete;

			virtual void integrate (float delta_time) = 0;
	};

	class bezier_segment_gpu : public bezier_segment_base {
		private:
			GLuint m_vao, m_vao_poly;
			GLuint m_position_buffer, m_position_buffer_poly;

			std::shared_ptr<shader_t> m_shader;
			std::shared_ptr<shader_t> m_line_shader;
			std::vector<float> m_positions, m_positions_poly;

			bool m_ready;
			int m_degree, m_last_degree;

		public:
			bezier_segment_gpu (std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2, 
				point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3);
			~bezier_segment_gpu ();

			bezier_segment_gpu (const bezier_segment_gpu &) = delete;
			bezier_segment_gpu & operator= (const bezier_segment_gpu &) = delete;

			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		private:
			void m_free_buffers ();
			void m_bind_shader (app_context & context, std::shared_ptr<shader_t> shader, const glm::mat4x4 & world_matrix) const;
			void m_init_positions ();
			void m_init_buffers ();
			void m_update_buffers ();
	};

	class bezier_segment_cpu : public bezier_segment_base {
		private:
			GLuint m_vao, m_poly_vao;
			GLuint m_position_buffer;
			GLuint m_position_buffer_poly;

			std::shared_ptr<shader_t> m_shader, m_poly_shader;
			std::vector<float> m_positions, m_positions_poly;

			scene_controller_base & m_scene;

			bool m_ready;
			int m_divisions, m_last_divisions, m_degree;

		public:
			bezier_segment_cpu (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2, 
				point_wptr p0, point_wptr p1, point_wptr p2, point_wptr p3);
			~bezier_segment_cpu ();

			bezier_segment_cpu (const bezier_segment_gpu &) = delete;
			bezier_segment_cpu & operator= (const bezier_segment_gpu &) = delete;

			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		private:
			void m_free_buffers ();
			void m_init_buffers ();
			void m_update_buffers ();
			bool m_update_positions ();

			float m_decasteljeu (float b00, float b01, float b02, float b03, float t) const;
			float m_decasteljeu (float b00, float b01, float b02, float t) const;
	};

	class curve_base : public scene_obj_t {
		private:
			struct point_wrapper_t {
				std::weak_ptr<point_object> point;
				bool selected;

				point_wrapper_t (std::weak_ptr<point_object> point) : point (point) {
					selected = false;
				}
			};

			std::vector<point_wrapper_t> m_points;

			bool m_queue_curve_rebuild;
			bool m_auto_extend, m_show_polygon, m_configured;

			glm::vec4 m_color;

		private:
			void m_moved_sighandler (signal_event_t sig, scene_obj_t & sender);

		public:
			void rebuild_curve ();

			bool is_rebuild_queued () const;
			bool is_auto_extend () const;
			bool is_show_polygon () const;
			const glm::vec4 & get_color () const;

			void set_rebuild_queued (bool rebuild);
			void set_auto_extend (bool extend);
			void set_show_polygon (bool show);
			void set_color (const glm::vec4 & color);

			std::vector<uint64_t> serialize_points () const;

		protected:
			curve_base (scene_controller_base & scene, const std::string & name);
			virtual ~curve_base () { }

			curve_base (const curve_base &) = delete;
			curve_base & operator=(const curve_base &) = delete;

			const std::vector<point_wrapper_t> & t_get_points () const;
			std::vector<point_wrapper_t> & t_get_points ();

			virtual void configure () override;

			virtual void t_on_object_created (std::shared_ptr<scene_obj_t> object) override;
			virtual void t_on_object_deleted (std::shared_ptr<scene_obj_t> object) override;

			virtual void t_rebuild_curve () = 0;
	};

	class bezier_curve_c0 : public curve_base {
		private: 
			std::vector<std::shared_ptr<bezier_segment_base>> m_segments;
			std::shared_ptr<shader_t> m_shader1, m_shader2;

			const bool m_is_gpu;
			
		public:
			bezier_curve_c0 (scene_controller_base & scene, std::shared_ptr<shader_t> shader1, std::shared_ptr<shader_t> shader2, bool is_gpu = true);
			~bezier_curve_c0 ();

			bezier_curve_c0 (const bezier_curve_c0 &) = delete;
			bezier_curve_c0 & operator= (const bezier_curve_c0 &) = delete;

			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual const object_serializer_base & get_serializer () const;

		protected:
			virtual void t_rebuild_curve () override;
	};
}