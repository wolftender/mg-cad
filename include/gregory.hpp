#pragma once
#include "object.hpp"
#include "point.hpp"
#include "surface.hpp"
#include "bezier.hpp"
#include "beziersurf.hpp"

namespace mini {
	struct patch_offset_t {
		int x, y;
	};

	patch_offset_t operator+ (const patch_offset_t & a, const patch_offset_t & b);
	patch_offset_t operator- (const patch_offset_t & a, const patch_offset_t & b);
	bool operator== (const patch_offset_t & a, const patch_offset_t & b);
	bool operator!= (const patch_offset_t & a, const patch_offset_t & b);

	struct patch_indexing_t {
		patch_offset_t start, end;
	};

	class gregory_surface : public scene_obj_t {
		private:
			// a quarter of bezier surface
			using quarter_surface = std::array<glm::vec3, 20>;
			using gregory_patch = std::array<glm::vec3, 16>;

			uint64_t m_id_surf1, m_id_surf2, m_id_surf3;

			bicubic_surface::surface_patch m_patch1;
			bicubic_surface::surface_patch m_patch2;
			bicubic_surface::surface_patch m_patch3;

			// used for reindexing
			patch_indexing_t m_index1;
			patch_indexing_t m_index2;
			patch_indexing_t m_index3;

			std::shared_ptr<shader_t> m_isoline_shader;
			std::shared_ptr<shader_t> m_solid_shader;
			std::shared_ptr<shader_t> m_line_shader;
			std::shared_ptr<shader_t> m_bezier_shader;

			std::vector<float> m_positions;
			std::vector<float> m_line_positions;

			// opengl buffers
			GLuint m_line_vao, m_vao;
			GLuint m_line_buffer, m_pos_buffer;

			bool m_ready, m_signals_setup, m_rebuild_queued, m_use_wireframe, m_show_grid, m_use_solid;
			int m_res_u, m_res_v;

			glm::vec4 m_color, m_grid_color;

		public:
			gregory_surface (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
				std::shared_ptr<shader_t> shader_solid,
				std::shared_ptr<shader_t> line_shader,
				std::shared_ptr<shader_t> bezier_shader,
				const bicubic_surface::surface_patch & patch1,
				const bicubic_surface::surface_patch & patch2,
				const bicubic_surface::surface_patch & patch3,
				const patch_indexing_t & index1,
				const patch_indexing_t & index2,
				const patch_indexing_t & index3
			);

			~gregory_surface ();

			gregory_surface (const gregory_surface &) = delete;
			gregory_surface & operator= (const gregory_surface &) = delete;

			virtual void configure () override;
			virtual void integrate (float delta_time) override;
			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		protected:
			virtual void t_on_object_deleted (std::shared_ptr<scene_obj_t> object) override;

		private:
			void m_setup_signals ();
			void m_changed_sighandler (signal_event_t sig, scene_obj_t & sender);
			void m_topology_sighandler (signal_event_t sig, scene_obj_t & sender);

			void m_initialize_buffers ();
			void m_destroy_buffers ();
			void m_bind_shader (app_context & context, shader_t & shader, const glm::mat4x4 & world_matrix) const;

			void m_calculate_points ();

			void m_calculate_adjacent_surf (
				quarter_surface & surface1,
				quarter_surface & surface2,
				const bicubic_surface::surface_patch & patch, 
				const patch_indexing_t idx
			);
	};
}