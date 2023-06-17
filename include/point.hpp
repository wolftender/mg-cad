#pragma once
#include <set>

#include "object.hpp"
#include "billboard.hpp"

namespace mini {
	class point_object;

	using point_ptr = std::shared_ptr<point_object>;
	using point_wptr = std::weak_ptr<point_object>;

	class point_family_base : public scene_obj_t {
		private:
			bool m_destroy_allowed;
			bool m_merge_allowed;

		public:
			bool get_destroy_allowed () const;
			bool get_merge_allowed () const;

			point_family_base (scene_controller_base & scene, const std::string & type_name, bool destroy_allowed, bool merge_allowed);
			virtual ~point_family_base () = default;

		protected:
			void t_set_destroy_allowed (bool value);
			void t_set_merge_allowed (bool value);

			virtual void t_on_point_destroy (const point_ptr point) = 0;
			virtual void t_on_point_merge (const point_ptr point, const point_ptr merge) = 0;

		private:	
			void m_point_destroy (const point_ptr point);
			void m_point_merge (const point_ptr point, const point_ptr merge);

		friend class point_object;
	};

	class point_object : public scene_obj_t {
		public:
			static constexpr glm::vec4 s_color_default = { 1.0f, 1.0f, 1.0f, 1.0f };
			static constexpr glm::vec4 s_select_default = { 0.960f, 0.646f, 0.0192f, 1.0f };

		private:
			billboard_object m_billboard;
			glm::vec4 m_color, m_selected_color;
			bool m_mergeable;

			std::list<std::weak_ptr<point_family_base>> m_parents;

		public:
			const glm::vec4 & get_color () const;
			const glm::vec4 & get_select_color () const;
			bool is_mergeable () const;

			void set_color (const glm::vec4 & color);
			void set_select_color (const glm::vec4 & color);

			point_object (scene_controller_base & scene, std::shared_ptr<shader_t> shader, std::shared_ptr<texture_t> texture);
			~point_object () = default;

			virtual void render (app_context & context, const glm::mat4x4 & world_matrix) const override;
			virtual void configure () override;
			virtual bool hit_test (const hit_test_data_t & data, glm::vec3 & hit_pos) const override;
			virtual bool box_test (const box_test_data_t & data) const override;

			void add_parent (std::shared_ptr<point_family_base> family);
			void clear_parent (const point_family_base & family);
			void merge (point_ptr point);

		protected:
			virtual void t_on_selection (bool selected) override;
			virtual void t_on_alt_select () override;

		private:
			void m_check_deletable ();
	};
}