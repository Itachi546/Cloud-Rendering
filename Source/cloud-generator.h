#pragma once

#include <memory>

#include "noise-generator/noise-generator.h"

struct GLTexture;
class GLProgram;
struct GLBuffer;

class CloudGenerator
{
public:
	CloudGenerator() = default;

	void Initialize();

	void AddUI();

	void Render(glm::mat4 P, glm::mat4 V, glm::vec3 camPos, float dt);

	void Shutdown();

	glm::vec3 aabbSize;
private:
	std::unique_ptr<GLTexture> mTexture1;
	std::unique_ptr<GLTexture> mTexture2;

	NoiseParams mTex1Params[4];
	NoiseParams mTex2Params[3];

	NoiseGenerator* mNoiseGenerator;
	std::unique_ptr<GLProgram> mRayMarchProgram;

	std::unique_ptr<GLBuffer> mQuadBuffer;

	float mCloudScale = 1.0f;
	glm::vec3 mCloudOffset{ 0.0f };
	float mDensityMultiplier = 7.0f;
	float mDensityThreshold = 0.2f;
	glm::vec3 mLightDirection = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec4 mLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 20.0f);

	bool mShowAABB = true;
};