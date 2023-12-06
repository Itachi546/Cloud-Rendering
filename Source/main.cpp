#include "gl-utils.h"
#include "imgui-service.h"
#include "logger.h"
#include "cloud-generator.h"
#include "debug-draw.h"
#include "utils.h"

#include <iostream>

struct WindowProps {
	GLFWwindow* window;
	int width;
	int height;

	float mouseX = 0.0f;
	float mouseY = 0.0f;

	float mDx = 0.0f;
	float mDy = 0.0f;

	glm::mat4 P;
	glm::mat4 V;

	bool mouseDown = false;
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

static void on_mouse_move(GLFWwindow* window, double mouseX, double mouseY) {
	float x = static_cast<float>(mouseX);
	float y = static_cast<float>(mouseY);
	gWindowProps.mDx = x - gWindowProps.mouseX;
	gWindowProps.mDy = y - gWindowProps.mouseY;

	gWindowProps.mouseX = x;
	gWindowProps.mouseY = y;
}

static void on_mouse_down(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_RELEASE) {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gWindowProps.mouseDown = false;
	}
	else {
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			gWindowProps.mouseDown = true;
	}
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

static glm::vec3 UpdateViewMatrix(float camDist, float yaw, float pitch) {
	glm::vec3 target = glm::vec3(0.0f);
	glm::vec3 camPos = glm::vec3{ cos(pitch) * cos(yaw), sin(pitch), cos(pitch) * sin(yaw)} *camDist;

	glm::vec3 forward = glm::normalize(target - camPos);
	glm::vec3 right = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward);
	glm::vec3 up = glm::cross(forward, right);
	gWindowProps.V = glm::lookAt(camPos, target, up);
	return camPos;
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
	glfwSetCursorPosCallback(window, on_mouse_move);
	glfwSetMouseButtonCallback(window, on_mouse_down);
	glfwSwapInterval(1);
	glfwMakeContextCurrent(window);


	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == -1) {
		std::cerr << "Failed to initialize OpenGL" << std::endl;
		return -1;
	}

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

	float camDist = 8.0f;
	float cameraPitch = glm::radians(45.0f);
	float cameraYaw = 0.0f;
	gWindowProps.P = glm::perspective(glm::radians(60.0f), float(gFBOWidth) / float(gFBOHeight), 0.3f, 100.0f);
	glm::vec3 camPos = UpdateViewMatrix(camDist, cameraYaw, cameraPitch);
	glm::mat4 VP = gWindowProps.P * gWindowProps.V;

	float startTime = (float)glfwGetTime();
	float dt = 1.0f / 60.0f;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		ImGuiService::NewFrame();

		ImGuiService::RenderDockSpace();

		if (gWindowProps.mouseDown) {
			cameraYaw += gWindowProps.mDx * dt * 0.1f;
			cameraPitch += gWindowProps.mDy * dt * 0.1f;
			cameraPitch = glm::clamp(cameraPitch, -glm::pi<float>() * 0.25f, glm::pi<float>() * 0.25f);
			camPos = UpdateViewMatrix(camDist, cameraYaw, cameraPitch);
			VP = gWindowProps.P * gWindowProps.V;
		}

		mainFBO.bind();
		mainFBO.setClearColor(0.5f, 0.7f, 1.0f, 1.0f);
		mainFBO.setViewport(gFBOWidth, gFBOHeight);
		mainFBO.clear(true);
		cloudGenerator->Render(gWindowProps.P, gWindowProps.V, camPos, dt);
		DebugDraw::Render(VP, glm::vec2(gFBOWidth, gFBOHeight));
		mainFBO.unbind();

		ImGui::Begin("MainWindow");

		ImVec2 dims = ImGui::GetContentRegionAvail();
		ImVec2 pos = ImGui::GetCursorScreenPos();

		ImGui::GetWindowDrawList()->AddImage(
			(ImTextureID)(uint64_t)mainFBO.attachments[0],
			ImVec2(pos.x, pos.y),
			ImVec2(pos.x + dims.x, pos.y + dims.y),
			ImVec2(0, 1),
			ImVec2(1, 0));
		ImGui::End();

		ImGui::Begin("Logs");
		std::vector<std::string>& logs = logger::GetAllLogs();
		for(auto& log : logs)
			ImGui::Text("%s", log.c_str());
		ImGui::End();

		ImGui::Begin("Options");
		cloudGenerator->AddUI();
		ImGui::End();

		ImGuiService::Render(window);

		glfwSwapBuffers(window);

		float endTime = (float)glfwGetTime();
		dt = endTime - startTime;
		startTime = endTime;

		gWindowProps.mDx = 0.0f;
		gWindowProps.mDy = 0.0f;
	}
	DebugDraw::Shutdown();
	NoiseGenerator::GetInstance()->Shutdown();
    ImGuiService::Shutdown();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}