#pragma once
#include <glad/glad.h>

#include "context.hpp"
#include "object.hpp"

namespace mini {
	class gizmo : public graphics_obj_t {
		public:
			enum class gizmo_mode_t {
				translation,
				rotation,
				scale
			};

		private:
			struct gizmo_mesh_t {
				GLuint pos_buffer, index_buffer, vao;

				std::vector<float> positions;
				std::vector<GLuint> indices;

				gizmo_mesh_t () {
					pos_buffer = 0;
					index_buffer = 0;
					vao = 0;
				}
			};
			
			gizmo_mesh_t m_mesh_arrow;
			std::shared_ptr<shader_t> m_shader_mesh;
			std::shared_ptr<shader_t> m_shader_line;

			gizmo_mode_t m_mode;

		public:
			gizmo_mode_t get_mode () const;
			void set_mode (gizmo_mode_t mode);

			gizmo (std::shared_ptr<shader_t> shader_mesh, std::shared_ptr<shader_t> shader_line);
			~gizmo ();

			gizmo (const gizmo &) = delete;
			gizmo & operator= (const gizmo &) = delete;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

		private:
			void m_render_translation (app_context & context, const glm::mat4x4 & world_matrix) const;

			void m_gen_arrow_mesh (gizmo_mesh_t & mesh);
			void m_free_mesh (gizmo_mesh_t & mesh);
	};
}