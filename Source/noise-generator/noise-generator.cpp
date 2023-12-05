#include "noise-generator.h"

#include "../gl-utils.h"
#include "../glm-includes.h"

void NoiseGenerator::Initialize()
{
	{
		GLShader shader("Shaders/worley.comp");

		mWorleyShader3D = std::make_unique<GLComputeProgram>();
		mWorleyShader3D->init(shader);
	}
}

void NoiseGenerator::GenerateWorley3D(const NoiseParams* params, const GLTexture* texture, int channel)
{
	assert(params != nullptr);
	assert(texture != nullptr);

	mWorleyShader3D->use();

	uint32_t workGroupX = (texture->width + 7) / 8;
	uint32_t workGroupY = (texture->height + 7) / 8;
	uint32_t workGroupZ = (texture->depth + 7) / 8;

	glm::vec4 amp_freq_lac_per{params->amplitude, params->frequency, params->lacunarity, params->persistence};
	mWorleyShader3D->setVec4("uAmp_Freq_Lac_Per", &amp_freq_lac_per[0]);
	mWorleyShader3D->setInt("uNumOctaves", params->numOctaves);

	glm::vec4 offsetAndChannel{ params->offset, channel };
	mWorleyShader3D->setVec4("uOffsetAndChannel", &offsetAndChannel[0]);


	glm::vec3 textureSize = {
		(float)texture->width,
		(float)texture->height,
		(float)texture->depth
	};
	mWorleyShader3D->setVec3("uImageSize", &textureSize[0]);

	mWorleyShader3D->setTexture(0, texture->handle, GL_READ_WRITE, texture->internalFormat, true);

	glDispatchCompute(workGroupX, workGroupY, workGroupZ);
}
