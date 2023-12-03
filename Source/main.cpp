#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"

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

GLTexture initialize3DTexture() {
	TextureCreateInfo createInfo = {
		256, 256, 256, GL_RED,
		GL_R32F,
		GL_TEXTURE_3D,
		GL_FLOAT
	};

	GLTexture texture;
	texture.init(&createInfo);

	GLShader shader("Shaders/test.comp");
	GLComputeProgram program;
	program.init(shader);

	program.use();
	program.setTexture(0, texture.handle, GL_READ_WRITE, GL_R32F);

	uint32_t workGroupSize = 256 / 8;
	program.dispatch(workGroupSize, workGroupSize, workGroupSize);

	return texture;
}

int main() {

	if(!glfwInit()) return 1;
	glfwWindowHint(GLFW_SAMPLES, 4);

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

	GLTexture layeredTexture = initialize3DTexture();

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		mainFBO.bind();
		mainFBO.setClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		mainFBO.setViewport(gFBOWidth, gFBOHeight);
		mainFBO.clear(true);
		mainFBO.unbind();

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
		ImGui::Text("Noise Texture");

		ImGui::PushID(1);
		static int layer1 = 0;
		ImGui::DragInt("Layer", &layer1, 0.1f, 0, 255);
		ImGuiService::Image3D((ImTextureID)(uint64_t)layeredTexture.handle, ImVec2{ 256.0f, 256.0f }, float(layer1) / 255.0f);
		ImGui::Separator();
		ImGui::PopID();

		ImGui::PushID(2);
		static int layer2 = 0;
		ImGui::Text("Noise Texture 2");
		ImGui::DragInt("Layer", &layer2, 0.1f, 0, 255);
		ImGuiService::Image3D((ImTextureID)(uint64_t)layeredTexture.handle, ImVec2{ 256.0f, 256.0f }, float(layer2) / 255.0f);
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