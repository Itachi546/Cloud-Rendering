#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "glm-includes.h"
#include "gl-utils.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <GLFW/glfw3.h>

#include <vector>
#include <sstream>
#include <chrono>
#include <ctime>

struct GLFWwindow;

enum class ImGuiCustomWindow {
	MainWindow = 0,
	Options = 1,
	Logs = 2,
};

static const char* gCustomWindowName[] = {
	"MainWindow",
	"Options",
	"Logs"
};

namespace ImGuiService {
	void Initialize(GLFWwindow* window);

	void NewFrame();

	void RenderDockSpace();

	void Image3D(ImTextureID user_texture_id,
		const ImVec2& size,
		float layer = 0,
		const ImVec4& border_col = { 0.5f, 0.5f, 0.5f, 1.0f });

	void Render(GLFWwindow* window);

	void Shutdown();
};