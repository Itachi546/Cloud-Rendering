#pragma once

#include <memory>

#include "noise-generator/noise-generator.h"

struct GLTexture;

class CloudGenerator
{
public:
	CloudGenerator() = default;

	void Initialize();

	void AddUI();

	void Render();

	void Shutdown();

private:
	std::unique_ptr<GLTexture> mTexture1;
	std::unique_ptr<GLTexture> mTexture2;

	NoiseParams mTex1Params[4];
	NoiseParams mTex2Params[3];

	NoiseGenerator* mNoiseGenerator;

	glm::vec3 aabbMin;
	glm::vec3 aabbMax;

};