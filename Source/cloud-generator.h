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

	void Render(glm::mat4 P, glm::mat4 V, glm::vec3 camPos);

	void Shutdown();

	glm::vec3 aabbMin;
	glm::vec3 aabbMax;
private:
	std::unique_ptr<GLTexture> mTexture1;
	std::unique_ptr<GLTexture> mTexture2;

	NoiseParams mTex1Params[4];
	NoiseParams mTex2Params[3];

	NoiseGenerator* mNoiseGenerator;
	std::unique_ptr<GLProgram> mRayMarchProgram;

	std::unique_ptr<GLBuffer> mQuadBuffer;
};