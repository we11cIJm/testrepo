#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>

#include "imfilebrowser.h"
#include "ImGuiColorTextEdit/TextEditor.h"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

#ifdef __EMSCRIPTEN__
#include "../libs/emscripten/emscripten_mainloop_stub.h"
#endif

void send() {

}

std::istream& readOneChar(std::istream& input) {
    char tmpC = 0;
    input.get(tmpC);
    return input;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int, char**)
{
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(3000, 2000, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.

    // Our state
    float globalScale = 1.1;

    bool toScrollEditor = false;
    bool toScrollComments = false;
    float prevScrollEditor = 0;
    float prevScrollComments = 0;
    float scrollEditor = 0;
    float scrollComments = 0;

    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    std::map<int, std::string> review_data;
    int numLines = 0;

    bool openChanger = false;
    int fileCount = 0;

    ImGui::FileBrowser fileDialog(ImGuiFileBrowserFlags_SelectDirectory | ImGuiFileBrowserFlags_ConfirmOnEnter);
    std::filesystem::path fileName;
    bool open_not_a_repo = false;
    fileDialog.SetTitle("Select path to repo");
    std::filesystem::path workPath = "";

    TextEditor editor;
    auto lang = TextEditor::LanguageDefinition::CPlusPlus();
    editor.SetLanguageDefinition(lang);
    editor.SetReadOnly(true);
    editor.SetImGuiChildIgnored(true);

    bool open = 0;

    if (std::filesystem::exists("settings.txt")) {
        std::ifstream settings("settings.txt");
        settings >> workPath;
        if (std::filesystem::exists(workPath / ".git")) {
            for (const auto& entry : std::filesystem::directory_iterator(workPath / "to_review")) {
                if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
                    ++fileCount;
                    if (fileCount == 1) {
                        fileName = entry.path().stem();
                    }
                }
            }
            std::ifstream readFirstFileTo_review((workPath / "to_review" / fileName).replace_extension(".cpp"));
            std::ifstream readFirstFileReview((workPath / "review" / fileName).replace_extension(".txt"));
            int num;
            std::string comment;
            while (std::getline(readOneChar(readFirstFileReview >> num), comment).good()) review_data[num] = comment;
            if (fileCount == 0) numLines = 0; else numLines = std::count(std::istreambuf_iterator<char>(readFirstFileTo_review), std::istreambuf_iterator<char>(), '\n');
            readFirstFileTo_review.seekg(0, std::ios::beg);
            std::string str((std::istreambuf_iterator<char>(readFirstFileTo_review)), std::istreambuf_iterator<char>());
            editor.SetText(str);
        }
        else {
            workPath = "";
        }
    }
    else {
        std::string err = "sth went wrong";
    }
    std::filesystem::path readPath = workPath;

#ifdef __EMSCRIPTEN__
    io.IniFilename = nullptr;
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while (!glfwWindowShouldClose(window))
#endif
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int x, y;
        glfwGetWindowSize(window, &x, &y);

        {
            ImGui::SetNextWindowPos(ImVec2(x / 2., 0));
            ImGui::SetNextWindowSize(ImVec2(x / 2., 0.71 * (globalScale * 37 + 18) / 1920 * x));
            float maxYRight = ImGui::GetScrollMaxY();
            float yRight = ImGui::GetScrollY();
            bool oi = true;
            ImGui::Begin("Main", &oi, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::SetWindowFontScale(globalScale * x / 1920);
            ImGui::Text("Write your comments here ");
            ImGui::SameLine();

            if (ImGui::Button("Send review")) {
                if (workPath == "") {
                    open = true;
                }
                else {
                    send();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Set path to repo")) {
                fileDialog.Open();
            }
            fileDialog.Display();

            ImGui::SameLine();
            if (ImGui::Button("Change review file")) {
                openChanger = true;
            }
            if (openChanger) {
                ImGui::SetNextWindowPos(ImVec2(0.4 * x, 0.4 * y));
                ImGui::SetNextWindowSize(ImVec2(0.2 * x, 0.2 * y));
                ImGui::Begin("File changer window", &openChanger, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
                std::filesystem::path toReview(workPath / "to_review");
                for (const auto& entry : std::filesystem::directory_iterator(toReview)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
                        if (ImGui::Button((std::string("Review file ") + entry.path().filename().string()).c_str(), ImVec2(globalScale * 120 / 1920 * x, globalScale * 13 / 1920 * x))) {
                            openChanger = false;

                            std::ofstream review((workPath / "review" / fileName).replace_extension(".txt"));
                            for (auto el : review_data) {
                                if (el.second.size() != 0) {
                                    review << el.first << ' ' << el.second << std::endl;
                                }
                            }
                            review_data.clear();

                            fileName = entry.path().stem();

                            std::ifstream readFirstFileReview((workPath / "review" / fileName).replace_extension(".txt"));
                            int num;
                            std::string comment;
                            while (std::getline(readOneChar(readFirstFileReview >> num), comment).good()) review_data[num] = comment;

                            std::ifstream readFirstFileTo_review((workPath / "to_review" / fileName).replace_extension(".cpp"));
                            if (fileCount == 0) numLines = 0; else numLines = std::count(std::istreambuf_iterator<char>(readFirstFileTo_review), std::istreambuf_iterator<char>(), '\n');
                            readFirstFileTo_review.seekg(0, std::ios::beg);
                            std::string str((std::istreambuf_iterator<char>(readFirstFileTo_review)), std::istreambuf_iterator<char>());
                            editor.SetText(str);
                        }
                    }
                }
                
                if (ImGui::Button("Close")) {
                    openChanger = false;
                }
                ImGui::End();
            }

            if (workPath != readPath) {
                workPath = readPath;
                fileCount = 0;
                for (const auto& entry : std::filesystem::directory_iterator(workPath / "to_review")) {
                    if (entry.is_regular_file() && entry.path().extension() == ".cpp") {
                        ++fileCount;
                        if (fileCount == 1) {
                            fileName = entry.path().stem();
                        }
                    }
                }
                std::ifstream readFirstFileTo_review((workPath / "to_review" / fileName).replace_extension(".cpp"));
                if (fileCount == 0) numLines = 0; else numLines = std::count(std::istreambuf_iterator<char>(readFirstFileTo_review), std::istreambuf_iterator<char>(), '\n');
                readFirstFileTo_review.seekg(0, std::ios::beg);
                std::string str((std::istreambuf_iterator<char>(readFirstFileTo_review)), std::istreambuf_iterator<char>());
                editor.SetText(str);
            }

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(x / 2., 0.71 * (globalScale * 37 + 18) / 1920 * x));
            ImGui::SetNextWindowSize(ImVec2(x / 2., y - 0.71 * (globalScale * 37 + 18) / 1920 * x));
            ImGui::Begin("Comments", &oi, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::SetWindowFontScale(0.88 * globalScale * x / 1920);

            ImGui::SetCursorPosY(globalScale * 13 / 1920 * x * numLines);
            ImGui::Dummy(ImVec2(0, globalScale * 22 / 1920 * x));

            if (toScrollComments) {
                ImGui::SetScrollY(scrollEditor * ImGui::GetScrollMaxY());
                toScrollComments = false;
            }

            int rem = 0;
            for (auto el : review_data) {
                int v = el.first;
                ImGui::SetCursorPos(ImVec2(0, globalScale * 13 / 1920 * x * (v - 1)));
                if (ImGui::Button((std::string("Remove comment ") + std::to_string(v)).c_str(), ImVec2(globalScale * 120 / 1920 * x, globalScale * 13 / 1920 * x))) {
                    rem = v;
                }
                ImGui::SameLine();
                char* tmp = new char(128);
                tmp = const_cast<char*>(el.second.c_str());
                ImGui::InputTextWithHint((std::string("##com ") + std::to_string(v)).c_str(), ("comment " + std::to_string(v)).c_str(), tmp, globalScale * 400 / 1920 * x);
                el.second = tmp;
                review_data[v] = tmp;
            }
            if (rem) {
                review_data.extract(rem);
            }
            prevScrollComments = scrollComments;
            scrollComments = ImGui::GetScrollY() / ImGui::GetScrollMaxY();
            ImGui::End();

            if (open) {
                ImGui::SetNextWindowPos(ImVec2(0.4 * x, 0.4 * y));
                ImGui::SetNextWindowSize(ImVec2(0.2 * x, 0.2 * y));
                ImGui::Begin("Didn't set path for review yet", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
                ImGui::SetWindowFontScale(globalScale * x / 1920);
                if (ImGui::Button("Close")) {
                    open = false;
                }
                ImGui::End();
            }
        }

        auto cpos = editor.GetCursorPosition();


        ImGui::SetNextWindowPos(ImVec2(0, 0), 0, ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(x / 2., y), 0);

        if (workPath != "") {
            ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::SetWindowFontScale(globalScale * x / 1920);
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Quit", "Alt-F4"))
                        break;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
                        editor.Copy();

                    ImGui::Separator();

                    if (ImGui::MenuItem("Select all", nullptr, nullptr))
                        editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    if (ImGui::MenuItem("Dark palette"))
                        editor.SetPalette(TextEditor::GetDarkPalette());
                    if (ImGui::MenuItem("Light palette"))
                        editor.SetPalette(TextEditor::GetLightPalette());
                    if (ImGui::MenuItem("Retro blue palette"))
                        editor.SetPalette(TextEditor::GetRetroBluePalette());
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            char mbstr[256];
            fileName.replace_extension(".cpp");
            wcstombs(mbstr, fileName.c_str(), sizeof(mbstr));
            fileName.replace_extension("");
            ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
                editor.IsOverwrite() ? "Ovr" : "Ins",
                editor.CanUndo() ? "*" : " ",
                editor.GetLanguageDefinition().mName.c_str(), mbstr);

            if ((ImGui::IsKeyPressed(ImGuiKey_Tab) && ImGui::GetIO().KeyCtrl)) {
                int line = editor.GetCursorPosition().mLine + 1;

                auto pred = [line](const std::pair<int, std::string>& item) {
                    return item.first == line;
                };
                if (review_data.find(line) == review_data.end()) {
                    review_data[line] = "";
                }
            }
            ImGui::BeginChild("TextEditor", ImVec2(0, 0), 0, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoMove);
            if (toScrollEditor) {
                ImGui::SetScrollY(scrollComments * ImGui::GetScrollMaxY());
                toScrollEditor = false;
            }
            editor.Render("TextEditor");
            prevScrollEditor = scrollEditor;
            scrollEditor = ImGui::GetScrollY() / ImGui::GetScrollMaxY();
            ImGui::EndChild();

            if (io.KeyCtrl) {
                globalScale += io.MouseWheel * 0.1f;
                globalScale = max(0.8f, globalScale);
            }

            ImGui::End();
        }

        if (fileDialog.HasSelected())
        {
            if (std::filesystem::exists(fileDialog.GetSelected() / ".git")) {
                readPath = fileDialog.GetSelected();
            }
            else {
                open_not_a_repo = true;
                fileDialog.Close();
            }
        }
        if (open_not_a_repo) {
            ImGui::SetNextWindowPos(ImVec2(0.4 * x, 0.4 * y));
            ImGui::SetNextWindowSize(ImVec2(0.2 * x, 0.2 * y));
            ImGui::Begin("Not a git repo");
            if (ImGui::Button("Close")) {
                open_not_a_repo = false;
                fileDialog.Open();
            }
            ImGui::End();
        }

        if (std::abs(scrollEditor - scrollComments) > 2.0 / y) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            if (prevScrollEditor != scrollEditor) {
                toScrollComments = true;
            }
            else {
                toScrollEditor = true;
            }
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif

    std::ofstream settings("settings.txt");
    settings << workPath;
    if (workPath != "") {
        std::ofstream review((workPath / "review" / fileName).replace_extension(".txt"));
        for (auto el : review_data) {
            if (el.second.size() != 0) {
                review << el.first << ' ' << el.second << std::endl;
            }
        }
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
