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

			bicubic_surface::surface_patch m_patch1;
			bicubic_surface::surface_patch m_patch2;
			bicubic_surface::surface_patch m_patch3;

			std::weak_ptr<bezier_surface_c0> m_surface1;
			std::weak_ptr<bezier_surface_c0> m_surface2;
			std::weak_ptr<bezier_surface_c0> m_surface3;

			// used for reindexing
			patch_indexing_t m_index1;
			patch_indexing_t m_index2;
			patch_indexing_t m_index3;

			std::shared_ptr<shader_t> m_solid_shader;
			std::shared_ptr<shader_t> m_line_shader;
			std::shared_ptr<shader_t> m_bezier_shader;

			std::vector<float> m_positions;
			std::vector<float> m_line_positions;

			// opengl buffers
			GLuint m_line_vao, m_vao;
			GLuint m_line_buffer, m_pos_buffer;

			bool m_ready;
			int m_res_u, m_res_v;

		public:
			gregory_surface (
				scene_controller_base & scene,
				std::shared_ptr<shader_t> shader,
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

		private:
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