#pragma once

#include <memory>

#include "noise-generator/noise-generator.h"

struct GLTexture;
class GLProgram;
struct GLBuffer;
class Camera;

class CloudGenerator
{
public:
	CloudGenerator() = default;

	void Initialize();

	void AddUI();

	void Render(Camera* camera, float dt, uint32_t depthTexture, uint32_t colorAttachment);

	void Shutdown();

private:
	std::unique_ptr<GLTexture> mTexture1;
	std::unique_ptr<GLTexture> mTexture2;
	std::unique_ptr<GLTexture> mBlueNoiseTex;

	NoiseParams mTex1Params[4];
	NoiseParams mTex2Params[3];

	NoiseGenerator* mNoiseGenerator;
	std::unique_ptr<GLProgram> mRayMarchProgram;

	std::unique_ptr<GLBuffer> mQuadBuffer;

	float mCloudScale = 0.002f;
	glm::vec3 mCloudOffset{ 0.0f };
	float mDensityMultiplier = 0.157f;
	float mDensityThreshold = 0.913f;
	glm::vec3 mLightDirection = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec4 mLightColor = glm::vec4(1.0f, 1.0f, 1.0f, 30.0f);
	glm::vec4 mLayerContribution = glm::vec4(1.0f, 0.625, 0.112, 0.938);
	float mPhaseG = 0.5f;
	float mLightAbsorption = 0.2f;
	bool mSugarPowder = true;

	unsigned int mGpuQuery;
	float mRenderTime = 0.0f;

	glm::vec2 mRadius{ 1500.0f, 4000.0f };

};