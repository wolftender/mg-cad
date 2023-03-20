#include "factory.hpp"
#include "gui.hpp"

// objects
#include "cube.hpp"
#include "torus.hpp"

namespace mini {
	object_factory::object_factory_impl_t::object_factory_impl_t (object_ctor_t c, const std::string & n, const std::string & d) :
		ctor (c),
		name (n),
		description (d) { }

	object_factory::object_factory (std::shared_ptr<resource_store> store) {
		m_store = store;

		m_factories.push_back ({
			&object_factory::make_test_cube, "cube", "a text cube that displays basic shader on itself"
		});

		m_factories.push_back ({
			&object_factory::make_torus, "torus", "a parametric torus with adjustable resolution"
		});
	}

	std::shared_ptr<scene_obj_t> object_factory::configure () const {
		std::shared_ptr<scene_obj_t> created = nullptr;
		bool sel = false;

		if (ImGui::BeginListBox ("##objectlist", ImVec2 (-1.0f, ImGui::GetWindowHeight () - 100.0f))) {
			for (auto & object_type : m_factories) {
				if (ImGui::Selectable (object_type.name.c_str (), &sel)) {
					created = object_type.ctor (m_store);
				}
			}

			ImGui::EndListBox ();
		}

		ImGui::NewLine ();
		return created;
	}

	std::shared_ptr<scene_obj_t> object_factory::make_test_cube (std::shared_ptr<const resource_store> store) {
		auto cube = std::make_shared<cube_object> (
			store->get_basic_shader ()
		);

		return cube;
	}

	std::shared_ptr<scene_obj_t> object_factory::make_torus (std::shared_ptr<const resource_store> store) {
		auto torus = std::make_shared<torus_object> (
			store->get_mesh_shader (),
			store->get_alt_mesh_shader (),
			1.0f,
			3.0f
		);

		return torus;
	}
}