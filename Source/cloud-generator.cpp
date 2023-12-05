#include "cloud-generator.h"

#include "gl-utils.h"
#include "imgui-service.h"
#include "debug-draw.h"

void CloudGenerator::Initialize()
{
	TextureCreateInfo createInfo = {
	128, 128, 128, GL_RGBA,
	GL_RGBA32F,
	GL_TEXTURE_3D,
	GL_FLOAT
	};

	mTexture1 = std::make_unique<GLTexture>();
	mTexture1->init(&createInfo);

	createInfo.width = createInfo.height = createInfo.depth = 32;
	mTexture2 = std::make_unique<GLTexture>();
	mTexture2->init(&createInfo);

	mTex1Params[0] = { 0.5f, 2.0f, 2.0f, 0.5f, 1, glm::vec3(0.4f, 0.6, 0.5) };
	mTex1Params[1] = { 0.5f, 4.0f, 2.0f, 0.5f, 2, glm::vec3(1.4f, 1.593f, 1.539f) };
	mTex1Params[2] = { 0.5f, 8.0f, 2.0f, 0.5f, 4, glm::vec3(2.8f, 2.99f, 2.48f) };
	mTex1Params[3] = { 0.5f, 8.0f, 2.0f, 0.5f, 6, glm::vec3(4.8f, 5.f, 5.43f) };

	mNoiseGenerator = NoiseGenerator::GetInstance();
	mNoiseGenerator->GenerateWorley3D(&mTex1Params[0], mTexture1.get(), 0);
	mNoiseGenerator->GenerateWorley3D(&mTex1Params[1], mTexture1.get(), 1);
	mNoiseGenerator->GenerateWorley3D(&mTex1Params[2], mTexture1.get(), 2);
	mNoiseGenerator->GenerateWorley3D(&mTex1Params[3], mTexture1.get(), 3);

	mTex2Params[0] = { 0.5f, 4.0f, 2.0f, 0.5f, 1, glm::vec3(29.4f, 25.6, 27.5) };
	mTex2Params[1] = { 0.5f, 5.0f, 2.0f, 0.5f, 2, glm::vec3(35.4f, 30.593f, 39.539f) };
	mTex2Params[2] = { 0.5f, 6.0f, 2.0f, 0.5f, 4, glm::vec3(40.8f, 44.99f, 45.48f) };
	mNoiseGenerator->GenerateWorley3D(&mTex2Params[0], mTexture2.get(), 0);
	mNoiseGenerator->GenerateWorley3D(&mTex2Params[1], mTexture2.get(), 1);
	mNoiseGenerator->GenerateWorley3D(&mTex2Params[2], mTexture2.get(), 2);

	aabbMin = { -2.0f, -1.5f, -2.0f };
	aabbMax = { 2.0f, 1.5f, 2.0f };
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
	ImGui::Text("Bounding Box");
	ImGui::DragFloat3("Min", &aabbMin[0], 0.1f);
	ImGui::DragFloat3("Max", &aabbMax[0], 0.1f);
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Noise Texture1")) {
		ImGui::PushID(1);
		static float layer1 = 0;
		static int channel1 = 0;
		SelectableTexture3D(mTexture1->handle, ImVec2{256, 256.0f}, &layer1, &channel1, 4);
		if (CreateNoiseWidget("Noise Params", &mTex1Params[channel1])) {
			mNoiseGenerator->GenerateWorley3D(&mTex1Params[channel1], mTexture1.get(), channel1);
		}
		ImGui::PopID();
		ImGui::Separator();
	}


	if (ImGui::CollapsingHeader("Noise Texture2")) {
		ImGui::PushID(2);
		static float layer2 = 0;
		static int channel2 = 0;
		SelectableTexture3D(mTexture2->handle, ImVec2{64.0f, 64.0f}, &layer2, &channel2, 3);
		if (CreateNoiseWidget("Noise Params", &mTex2Params[channel2]))
			mNoiseGenerator->GenerateWorley3D(&mTex2Params[channel2], mTexture2.get(), channel2);
		ImGui::PopID();
		ImGui::Separator();
	}
}

void CloudGenerator::Render()
{
	DebugDraw::AddRect(aabbMin, aabbMax);
}

void CloudGenerator::Shutdown()
{
	mTexture1->destroy();
	mTexture2->destroy();
}
