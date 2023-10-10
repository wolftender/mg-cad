#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "object.hpp"

namespace mini {
    class curve : public scene_obj_t {
        private:
            std::vector<glm::vec3> m_points;
            std::shared_ptr<shader_t> m_line_shader;

            std::vector<float> m_positions;
            std::vector<uint32_t> m_indices;
            GLuint m_vao, m_position_buffer, m_index_buffer;

            bool m_ready;

        public:
            curve(
                scene_controller_base& scene, 
                std::shared_ptr<shader_t> line_shader
            );

            curve(
                scene_controller_base& scene, 
                std::shared_ptr<shader_t> line_shader, 
                const std::vector<glm::vec3> & points
            );

            curve(const curve&) = delete;
            curve& operator=(const curve&) = delete;

            void append_position(const glm::vec3& position);
            void prepend_position(const glm::vec3& position);

            void append_positions(const std::vector<glm::vec3> & positions);
            void prepend_positions(const std::vector<glm::vec3> & positions);

            void erase_tail();
            void erase_head();

            virtual void configure () override;
			virtual void integrate (float delta_time) override;
            virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;

        private:
            void m_rebuild_buffers();
            void m_free_buffers();
    };
}