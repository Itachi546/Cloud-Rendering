#pragma once

#include "../glm-includes.h"

#include <memory>

enum class NoiseType {
	Perlin = 0,
	Worley
};

struct NoiseParams {
	float amplitude;
	float frequency;
	float lacunarity;
	float persistence;
	int numOctaves;

	glm::vec3 offset;
	NoiseType noiseType = NoiseType::Worley;
};

struct GLTexture;
class GLComputeProgram;

class NoiseGenerator {

public:
	NoiseGenerator(const NoiseGenerator&) = delete;
	void operator=(const NoiseGenerator&) = delete;

	static NoiseGenerator* GetInstance() {
		static NoiseGenerator* noise = new NoiseGenerator();
		return noise;
	}

	void Initialize();

	// channel is used to defined the generation of noise in a specific channel only
	// 0 - red, 1 - green, 2 - blue, 3 - alpha
	void Generate(const NoiseParams* params, const GLTexture* texture, int channel = 0);

	void Shutdown();
private:

	void Generate(const NoiseParams* param, const GLTexture* texture, std::unique_ptr<GLComputeProgram>& shader, int channel = 0);

	NoiseGenerator() = default;

	std::unique_ptr<GLComputeProgram> mWorleyShader3D;
	std::unique_ptr<GLComputeProgram> mPerlinShader3D;
};
	
