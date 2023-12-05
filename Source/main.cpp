#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"
#include "noise-generator/noise-generator.h"

#include <iostream>

struct WindowProps {
	GLFWwindow* window;
	int width;
	int height;
};

WindowProps gWindowProps = {
	nullptr, 1360, 769
};

uint32_t gFBOWidth = 1920;
uint32_t gFBOHeight = 1080;

static void on_window_resize(GLFWwindow* window, int width, int height) {
	gWindowProps.width = std::max(width, 2);
	gWindowProps.height = std::max(height, 2);
}

static void on_key_press(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
		glfwSetWindowShouldClose(window, true);
}

void GLAPIENTRY
MessageCallback(GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	if (type != GL_DEBUG_TYPE_ERROR) return;
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

GLTexture create3DTexture() {
	TextureCreateInfo createInfo = {
		128, 128, 128, GL_RGBA,
		GL_RGBA32F,
		GL_TEXTURE_3D,
		GL_FLOAT
	};

	GLTexture texture;
	texture.init(&createInfo);
	return texture;
}

void SelectableTexture3D(GLuint textureHandle, float* layer, int* channel) {
	ImGui::SliderFloat("Layer", layer, 0.0f, 1.0f);
	ImGui::Combo("Channel", channel, "R\0G\0B\0A\0");
	ImGuiService::Image3D((ImTextureID)(uint64_t)textureHandle, ImVec2{ 256.0f, 256.0f }, *layer, *channel);
}

bool CreateNoiseWidget(const char* name, NoiseParams* params) {
	bool changed = false;   
	if(ImGui::CollapsingHeader(name)) {
		changed = ImGui::SliderFloat("Amplitude", &params->amplitude, 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Frequency", &params->frequency, 0.0f, 10.0f);
		changed |= ImGui::SliderFloat("Lacunarity", &params->lacunarity, 0.0f, 2.0f);
		changed |= ImGui::SliderFloat("Persitence", &params->persistence, 0.0f, 1.0f);
		changed |= ImGui::SliderInt("Octave", &params->numOctaves, 1, 8);
		changed |= ImGui::DragFloat3("Offset", &params->offset[0], 0.1f);
	}
	return changed;
}

int main() {

	if(!glfwInit()) return 1;

	GLFWwindow* window = glfwCreateWindow(gWindowProps.width, gWindowProps.height, "Hello OpenGL", 0, 0);
	gWindowProps.window = window;

	if (window == nullptr)
	{
		std::cerr << "Failed to create Window" << std::endl;
		return 1;
	}
	logger::Debug("Initialized GLFW Window ...");

	glfwSetWindowSizeCallback(window, on_window_resize);
	glfwSetKeyCallback(window, on_key_press);
	glfwMakeContextCurrent(window);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	logger::Debug("Initialized GLAD ...");

	glEnable(GL_DEBUG_OUTPUT);
	logger::Debug("Enabled Debug Callback ...");
	glDebugMessageCallback(MessageCallback, 0);

	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	logger::Debug("Renderer: " + std::string(renderer));
	logger::Debug("Vendor: " + std::string(vendor));

	glEnable(GL_DEPTH_TEST);
	logger::Debug("Enabled Depth Test ...");

    ImGuiService::Initialize(window);

	TextureCreateInfo colorAttachment = {gFBOWidth, gFBOHeight};
	TextureCreateInfo depthAttachment;
	InitializeDepthTexture(&depthAttachment, gFBOWidth, gFBOHeight, GL_DEPTH_COMPONENT32F);

	GLFramebuffer mainFBO;
	mainFBO.init({ Attachment{ 0, &colorAttachment } }, nullptr);

	GLTexture layeredTexture = create3DTexture();

	NoiseGenerator* noiseGenerator = NoiseGenerator::GetInstance();
	noiseGenerator->Initialize();

	NoiseParams worleyParams[4];
	worleyParams[0] = { 0.5f, 2.0f, 2.0f, 0.5f, 2, glm::vec3(0.0f) };
	worleyParams[1] = { 0.5f, 4.0f, 2.0f, 0.5f, 4, glm::vec3(1.0f) };
	worleyParams[2] = { 0.5f, 8.0f, 2.0f, 0.5f, 6, glm::vec3(1.0f) };
	worleyParams[3] = { 0.5f, 8.0f, 2.0f, 0.5f, 6, glm::vec3(1.0f) };

	noiseGenerator->GenerateWorley3D(&worleyParams[0], &layeredTexture, 0);
	noiseGenerator->GenerateWorley3D(&worleyParams[1], &layeredTexture, 1);
	noiseGenerator->GenerateWorley3D(&worleyParams[2], &layeredTexture, 2);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		mainFBO.bind();
		mainFBO.setClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		mainFBO.setViewport(gFBOWidth, gFBOHeight);
		mainFBO.clear(true);
		mainFBO.unbind();

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		ImGui::Begin("MainWindow");
		const float windowWidth = ImGui::GetContentRegionAvail().x;
		const float windowHeight = ImGui::GetContentRegionAvail().y;
		ImGui::Image((ImTextureID)((uint64_t)mainFBO.attachments[0]), ImVec2{ windowWidth, windowHeight });
		ImGui::End();

		ImGui::Begin("Logs");
		std::vector<std::string>& logs = logger::GetAllLogs();
		for(auto& log : logs)
			ImGui::Text("%s", log.c_str());
		ImGui::End();

		ImGui::Begin("Options");

		ImGui::PushID(1);
		ImGui::Text("Noise Texture");
		static float layer = 0;
		static int channel = 0;
		SelectableTexture3D(layeredTexture.handle, &layer, &channel);
		if (CreateNoiseWidget("Noise Params", &worleyParams[channel])) {
			noiseGenerator->GenerateWorley3D(&worleyParams[channel], &layeredTexture, channel);
		}

		ImGui::Separator();
		ImGui::PopID();

		ImGui::End();

		ImGuiService::Render(window);

		glfwSwapBuffers(window);
	}

    ImGuiService::Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}