#pragma once

#include <memory>

#include "glm-includes.h"

struct GLBuffer;
class GLProgram;
class GLComputeProgram;
struct GLTexture;
class Camera;

class Terrain {

public:
	void Initialize(uint32_t width, uint32_t height);

	void Render(Camera* camera);

	void Shutdown();

private:
	std::unique_ptr<GLBuffer> mVBO;
	std::unique_ptr<GLBuffer> mIBO;
	unsigned int mVAO;

	std::unique_ptr<GLProgram> mProgram;
	std::unique_ptr<GLTexture> mHeightTexture;
	std::unique_ptr<GLTexture> mDiffuseTexture;

	uint32_t mWidth, mHeight;
	uint32_t mNumIndices;
};