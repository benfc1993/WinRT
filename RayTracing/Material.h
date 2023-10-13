#pragma once

#include <glm/glm.hpp>

struct Material
{
	glm::vec3 Albedo = { 1,1,1 };
	glm::vec3 EmissionColor = { 1,1,1 };
	float EmissionStrength = 0;
	float roughness = 1.0f;
	float Specular = 0.0f;
	glm::vec3 SpecularColor = { 1,1,1 };
};