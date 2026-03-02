#pragma once

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <widgets.h>

struct GuiData {
    glm::vec4 color;
};

class GuiLayer {
    public:
        GuiLayer(GLFWwindow* window) {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            ImGui::StyleColorsDark();
            ImGui_ImplGlfw_InitForOpenGL(window, true);
            ImGui_ImplOpenGL3_Init("#version 330");

            io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
        }   

        ~GuiLayer() {
            // Deletes all ImGUI instances
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        void beginFrame() {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }
        void endFrame() {
            ImGui::Render();
		    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        void build(GuiData& data) {
            DrawTopPanel();
            DrawEditorWindow();
            DrawFPSOverlay();
        }

        static void setMouseEnabled(bool value) {
            ImGuiIO& io = ImGui::GetIO();
            if (value)
            {
                io.ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
            }
            else
            {
                io.ConfigFlags |= ImGuiConfigFlags_NoMouse;
            }
        }

        static bool wantCaptureMouse() {
            return ImGui::GetIO().WantCaptureMouse;
        }

        static bool wantCaptureKeyboard() {
            return ImGui::GetIO().WantCaptureKeyboard;
        }
};