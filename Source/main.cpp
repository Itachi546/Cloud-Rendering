#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"
#include "cloud-generator.h"
#include "debug-draw.h"

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

	DebugDraw::Initialize();
	NoiseGenerator::GetInstance()->Initialize();
	std::unique_ptr<CloudGenerator> cloudGenerator = std::make_unique<CloudGenerator>();
	cloudGenerator->Initialize();
	float angle = 0.0f;
	glm::mat4 P = glm::perspective(glm::radians(60.0f), float(gFBOWidth) / float(gFBOHeight), 0.3f, 100.0f);
	glm::mat4 V = glm::lookAt(glm::vec3(0.0f, -4.0f, -4.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 VP = P * V * M;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		mainFBO.bind();
		mainFBO.setClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		mainFBO.setViewport(gFBOWidth, gFBOHeight);
		mainFBO.clear(true);
		cloudGenerator->Render();
		DebugDraw::Render(VP, glm::vec2(gFBOWidth, gFBOHeight));
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
		if (ImGui::SliderAngle("Rotation", &angle)) {
			M = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
			VP = P * V * M;
		}

		cloudGenerator->AddUI();
		ImGui::End();

		ImGuiService::Render(window);

		glfwSwapBuffers(window);
	}
	DebugDraw::Shutdown();
	NoiseGenerator::GetInstance()->Initialize();
    ImGuiService::Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}