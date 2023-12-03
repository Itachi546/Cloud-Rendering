#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include "glm-includes.h"

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
	static GLProgram gDraw3DTextureProgram;
	static glm::mat4 gProjectionMatrix;

	static void Initialize3DTextureProgram() {
		const char* vsCode = "#version 410\n"
			"layout (location = 0) in vec2 Position;\n"
			"layout (location = 1) in vec2 UV;\n"
			"layout (location = 2) in vec4 Color;\n"
			"uniform mat4 ProjMtx;\n"
			"out vec2 Frag_UV;\n"
			"out vec4 Frag_Color;\n"
			"void main()\n"
			"{\n"
			"    Frag_UV = UV;\n"
			"    Frag_Color = Color;\n"
			"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
			"}\n";
		const char* fsCode =   "#version 410\n"
		"in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler3D Texture;\n"
		"uniform float Layer;\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * vec4(texture(Texture, vec3(Frag_UV.st, Layer)).rrr, 1.0f);\n"
			"}\n";

		GLShader vs(GL_VERTEX_SHADER, vsCode);
		GLShader fs(GL_FRAGMENT_SHADER, fsCode);

		gDraw3DTextureProgram.init(vs, fs);
	}

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

		Initialize3DTextureProgram();
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

		ImGuiIO& io = ImGui::GetIO();
		ImVec2 size = io.DisplaySize;
		ImVec2 pos = ImGui::GetMainViewport()->Pos;
		gProjectionMatrix = glm::ortho(pos.x, pos.x + size.x, pos.y + size.y, pos.y);
	}

	void RenderDockSpace() {
		gDockSpaceId = ImGui::DockSpaceOverViewport();
		if (gFirstFrame) {
			SetEditorLayoutPreset();
			gFirstFrame = false;
		}
	}

	static void BeginDraw3DTex(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
		gDraw3DTextureProgram.use();
		float layer = *reinterpret_cast<float*>(cmd->UserCallbackData);

		uint32_t textureId = (uint32_t)(uint64_t)cmd->TextureId;

		glBindTexture(GL_TEXTURE_3D, textureId);
		gDraw3DTextureProgram.setFloat("Layer", layer);
		gDraw3DTextureProgram.setMat4("ProjMtx", &gProjectionMatrix[0][0]);
	}

	void Image3D(ImTextureID user_texture_id,
		const ImVec2& size,
		float layer = 0,
		const ImVec2& uv0 = {0.0f, 0.0f},
		const ImVec2& uv1 = {1.0f, 1.0f},
		const ImVec4& border_col = {1.0f, 1.0f, 1.0f, 0.0f}) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		
		if (window->SkipItems)
			return;

		ImRect bb(window->DC.CursorPos, ImVec2{ window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y });
		if (border_col.w > 0.0f) {
			bb.Max.x += 2;
			bb.Max.y += 2;
		}

		window->DrawList->PushTextureID(user_texture_id);
		window->DrawList->AddCallback(BeginDraw3DTex, &layer);
		window->DrawList->PopTextureID();

		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, 0))
			return;

		if (border_col.w > 0.0f)
		{
			window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(border_col), 0.0f);
			window->DrawList->PrimReserve(6, 4);

			ImVec2 bbMin = { bb.Min.x + 1.0f, bb.Min.y + 1.0f };
			ImVec2 bbMax = { bb.Max.x - 1.0f, bb.Max.y - 1.0f };
			window->DrawList->PrimRectUV(bbMin, bbMax, uv0, uv1, ImColor{ 1.0f, 1.0f, 1.0f, 1.0f });
		}
		else
		{
			window->DrawList->PrimReserve(6, 4);
			window->DrawList->PrimRectUV(bb.Min, bb.Max, uv0, uv1, ImColor{ 1.0f, 1.0f, 1.0f, 1.0f });
		}
		window->DrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

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