#include "cloud-generator.h"

#include "gl-utils.h"
#include "imgui-service.h"
#include "debug-draw.h"
#include "logger.h"
#include "utils.h"
#include "camera.h"

void CloudGenerator::Initialize()
{
	TextureCreateInfo createInfo = {
	128, 128, 128, GL_RGBA,
	GL_RGBA32F,
	GL_TEXTURE_3D,
	GL_FLOAT
	};
	createInfo.wrapType = GL_REPEAT;

	mTexture1 = std::make_unique<GLTexture>();
	mTexture1->init(&createInfo);

	createInfo.width = createInfo.height = createInfo.depth = 32;
	mTexture2 = std::make_unique<GLTexture>();
	mTexture2->init(&createInfo);

	mBlueNoiseTex = std::make_unique<GLTexture>();
	int width, height, nChannel;
	unsigned char* noiseData = Utils::LoadImage("Textures/BlueNoise64.png", &width, &height, &nChannel);
	createInfo.width = width;
	createInfo.height = height;
	createInfo.internalFormat = GL_RGBA8;
	createInfo.dataType = GL_UNSIGNED_BYTE;
	createInfo.target = GL_TEXTURE_2D;
	mBlueNoiseTex->init(&createInfo, noiseData);

	Utils::FreeImage(noiseData);

	mTex1Params[0] = { 1.0f, 1.0f, 1.0f, 0.5f, 7, glm::vec3(0.4f, 0.6, 0.5), NoiseType::Perlin };
	mTex1Params[1] = { 0.5f, 4.0f, 2.0f, 0.5f, 2, glm::vec3(1.4f, 1.593f, 1.539f) };
	mTex1Params[2] = { 0.5f, 8.0f, 2.0f, 0.5f, 4, glm::vec3(2.8f, 2.99f, 2.48f) };
	mTex1Params[3] = { 0.5f, 8.0f, 2.0f, 0.5f, 6, glm::vec3(4.8f, 5.f, 5.43f) };

	mNoiseGenerator = NoiseGenerator::GetInstance();
	mNoiseGenerator->Generate(&mTex1Params[0], mTexture1.get(), 0);
	mNoiseGenerator->Generate(&mTex1Params[1], mTexture1.get(), 1);
	mNoiseGenerator->Generate(&mTex1Params[2], mTexture1.get(), 2);
	mNoiseGenerator->Generate(&mTex1Params[3], mTexture1.get(), 3);

	mTex2Params[0] = { 0.5f, 4.0f, 2.0f, 0.5f, 1, glm::vec3(29.4f, 25.6, 27.5) };
	mTex2Params[1] = { 0.5f, 5.0f, 2.0f, 0.5f, 2, glm::vec3(35.4f, 30.593f, 39.539f) };
	mTex2Params[2] = { 0.5f, 6.0f, 2.0f, 0.5f, 4, glm::vec3(40.8f, 44.99f, 45.48f) };
	mNoiseGenerator->Generate(&mTex2Params[0], mTexture2.get(), 0);
	mNoiseGenerator->Generate(&mTex2Params[1], mTexture2.get(), 1);
	mNoiseGenerator->Generate(&mTex2Params[2], mTexture2.get(), 2);

	GLShader rayMarchVS("Shaders/raymarch.vert");
	GLShader rayMarchFS("Shaders/raymarch.frag");
	mRayMarchProgram = std::make_unique<GLProgram>();
	mRayMarchProgram->init(rayMarchVS, rayMarchFS);

	std::vector<glm::vec2> positions = {
		glm::vec2(-1.0f, -1.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(-1.0f, 1.0f),
		glm::vec2(-1.0f, -1.0f),
		glm::vec2(1.0f, 1.0f),
		glm::vec2(1.0f, -1.0f)
	};
	
	mQuadBuffer = std::make_unique<GLBuffer>();
	uint32_t dataSize = static_cast<uint32_t>(positions.size() * sizeof(glm::vec2));
	mQuadBuffer->init(positions.data(), dataSize, 0);

	glGenQueries(1, &mGpuQuery);
}

static const char* CHANNELS_DROPDOWN[] = {
	"R\0",
	"R\0G\0",
	"R\0G\0B\0",
	"R\0G\0B\0A\0"
};

void SelectableTexture3D(GLuint textureHandle, const ImVec2& size, float* layer, int* channel, int numChannel) {
	ImGui::SliderFloat("Layer", layer, 0.0f, 1.0f);
	ImGui::Combo("Channel", channel, CHANNELS_DROPDOWN[numChannel-1]);
	ImGuiService::Image3D((ImTextureID)(uint64_t)textureHandle, size, *layer, *channel);
}

static bool CreateNoiseWidget(const char* name, NoiseParams* params) {
	bool changed = false;
	changed = ImGui::SliderFloat("Amplitude", &params->amplitude, 0.0f, 1.0f);
	changed |= ImGui::SliderFloat("Frequency", &params->frequency, 0.0f, 10.0f);
	changed |= ImGui::SliderFloat("Lacunarity", &params->lacunarity, 0.0f, 2.0f);
	changed |= ImGui::SliderFloat("Persitence", &params->persistence, 0.0f, 1.0f);
	changed |= ImGui::SliderInt("Octave", &params->numOctaves, 1, 8);
	changed |= ImGui::DragFloat3("Offset", &params->offset[0], 0.1f);
	return changed;
}

void CloudGenerator::AddUI()
{
	ImGui::Text("Render Time: %.2fms", mRenderTime);
	ImGui::DragFloat2("Radius", &mRadius[0], 10.0f);
	ImGui::Spacing();

	ImGui::SliderFloat("CloudScale", &mCloudScale, 0.0f, 0.01f);
	ImGui::DragFloat3("CloudOffset", &mCloudOffset[0], 0.1f);

	ImGui::SliderFloat("DensityMultiplier", &mDensityMultiplier, 0.0f, 1.0f);
	ImGui::SliderFloat("DensityThreshold", &mDensityThreshold, 0.0f, 1.0f);
	ImGui::SliderFloat4("Layer Contribution", &mLayerContribution[0], 0.0f, 1.0f);

	ImGui::ColorEdit3("Light Color", &mLightColor[0]);
	ImGui::DragFloat("Light Intensity", &mLightColor.w, 0.1f);
	ImGui::SliderFloat("PhaseG", &mPhaseG, 0.0f, 1.0f);
	ImGui::SliderFloat("Light Absorption", &mLightAbsorption, 0.0f, 4.0f);
	ImGui::Checkbox("Sugar Powder", &mSugarPowder);

	static float theta = glm::radians(90.0f), phi = 0.0f;
	ImGui::Text("Light Direction");
	bool changed = ImGui::SliderAngle("phi", &phi);
	changed |= ImGui::SliderAngle("theta", &theta);
	if (changed)
		mLightDirection = glm::vec3{ sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta) };

	ImGui::Spacing();
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Noise Texture1", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID(1);
		static float layer1 = 0;
		static int channel1 = 0;
		SelectableTexture3D(mTexture1->handle, ImVec2{256, 256.0f}, &layer1, &channel1, 4);
		if (CreateNoiseWidget("Noise Params", &mTex1Params[channel1])) {
			mNoiseGenerator->Generate(&mTex1Params[channel1], mTexture1.get(), channel1);
		}
		ImGui::PopID();
		ImGui::Separator();
	}


	if (ImGui::CollapsingHeader("Noise Texture2", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushID(2);
		static float layer2 = 0;
		static int channel2 = 0;
		SelectableTexture3D(mTexture2->handle, ImVec2{64.0f, 64.0f}, &layer2, &channel2, 3);
		if (CreateNoiseWidget("Noise Params", &mTex2Params[channel2]))
			mNoiseGenerator->Generate(&mTex2Params[channel2], mTexture2.get(), channel2);
		ImGui::PopID();
		ImGui::Separator();
	}

	ImGui::Text("Blue Noise Texture");
	ImGui::Image((ImTextureID)(uint64_t)mBlueNoiseTex->handle, ImVec2 { 64, 64 });

}

void CloudGenerator::Render(Camera* camera, float dt, uint32_t depthTexture, uint32_t colorAttachment)
{
	glm::mat4 invP = camera->GetInvProjectionMatrix();
	glm::mat4 invV = camera->GetInvViewMatrix();
	glm::vec3 camPos = camera->GetPosition();

	//mCloudOffset.x += dt * 0.1f;
	glBeginQuery(GL_TIME_ELAPSED, mGpuQuery);
	/**/
	glUseProgram(0);
	mRayMarchProgram->use();
	mRayMarchProgram->setVec3("mRadius", &mRadius[0]);
	mRayMarchProgram->setVec3("uCamPos", &camPos[0]);
	mRayMarchProgram->setMat4("uInvP", &invP[0][0]);
	mRayMarchProgram->setMat4("uInvV", &invV[0][0]);

	mRayMarchProgram->setTexture("uNoiseTex1", 0, mTexture1->handle, true);
	mRayMarchProgram->setTexture("uNoiseTex2", 1, mTexture2->handle, true);
	mRayMarchProgram->setTexture("uBlueNoiseTex", 2, mBlueNoiseTex->handle);
	mRayMarchProgram->setTexture("uDepthTexture", 3, depthTexture);
	mRayMarchProgram->setTexture("uSceneTexture", 4, colorAttachment);

	mRayMarchProgram->setVec3("uCloudOffset", &mCloudOffset[0]);
	mRayMarchProgram->setFloat("uCloudScale", mCloudScale);
	mRayMarchProgram->setFloat("uDensityMultiplier", mDensityMultiplier);
	mRayMarchProgram->setFloat("uDensityThreshold", mDensityThreshold);
	mRayMarchProgram->setVec3("uLightDirection", &mLightDirection[0]);
	mRayMarchProgram->setVec4("uLightColor", &mLightColor[0]);
	mRayMarchProgram->setVec4("uLayerContribution", &mLayerContribution[0]);
	mRayMarchProgram->setFloat("uPhaseG", mPhaseG);
	mRayMarchProgram->setFloat("uLightAbsorption", mLightAbsorption);
	mRayMarchProgram->setInt("uSugarPowder", int(mSugarPowder));

	glBindBuffer(GL_ARRAY_BUFFER, mQuadBuffer->handle);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);

	glEndQuery(GL_TIME_ELAPSED);
	// Wait for query to be available (stalling)
	int done = 0;
	while (!done)
		glGetQueryObjectiv(mGpuQuery, GL_QUERY_RESULT_AVAILABLE, &done);

	uint64_t renderTimeElapsed = 0;
	glGetQueryObjectui64v(mGpuQuery, GL_QUERY_RESULT, &renderTimeElapsed);
	mRenderTime = renderTimeElapsed * 0.000001f;
}

void CloudGenerator::Shutdown()
{
	mTexture1->destroy();
	mTexture2->destroy();
	mRayMarchProgram->destroy();
	mQuadBuffer->destroy();
}
