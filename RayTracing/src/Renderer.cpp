#include "Renderer.h"
#include "Walnut/Random.h"

#define _USE_MATH_DEFINES

#include <execution>
#include <math.h>

namespace Utils
{
	uint32_t ConvertToRGBA(glm::vec4 color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;

		return result;
	}

	float magSqr(glm::vec3 v)
	{
		return v.x * v.x + v.y * v.y + v.z * v.z;
	}

	float mag(glm::vec3 v)
	{
		return glm::sqrt(magSqr(v));
	}

	float distSqr(glm::vec3 a, glm::vec3 b)
	{
		const auto diffVec = a - b;
		return diffVec.x * diffVec.x + diffVec.y * diffVec.y + diffVec.z * diffVec.z;
	}

	float dist(glm::vec3 a, glm::vec3 b)
	{
		return glm::sqrt(distSqr(a, b));
	}

	float smoothstep(float edge0, float edge1, float x) {
		// Scale, bias and saturate x to 0..1 range
		x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
		// Evaluate polynomial
		return x * x * (3 - 2 * x);
	}

	float lerp(float a, float b, float t) {
		return a + ((b - a) * t);
	}

	glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t) {
		return a + ((b - a) * t);
	}

	glm::vec4 lerp(glm::vec4 a, glm::vec4 b, float t) {
		return a + ((b - a) * t);
	}

	float RandomValueNormalDistribution()
	{
		// Thanks to https://stackoverflow.com/a/6178290
		float theta = 2 * 3.1415926 * Walnut::Random::Float();
		float rho = sqrt(-2 * log(Walnut::Random::Float()));
		return rho * cos(theta);
	}

	// Calculate a random direction
	glm::vec3 RandomDirection()
	{
		// Thanks to https://math.stackexchange.com/a/1585996
		float x = RandomValueNormalDistribution();
		float y = RandomValueNormalDistribution();
		float z = RandomValueNormalDistribution();
		return glm::normalize(glm::vec3(x, y, z));
	}

	glm::vec2 RandomPointInCircle()
	{
		float angle = Walnut::Random::Float() * 2 * M_PI;
		glm::vec2 pointOnCircle = { cos(angle), sin(angle) };
		return pointOnCircle * glm::sqrt(Walnut::Random::Float());
	}
}

void Renderer::ZSort(Scene& scene, Camera camera)
{
	std::sort(scene.SphereIds.begin(), scene.SphereIds.end(), [&](int a, int b) {
		const auto vecA = scene.Spheres.at(a).Position - camera.GetPosition();

		const float magA = Utils::mag(vecA) - scene.Spheres.at(a).Radius;

		const auto vecB = scene.Spheres.at(b).Position - camera.GetPosition();
		const float magB = Utils::mag(vecB) - scene.Spheres.at(b).Radius;
		return magA < magB;
		});
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetWidth() * m_FinalImage->GetHeight() * sizeof(glm::vec4));

	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;


	std::for_each(std::execution::par, m_VerticalItterator.begin(), m_VerticalItterator.end(), [this](uint32_t y) {
		std::for_each(std::execution::par, m_HorizontalItterator.begin(), m_HorizontalItterator.end(), [this, y](uint32_t x) {


			glm::vec4 pixelColor = PerPixel(x, y);

			auto index = x + y * m_FinalImage->GetWidth();
			m_AccumulationData[x + y * m_FinalImage->GetWidth()] += pixelColor;

			glm::vec4 accumulatedColor = m_AccumulationData[index];
			accumulatedColor /= (float)m_FrameIndex;

			accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[index] = Utils::ConvertToRGBA(accumulatedColor);
			});
		});


	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

void Renderer::OnResize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		if (!m_FinalImage->Resize(width, height))
			return;

	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_HorizontalItterator.resize(width);
	m_VerticalItterator.resize(height);
	for (uint32_t i = 0; i < width; i++)
		m_HorizontalItterator[i] = i;
	for (uint32_t i = 0; i < height; i++)
		m_VerticalItterator[i] = i;
}

glm::vec4 Renderer::PerPixel(uint32_t x, uint32_t y)
{
	auto index = x + y * m_FinalImage->GetWidth();
	glm::vec3 light = { 0,0,0 };
	glm::vec3 color = { 1,1,1 };

	for (int numRays = 0; numRays < m_Settings.RaysPerPixel; numRays++)
	{
		glm::vec3 rayLight = { 0,0,0 };
		glm::vec3 rayColor = { 1,1,1 };

		Ray ray;
		ray.Origin = m_ActiveCamera->GetPosition();
		glm::vec3 cameraRotation = m_ActiveCamera->GetRotation();
		float pitch = cameraRotation.x;
		float yaw = cameraRotation.z;
		glm::vec3 cameraUp = { sin(pitch) * sin(yaw), cos(pitch), sin(pitch) * cos(yaw) };
		glm::vec3 cameraRight = { cos(yaw), 0, -sin(yaw) };
		glm::vec2 jitter = Utils::RandomPointInCircle() * m_Settings.AntiAlias / (float)m_HorizontalItterator.size();
		glm::vec3 jitteredViewPoint = m_ActiveCamera->GetPosition() + cameraRight.x * jitter.x + cameraUp.y * jitter.y;
		ray.Direction = jitteredViewPoint - ray.Origin + m_ActiveCamera->GetRayDirections()[index];



		for (int i = 0; i < m_Settings.NumBounces; i++)
		{
			HitInfo hitInfo = Renderer::TraceRay(ray);
			if (hitInfo.ObjectIndex != -1)
			{
				Material mat = m_ActiveScene->Materials[m_ActiveScene->Spheres.at(hitInfo.ObjectIndex).MaterialIndex];
				glm::vec3 diffuseDir = glm::normalize(hitInfo.Normal + Utils::RandomDirection());
				glm::vec3 reflectionDir = glm::reflect(ray.Direction, hitInfo.Normal);
				float rnd = Walnut::Random::Float();
				bool isSpecularBounce = mat.Specular >= rnd;
				float d = glm::max(glm::dot(hitInfo.Normal, -glm::normalize(ray.Direction)), 0.0f);


				//color *= hitInfo.Normal; // Debugging
				auto emittedLight = (mat.EmissionColor * mat.EmissionStrength * rayColor);

				rayLight += emittedLight;

				rayColor *= Utils::lerp(mat.Albedo, mat.SpecularColor, isSpecularBounce);

				ray.Origin = hitInfo.Point + hitInfo.Normal * 0.0001f;
				ray.Direction = Utils::lerp(diffuseDir, reflectionDir, ((mat.roughness - 1) * -1) * isSpecularBounce);
				//float p = glm::max(color.r, glm::max(color.g, color.b));
		/*		if (Walnut::Random::Float() >= p) {
					break;
				}*/
				//color *= 1.0f / p;
			}
			else {
				glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
				float d = glm::max(glm::dot(hitInfo.Normal, -glm::normalize(ray.Direction)), 0.0f);

				rayColor *= skyColor;
				rayLight += rayColor;
				break;
			}
		}
		rayLight /= (float)m_Settings.RaysPerPixel;
		light += rayLight;
	}
	return glm::vec4(light , 1);

}

Renderer::HitInfo Renderer::TraceRay(const Ray& ray)
{
	HitInfo hitInfo;
	hitInfo.ObjectIndex = -1;

	if (m_ActiveScene->Spheres.size() == 0)

		return hitInfo;

	float closestHit = INFINITY;
	int closestSphere = -1;


	for (int i = 0; i < m_ActiveScene->SphereIds.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres.at(m_ActiveScene->SphereIds[i]);
		//if (glm::dot(sphere.Position - ray.Origin, m_ActiveCamera->GetDirection()) < 0.0f)
		//	continue;

		glm::vec3 offsetRayOrigin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(offsetRayOrigin, ray.Direction);
		float c = glm::dot(offsetRayOrigin, offsetRayOrigin) - sphere.Radius * sphere.Radius;

		float discriminant = b * b - 4.0f * a * c;
		if (discriminant < 0.0f)
			continue;

		float discriminantSqrt = glm::sqrt(discriminant);

		float t2 = (-b - discriminantSqrt) / (2.0f * a);

		if (t2 > 0.0f && t2 < closestHit)
		{
			closestHit = t2;
			closestSphere = i;
		}

		//if (i == m_ActiveScene->SphereIds.size() - 1)
		//	break;

		//const auto nextSphere = m_ActiveScene->Spheres.at(m_ActiveScene->SphereIds[i + 1]);
		//float dist = Utils::distSqr(sphere.Position, nextSphere.Position);
		//const auto radiusSqr = (sphere.Radius + nextSphere.Radius) * (sphere.Radius + nextSphere.Radius);
		//if (dist > radiusSqr)
		//	break;

	}

	if (closestSphere == -1)
		return hitInfo;


	const Sphere& sphere = m_ActiveScene->Spheres.at(m_ActiveScene->SphereIds[closestSphere]);
	glm::vec3 origin = ray.Origin - sphere.Position;

	glm::vec3 hitPosition = origin + ray.Direction * closestHit;
	glm::vec3 hitNormal = glm::normalize(hitPosition);

	hitInfo.ObjectIndex = m_ActiveScene->SphereIds[closestSphere];
	hitInfo.Normal = hitNormal;
	hitInfo.Point = hitPosition + sphere.Position;
	hitInfo.hitDistance = closestHit;
	return hitInfo;

}
