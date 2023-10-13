#pragma once

#include "Walnut/Image.h"

#include <memory>
#include <glm/glm.hpp>
#include "../Camera.h"
#include "../Ray.h"
#include "../Scene.h"

class Renderer
{
	struct HitInfo
	{
		glm::vec3 Point;
		glm::vec3 Normal;
		float hitDistance;
		
		uint32_t ObjectIndex;
	};

	struct Settings
	{
		bool Accumulate = true;
		float NumBounces = 10;
		float AntiAlias = 1.3f;
		int RaysPerPixel = 1;
	};
public:
	Renderer() = default;

	void Render(const Scene & scene, const Camera& camera);

	void OnResize(uint32_t width, uint32_t height);

	void ZSort(Scene& scene, Camera camera);

	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

	void ResetFrameIndex() { m_FrameIndex = 1; }
	Settings& GetSettings() { return m_Settings; }

private:
	glm::vec4 PerPixel(uint32_t x, uint32_t y);
	HitInfo TraceRay(const Ray& ray);

private:
	const Scene* m_ActiveScene = nullptr;
	const Camera* m_ActiveCamera = nullptr;

	Settings m_Settings;

	std::shared_ptr<Walnut::Image> m_FinalImage;
	uint32_t* m_ImageData = nullptr;
	std::vector<uint32_t> m_VerticalItterator, m_HorizontalItterator;
	glm::vec4* m_AccumulationData = nullptr;

	uint32_t m_FrameIndex = 1;
};
