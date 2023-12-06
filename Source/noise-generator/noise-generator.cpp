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
	{
		GLShader shader("Shaders/perlin.comp");
		mPerlinShader3D = std::make_unique<GLComputeProgram>();
		mPerlinShader3D->init(shader);
	}

}

void NoiseGenerator::Generate(const NoiseParams* params, const GLTexture* texture, int channel)
{
	switch (params->noiseType) {
	case NoiseType::Worley:
		Generate(params, texture, mWorleyShader3D, channel);
		break;
	case NoiseType::Perlin:
		Generate(params, texture, mPerlinShader3D, channel);
		break;
	}
}

void NoiseGenerator::Shutdown()
{
	mWorleyShader3D->destroy();
	mPerlinShader3D->destroy();
}

void NoiseGenerator::Generate(const NoiseParams* params, const GLTexture* texture, std::unique_ptr<GLComputeProgram>& shader, int channel)
{
	assert(params != nullptr);
	assert(texture != nullptr);

	shader->use();

	glm::vec4 amp_freq_lac_per{ params->amplitude, params->frequency, params->lacunarity, params->persistence };
	shader->setVec4("uAmp_Freq_Lac_Per", &amp_freq_lac_per[0]);
	shader->setInt("uNumOctaves", params->numOctaves);

	glm::vec4 offsetAndChannel{ params->offset, channel };
	shader->setVec4("uOffsetAndChannel", &offsetAndChannel[0]);


	glm::vec3 textureSize = {
		(float)texture->width,
		(float)texture->height,
		(float)texture->depth
	};
	shader->setVec3("uImageSize", &textureSize[0]);

	shader->setTexture(0, texture->handle, GL_READ_WRITE, texture->internalFormat, true);

	uint32_t workGroupX = (texture->width + 7) / 8;
	uint32_t workGroupY = (texture->height + 7) / 8;
	uint32_t workGroupZ = (texture->depth + 7) / 8;
	glDispatchCompute(workGroupX, workGroupY, workGroupZ);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
