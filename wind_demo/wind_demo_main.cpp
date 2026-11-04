#include "core/engine.hpp"
#include "core/ecs.hpp"
#include "components/transform.hpp"
#include "core/resources.hpp"
#include "components/rendering_components.hpp"
#include "resources/model.hpp"
#include "core/input.hpp"
#include "resources/skybox.hpp"
#include "rendering/rendering/renderer_dx.hpp"

class CameraSystem : public SE::System
{
public:
	virtual void Update(float dt) override
	{
		const float speed = 25.f * dt;
		const float RotationSpeed = glm::radians(45.f * dt);
		for (const auto& [ent, cam, trans] : SE::Engine.ECS().Registry.view<SE::Camera, SE::Transform>().each())
		{
			auto input = trans.GetTranslation();
			auto rotation = trans.GetRotation();
			auto righ = (rotation * glm::vec3(1, 0, 0)) * speed;
			auto forward = (rotation * glm::vec3(0, 0, 1)) * speed;

			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::W))
			{
				input -= forward;
				// GPU_RIPPER::GetStatistics();
			}
			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::S))
			{
				input += forward;
			}
			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::A))
			{
				input -= righ;
			}
			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::D))
			{
				input += righ;
			}

			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::E))
			{
				rotation *= glm::angleAxis(-RotationSpeed, glm::vec3(0, 1, 0));
			}
			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::Q))
			{
				rotation *= glm::angleAxis(RotationSpeed, glm::vec3(0, 1, 0));
			}
			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::R))
			{
				rotation *= glm::angleAxis(-RotationSpeed, glm::vec3(1, 0, 0));
			}
			if (SE::Engine.Input().GetKeyboardKey(SE::Input::KeyboardKey::F))
			{
				rotation *= glm::angleAxis(RotationSpeed, glm::vec3(1, 0, 0));
			}

			trans.SetTranslation(input);
			trans.SetRotation(rotation);
		}
	}
};

void LoadShowcase()
{
	// camera
	{
		auto rot = glm::identity<glm::quat>();
		rot *= glm::angleAxis(glm::radians(-15.f), glm::vec3(1, 0, 0));

		auto cam = SE::Engine.ECS().CreateEntity();
		auto& camTrans = SE::Engine.ECS().CreateComponent<SE::Transform>(cam, glm::vec3(0, 2, 6), glm::vec3(1), rot);
		camTrans.SetName("Camera");
		SE::Engine.ECS().CreateComponent<SE::Camera>(cam, glm::perspective(glm::radians(45.f), 16.0f / 9.0f, 0.1f, 100.0f));
	}

	// models
	{
		{
			auto model = SE::Engine.Resources().Load<SE::Model>(SE::FileIO::Directory::Assets, "SciFiHelmet.gltf");

			auto mod = SE::Engine.ECS().CreateEntity();
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(mod);
			trans.SetName("scifi helmet");
			trans.SetTranslation(glm::vec3(2, 0, 0));
			trans.SetScale(glm::vec3(0.5f));
			model->InstanciateModel(mod);
		}
	}

	// skybox
	{
		auto sk = SE::Engine.Resources().Load<SE::Skybox>(SE::FileIO::Directory::Assets, "qwantani_sunset_2k.hdr");
		auto ent = SE::Engine.ECS().CreateEntity();
		auto& env = SE::Engine.ECS().CreateComponent<SE::Enviroment>(ent);
		env.m_skyox = sk;
	}

	// lights
	{
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(1, 1, 1);
			light.intensity = 1.f;
			light.type = SE::Light::Direct;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("directLight");
		}

		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(.1f, .1f, .1f);
			light.intensity = 1.f;
			light.range = 5.f;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point");
			trans.SetTranslation(glm::vec3(0, 0, 0));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(.5f, .1f, .1f);
			light.intensity = 5;
			light.range = 5;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 1");
			trans.SetTranslation(glm::vec3(0, 2.5f, 2));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(.1f, .1f, .5f);
			light.intensity = 5;
			light.range = 9;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 2");
			trans.SetTranslation(glm::vec3(3, 0.5f, 2));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(.1f, .5f, .1f);
			light.intensity = 10;
			light.range = 5.f;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 3");
			trans.SetTranslation(glm::vec3(-3, 0.5f, 2));
		}

		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(.25f, 0, 0);
			light.intensity = 9;
			light.range = 25;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 4");
			trans.SetTranslation(glm::vec3(0, -2.7f, 0.2f));
		}


		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(.2f, .35f, 0);
			light.intensity = 7.5f;
			light.range = 10;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 5");
			trans.SetTranslation(glm::vec3(3.7, -1, 2));
		}

		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0, 0, .3f);
			light.intensity = 4;
			light.range = 10;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 6");
			trans.SetTranslation(glm::vec3(-4, 2.5f, -0.25f));
		}

		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0.25f, 0, .2f);
			light.intensity = 2;
			light.range = 7.5f;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 7");
			trans.SetTranslation(glm::vec3(0, -.5f, -3.6f));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0, 0.1f, 0.15f);
			light.intensity = 5;
			light.range = 5;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 8");
			trans.SetTranslation(glm::vec3(-1.5f, 2.5f, -2));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0.4f, 0, 0.3f);
			light.intensity = 4;
			light.range = 15;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 9");
			trans.SetTranslation(glm::vec3(1.5f, 2.5f, -7));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0, 0.2f, .2f);
			light.intensity = 2;
			light.range = 10;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 10");
			trans.SetTranslation(glm::vec3(0, 2.8f, 3));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0, 0.4f, 0.3f);
			light.intensity = 5;
			light.range = 10;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 11");
			trans.SetTranslation(glm::vec3(3, 2.5f, -3));
		}
		{
			auto ent = SE::Engine.ECS().CreateEntity();
			auto& light = SE::Engine.ECS().CreateComponent<SE::Light>(ent);
			light.color = glm::vec3(0.4f, 0.25f, 0.2f);
			light.intensity = 0;
			light.range = 10;
			light.type = SE::Light::Point;
			auto& trans = SE::Engine.ECS().CreateComponent<SE::Transform>(ent);
			trans.SetName("Point 12");
			trans.SetTranslation(glm::vec3(-2, 2.5f, -0.25f));
		}
	}
}

int main()
{
	SE::Engine.Initialize();

	SE::Engine.ECS().CreateSystem<SE::Renderer>();
	SE::Engine.ECS().CreateSystem<CameraSystem>();

	LoadShowcase();

	SE::Engine.Run();
	SE::Engine.Shutdown();

	return 0;
}