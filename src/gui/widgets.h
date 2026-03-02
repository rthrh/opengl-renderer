#include <imgui.h>
#include <vector>
#include <string>




const float topPanelHeight = 40.0f;
const float editorWidthDefault = 100.0f;
const float editorHeightDefault = 200.0f;



struct CodeTab
{
    std::string name;
    std::vector<char> buffer;

    CodeTab(const std::string& n)
        : name(n), buffer(8192, 0) // 8KB per tab
    {}
};

void DrawEditorWindow()
{
    static std::vector<CodeTab> tabs = {
        CodeTab("shader.vert"),
        CodeTab("shader.frag"),
        CodeTab("shader.geom"),
        CodeTab("shader.comp")
    };

    static int activeTab = 0;

    ImGui::Begin("Code Editor");

    // Top buttons
    if (ImGui::Button("Build")) {}
    ImGui::SameLine();
    if (ImGui::Button("Run")) {}
    ImGui::SameLine();
    if (ImGui::Button("Save")) {}

    ImGui::Separator();

    // Tabs
    if (ImGui::BeginTabBar("Shaders"))
    {
        for (size_t i = 0; i < tabs.size(); ++i)
        {
            if (ImGui::BeginTabItem(tabs[i].name.c_str()))
            {
                activeTab = i;
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::Separator();

    // Text editor
    ImGui::InputTextMultiline(
        "##editor",
        tabs[activeTab].buffer.data(),
        tabs[activeTab].buffer.size(),
        ImVec2(-FLT_MIN, -FLT_MIN), // fill remaining space
        ImGuiInputTextFlags_AllowTabInput
    );

    ImGui::End();
}

void DrawTopPanel()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    float panelHeight = 40.0f;

    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, panelHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Toolbar", nullptr, flags);


    if (ImGui::Button("Load model"))
    {
    }

    ImGui::SameLine();

    if (ImGui::Button("Something"))
    {
    }

    ImGui::SameLine();

    if (ImGui::Button("Button 3"))
    {
    }

    ImGui::End();
}

void DrawFPSOverlay()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float padding = 10.0f;

    ImVec2 position = ImVec2(
        viewport->WorkPos.x + padding,
        viewport->WorkPos.y + padding + topPanelHeight
    );

    ImGui::SetNextWindowPos(position, ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f); // Transparent background

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoInputs;  // Let mouse pass through

    if (ImGui::Begin("FPSOverlay", nullptr, flags))
    {
        float fps = ImGui::GetIO().Framerate;
        ImGui::Text("FPS: %.1f", fps);
    }
    ImGui::End();
}

