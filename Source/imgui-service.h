#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/glad.h>
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

	static ImGuiID gDockSpaceId = 0;
	static bool gFirstFrame = true;

	void Initialize(GLFWwindow* window) {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init();
	}

    // https://gist.github.com/AidanSun05/953f1048ffe5699800d2c92b88c36d9f
	static void SetEditorLayoutPreset() {
		ImVec2 workCenter = ImGui::GetMainViewport()->GetCenter();
		ImGui::DockBuilderRemoveNode(gDockSpaceId);
		ImGui::DockBuilderAddNode(gDockSpaceId);

		ImGuiID mainWindowDoc = ImGui::DockBuilderSplitNode(gDockSpaceId, ImGuiDir_Left, 0.85f, nullptr, &gDockSpaceId);
		ImGuiID optionWindowDoc = ImGui::DockBuilderSplitNode(gDockSpaceId, ImGuiDir_Right, 0.1f, nullptr, &gDockSpaceId);
		ImGuiID logDoc = ImGui::DockBuilderSplitNode(mainWindowDoc, ImGuiDir_Down, 0.2f, nullptr, &mainWindowDoc);

		ImGui::DockBuilderDockWindow(gCustomWindowName[int(ImGuiCustomWindow::MainWindow)], mainWindowDoc);
		ImGui::DockBuilderDockWindow(gCustomWindowName[int(ImGuiCustomWindow::Options)], optionWindowDoc);
		ImGui::DockBuilderDockWindow(gCustomWindowName[int(ImGuiCustomWindow::Logs)], logDoc);
		ImGui::DockBuilderFinish(gDockSpaceId);
	}

	void NewFrame() {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void RenderDockSpace() {
		gDockSpaceId = ImGui::DockSpaceOverViewport();
		if (gFirstFrame) {
			SetEditorLayoutPreset();
			gFirstFrame = false;
		}
	}

	void RenderGameWindow() {
		ImGui::Begin("Options");
		ImGui::End();

		
	}


	void Render(GLFWwindow* window) {
		ImGui::Render();

		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		// Update and Render additional Platform Windows
		// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
		//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}

	void Shutdown() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
};