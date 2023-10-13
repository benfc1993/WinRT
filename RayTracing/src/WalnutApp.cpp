#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"
#include "Renderer.h"
#include "../Camera.h"
#include "../Scene.h"

#include <glm/gtc/type_ptr.hpp>

using namespace Walnut;

class ExampleLayer : public Walnut::Layer
{
public:
	ExampleLayer() : m_Camera(45.0f, 0.1f, 100.0f)
	{
		Material pinkMat;
		pinkMat.Albedo = { 1,0,1 };
		pinkMat.roughness = 0.3f;
		pinkMat.Specular = 0.72f;
		m_Scene.Materials.push_back(pinkMat);

		Material blueMat;
		blueMat.Albedo = { 0.02f,0.3f,1 };
		blueMat.roughness = 0.0f;
		blueMat.Specular = 0.07f;
		m_Scene.Materials.push_back(blueMat);

		Material greenMat;
		greenMat.Albedo = { 0.02f,1,0.3f };
		greenMat.EmissionStrength = 0.9f;
		greenMat.EmissionColor = { 1.000f, 0.937f, 0.491f };
		m_Scene.Materials.push_back(greenMat);

		{
			Material mat;
			mat.EmissionStrength = 12.0f;
			m_Scene.Materials.push_back(mat);
		}

		{
			Material mat;
			mat.Albedo = { 1.000f, 0.982f, 0.000f };
			mat.roughness = 0.89f;
			m_Scene.Materials.push_back(mat);
		}

		{
			Material mat;
			mat.Albedo = { 1.000f, 0.0f, 0.0f };
			mat.roughness = 0.33f;
			mat.Specular = 0.37f;
			m_Scene.Materials.push_back(mat);
		}


		{
			Sphere sphere;
			sphere.Radius = 0.350f;
			sphere.Position = { 0, -0.7f, 0 };
			m_Scene.AddSphere(sphere);
		}

		{
			Sphere sphere;
			sphere.Radius = 8.79f;
			sphere.MaterialIndex = 2;
			sphere.Position = { -6.6f, 4.4f, -17.9f };
			m_Scene.AddSphere(sphere);
		}

		{
			Sphere sphere;
			sphere.Radius = 0.51f;
			sphere.MaterialIndex = 1;
			sphere.Position = { -1.6f, 0, 0 };
			m_Scene.AddSphere(sphere);
		}

		{
			Sphere sphere;
			sphere.Radius = 30.0f;
			sphere.MaterialIndex = 0;
			sphere.Position = { 0, -31.3f, 0 };
			m_Scene.AddSphere(sphere);
		}

		{
			Sphere sphere;
			sphere.Radius = 0.5f;
			sphere.MaterialIndex = 3;
			sphere.Position = { -2.9f, 3.1f, 0 };
			m_Scene.AddSphere(sphere);
		}

		{
			Sphere sphere;
			sphere.Radius = 0.5f;
			sphere.MaterialIndex = 4;
			sphere.Position = { -3.2f, -1.2f, 0 };
			m_Scene.AddSphere(sphere);
		}

		{
			Sphere sphere;
			sphere.Radius = 0.5f;
			sphere.MaterialIndex = 5;
			sphere.Position = { 1.4f, -1.6f, 0 };
			m_Scene.AddSphere(sphere);
		}
	}
	virtual void OnUpdate(float ts) override
	{
		if (m_Camera.OnUpdate(ts))
		{
			ReEval();
			m_Renderer.ResetFrameIndex();
		}
	}
	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");

		ImGui::Text("Last render: %.3fms", m_LastRenderTime);

		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
		ImGui::DragFloat("Number of bounces", &m_Renderer.GetSettings().NumBounces);
		ImGui::DragFloat("Anti Alias", &m_Renderer.GetSettings().AntiAlias);
		ImGui::DragInt("Rays Per Pixel", &m_Renderer.GetSettings().RaysPerPixel);

		if (ImGui::Button("Reset"))
			m_Renderer.ResetFrameIndex();

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportHeight = ImGui::GetContentRegionAvail().y;
		m_ViewportWidth = ImGui::GetContentRegionAvail().x;

		auto image = m_Renderer.GetFinalImage();
		if (image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() }, { 0,1 }, { 1, 0 });

		ImGui::End();
		ImGui::PopStyleVar();

		ScenePanel();

		Render();
	}

	void Render()
	{
		Timer timer;
		m_Renderer.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		m_LastRenderTime = timer.ElapsedMillis();
	}

	void ScenePanel()
	{
		ImGui::Begin("Scene");

		ImGui::DragFloat3("Light direction", glm::value_ptr(m_Scene.LightDir), 0.1f);
		ImGui::DragFloat("Sun Focus", &m_Scene.SunFocus, 0.1f);
		ImGui::DragFloat("Sun Intensity", &m_Scene.SunIntensity, 0.1f);

		if (ImGui::CollapsingHeader("Spheres"))
		{
			if (ImGui::Button("Add Sphere"))
				AddSphere();

			for (int i = 0; i < m_Scene.Spheres.size(); i++) {
				ImGui::PushID(i);
				Sphere& sphere = m_Scene.Spheres[i];
				if (ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f))
					ReEval();
				if (ImGui::DragFloat("Radius", &sphere.Radius, 0.01f))
					ReEval();
				ImGui::SliderInt("Material Index", &sphere.MaterialIndex, 0, static_cast<int>(m_Scene.Materials.size() - 1));
				ImGui::Spacing();
				ImGui::PopID();
			}

		}

		if (ImGui::CollapsingHeader("Materials"))
		{
			if (ImGui::Button("Add Material"))
				AddMaterial();
			for (int i = 0; i < m_Scene.Materials.size(); i++)
			{
				ImGui::PushID(i);
				Material& mat = m_Scene.Materials[i];
				ImGui::ColorEdit3("Albedo", &mat.Albedo.r);
				ImGui::DragFloat("Roughness", &mat.roughness, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Specular", &mat.Specular, 0.01f, 0.0f, 1.0f);
				ImGui::ColorEdit3("Specular Color", &mat.SpecularColor.r);
				ImGui::ColorEdit3("Emission Color", &mat.EmissionColor.r);
				ImGui::DragFloat("Emission Strength", &mat.EmissionStrength);
				ImGui::PopID();
			}
		}

		ImGui::End();
	}

	void AddSphere()
	{
		Sphere sphere;
		m_Scene.AddSphere(sphere);
	}

	void AddMaterial()
	{
		Material mat;
		m_Scene.Materials.push_back(mat);
	}

	void ReEval()
	{
		m_Renderer.ZSort(m_Scene, m_Camera);
	}

private:
	Camera m_Camera;
	Scene m_Scene;
	Renderer m_Renderer;
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
	float m_LastRenderTime = 0.0f;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Exit"))
				{
					app->Close();
				}
				ImGui::EndMenu();
			}
		});
	return app;
}