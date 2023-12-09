#include "terrain.h"

#include "gl-utils.h"
#include "utils.h"
#include "camera.h"

void Terrain::Initialize(uint32_t width, uint32_t height)
{
	mWidth = width;
	mHeight = height;

	std::vector<glm::vec2> vertices;
	for (uint32_t y = 0; y < height; ++y) {
		for (uint32_t x = 0; x < width; ++x) {
			vertices.push_back({ x, y });
		}
	}
	mVBO = std::make_unique<GLBuffer>();
	mVBO->init(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(glm::vec2)), 0);

	std::vector<uint32_t> indices;
	for (uint32_t i = 0; i < height - 1; ++i) {
		for (uint32_t j = 0; j < width - 1; ++j) {
			uint32_t p0 = i * width + j;
			uint32_t p1 = p0 + 1;
			uint32_t p2 = (i + 1) * width + j;
			uint32_t p3 = p2 + 1;
			indices.push_back(p2);
			indices.push_back(p1);
			indices.push_back(p0);

			indices.push_back(p2);
			indices.push_back(p3);
			indices.push_back(p1);
		}
	}

	mIBO = std::make_unique<GLBuffer>();
	mIBO->init(indices.data(), static_cast<uint32_t>(indices.size() * sizeof(uint32_t)), 0);
	mNumIndices = static_cast<uint32_t>(indices.size());

	glGenVertexArrays(1, &mVAO);
	
	GLShader vs("Shaders/terrain.vert");
	GLShader fs("Shaders/terrain.frag");
	mProgram = std::make_unique<GLProgram>();
	mProgram->init(vs, fs);

	{
		int texWidth, texHeight, nChannel;
		float* heightData = Utils::LoadImageFloat("Textures/terrain-height.png", &texWidth, &texHeight, &nChannel);

		TextureCreateInfo createInfo = { (uint32_t)texWidth, (uint32_t)texHeight, 1, GL_RED, GL_R16F, GL_TEXTURE_2D, GL_FLOAT };
		mHeightTexture = std::make_unique<GLTexture>();
		mHeightTexture->init(&createInfo, heightData);
		Utils::FreeImage(heightData);
	}
	{
		int texWidth, texHeight, nChannel;
		unsigned char* colorData = Utils::LoadImage("Textures/terrain-diffuse.png", &texWidth, &texHeight, &nChannel);
		TextureCreateInfo createInfo = { (uint32_t)texWidth, (uint32_t)texHeight, 1, GL_RGBA, GL_RGBA8, GL_TEXTURE_2D, GL_UNSIGNED_BYTE };
		mDiffuseTexture = std::make_unique<GLTexture>();
		mDiffuseTexture->init(&createInfo, colorData);
		Utils::FreeImage(colorData); 
	}
}

void Terrain::Render(Camera* camera)
{
	glm::mat4 M = glm::translate(glm::mat4(1.0f), -glm::vec3(mWidth * 0.5f, 0.0f, mHeight * 0.5f));
	glm::mat4 VP = camera->GetProjectionMatrix() * camera->GetViewMatrix() * M;
	mProgram->use();
	mProgram->setTexture("uHeightMap", 0, mHeightTexture->handle);
	mProgram->setTexture("uDiffuseMap", 1, mDiffuseTexture->handle);
	mProgram->setMat4("uVP", &VP[0][0]);

	glm::vec2 invSize{ 1.0f / float(mWidth), 1.0f / float(mHeight) };
	mProgram->setVec2("uInvTerrainSize", &invSize[0]);

	glBindVertexArray(mVAO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO->handle);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO->handle);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 0);

	glDrawElements(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, 0);
}

void Terrain::Shutdown()
{
	mHeightTexture->destroy();
	mDiffuseTexture->destroy();
	mVBO->destroy();
	mIBO->destroy();
	mProgram->destroy();
}
