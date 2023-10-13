#pragma once

#include <glm/glm.hpp>

#include <vector>
#include <unordered_map>
#include "Material.h"

struct Sphere
{
	glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
	float Radius = 0.5f;

	int MaterialIndex = 0;
};

struct Scene
{
	int currentId = 0;
	std::vector<int> SphereIds;
	std::unordered_map<int, Sphere> Spheres;
	std::vector<Material> Materials;
	glm::vec3 LightDir = { 1,1,1 };

	float SunFocus = 1.0f;
	float SunIntensity = 0.002f;
	void AddSphere(Sphere sphere)
	{
		SphereIds.push_back(currentId);
		Spheres[currentId] = sphere;
		currentId++;
	}
};
