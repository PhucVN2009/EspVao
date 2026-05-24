#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "Includes/obfuscate.h"
#include "Includes/Logger.h"
#include "Includes/Macros.h"
#include "Includes/Utils.h"
#include "TuanMeta/Call_Me.h"
#include "UnityResolve.h"
#include "TouchInput.h"
#include "Hook.h"
#include "SaveLoadMenu.h"
#include <sys/stat.h>
#include <ctime>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include "login.h"
static bool keyLoaded = true;
static bool isLogin = true;
static bool showLoginSuccess = false;
static float loginSuccessTimer = 0.0f;
static int Type = 0;
static float progress = 1.0f; 
static auto startTime = std::chrono::steady_clock::now();

void ShowLoginSuccess()
{
    ImGui::Begin("Login Success", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

    ImGui::TextColored(ImVec4(0, 1, 0, 1), "Đăng nhập thành công");
    ImGui::Text("Chào mừng đến với HAX - LQMB");

    auto currentTime = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsedTime = currentTime - startTime;

    float countdownTime = 7.0f;
    progress = 1.0f - (elapsedTime.count() / countdownTime);

    if (progress < 0.0f)
        progress = 0.0f;

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0, 1, 0, 1));
    ImGui::ProgressBar(progress, ImVec2(200, 5));
    ImGui::PopStyleColor();

    ImGui::End();
}

EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);

EGLBoolean _eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {

    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (glWidth > 0 && glHeight > 0) {
        if (Width == 0) Width = glWidth;
        if (Height == 0) Height = glHeight;
    }

    if (!setup) {
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        DrawImGuiStyle();

        static const ImWchar icons_ranges[] = {0xe000, 0xf8ff, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 2.3;
        icons_config.OversampleV = 2.3;

        io.Fonts->AddFontFromMemoryTTF(const_cast<std::uint8_t *>(Custom), sizeof(Custom), 25.f, NULL, io.Fonts->GetGlyphRangesVietnamese());
        io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data, font_awesome_size, 25.0f, &icons_config, icons_ranges);

        io.KeyMap[ImGuiKey_UpArrow] = 19;
        io.KeyMap[ImGuiKey_DownArrow] = 20;
        io.KeyMap[ImGuiKey_LeftArrow] = 21;
        io.KeyMap[ImGuiKey_RightArrow] = 22;
        io.KeyMap[ImGuiKey_Enter] = 66;
        io.KeyMap[ImGuiKey_Backspace] = 67;
        io.KeyMap[ImGuiKey_Escape] = 111;
        io.KeyMap[ImGuiKey_Delete] = 112;
        io.KeyMap[ImGuiKey_Home] = 122;
        io.KeyMap[ImGuiKey_End] = 123;

        ImGui_ImplOpenGL3_Init(OBFUSCATE("#version 300 es"));
        ImGui::GetStyle().ScaleAllSizes(3.0f);
        GetIconHero();
        LoadSaveLoadMenu();
        setup = true;
    }

    if (SetResolution && Width != glWidth) {
        SetResolution(Width, Height, true);
    }

    ImGuiIO &io = ImGui::GetIO();
    static bool WantTextInputLast = false;
    if (io.WantTextInput && !WantTextInputLast) ShowSoftKeyboardInput();
    WantTextInputLast = io.WantTextInput;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
    ImGui::NewFrame();
    TouchInput::Update();

    static bool AutoLogin = false;
    static std::string err;
    if (!isLogin) {
        ImGui::OpenPopup(OBFUSCATE("##LoginPage"));
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(OBFUSCATE("##LoginPage"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
            ImGui::Text(OBFUSCATE("Please Login Key (Lần Đầu Login Sẽ Bị Văng)"));
            ImGui::PushItemWidth(-1);
            ImGui::InputText(OBFUSCATE("##key"), s, sizeof s);
                 if (!keyLoaded) {
                loadKey();
                keyLoaded = true;
                
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(-1);
            if (ImGui::Button(OBFUSCATE("Dán Key"), ImVec2(ImGui::GetWindowContentRegionWidth(), 0))) {
                auto key = getClipboard();
                strncpy(s, key.c_str(), sizeof s);
            }
            ImGui::PopItemWidth();
            ImGui::PushItemWidth(-1);
            if (ImGui::Button(OBFUSCATE("Đăng Nhập"), ImVec2(ImGui::GetWindowContentRegionWidth(), 0)) || (AutoLogin && err.empty())) {
                
                err = Login(s);
                if (err == "OK") {
                    isLogin = bValid && g_Auth == g_Token;
					saveKey();
                    showLoginSuccess = true;
                    loginSuccessTimer = 0.0f; // Reset lại bộ đếm thời gian khi đăng nhập thành công
                }
            }
            ImGui::Text(OBFUSCATE("Ấn Tăng/Giảm Âm Lượng Để Hiện/Ẩn Menu"));
            if (!err.empty() && err != std::string(OBFUSCATE("OK"))) {
                ImGui::Text(OBFUSCATE("Error: %s"), err.c_str());
            }
            ImGui::EndPopup();
        }
    } else {
      /*  if (!g_Token.empty() && !g_Auth.empty() && g_Token == g_Auth) {*/
            DrawESP(ImGui::GetBackgroundDrawList());
            if (ShowMenu) {
                ImGui::OpenPopup(OBFUSCATE("##MenuMod"));
                ImGui::SetNextWindowSize(ImVec2(900, 0));
                ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal(OBFUSCATE("##MenuMod"), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove)) {
                    ImGuiWindow* window = ImGui::GetCurrentWindow();
                    ImDrawList* drawList = window->DrawList;
                    ImVec2 windowPos = window->Pos;
                    ImVec2 windowSize = window->Size;
                    ImVec2 textSize = ImGui::CalcTextSize("ESP MOD " __DATE__ " " __TIME__);
                    ImVec2 textPos = ImVec2(windowPos.x + (windowSize.x - textSize.x) * 0.5f, windowPos.y + 24.0f);
                    drawList->AddText(textPos, IM_COL32_WHITE, "ESP MOD " __DATE__ " " __TIME__);

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.f, 6.f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 300.0f);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(255, 0, 0, 255));
                    if (ImGui::Button(OBFUSCATE("##CloseMenu"), ImVec2(24, 24))) {
                        ShowMenu = false;
                    }
                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
                    ImGui::PopStyleVar();

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Columns(2, NULL, false);
                    ImGui::SetColumnOffset(1, 200.0f);

                    ImGui::Text(OBFUSCATE("LÂM MOD LQ 2.3"));
                    ImGui::Text(OBFUSCATE("FPS: %.1f"), ImGui::GetIO().Framerate);

                    ImGui::PushStyleColor(ImGuiCol_Button, TabMenu == 1 ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
                    if (ImGui::Button(OBFUSCATE(ICON_FA_EYE " Visual"), ImVec2(170, 60))) TabMenu = 1;
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_Button, TabMenu == 2 ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
                    if (ImGui::Button(OBFUSCATE(ICON_FA_CAMERA " Camera"), ImVec2(170, 60))) TabMenu = 2;
                    ImGui::PopStyleColor();

                    ImGui::PushStyleColor(ImGuiCol_Button, TabMenu == 3 ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
                    if (ImGui::Button(OBFUSCATE(ICON_FA_MICROCHIP " Memory"), ImVec2(170, 60))) TabMenu = 3;
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, TabMenu == 4 ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
                if(ImGui::Button(OBFUSCATE(ICON_FA_WRENCH " Setting"), ImVec2(170, 60))) TabMenu = 4;
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, TabMenu == 5 ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
                if(ImGui::Button(OBFUSCATE(ICON_FA_USERS " About"), ImVec2(170, 60))) TabMenu = 5;
                ImGui::PopStyleColor();

                ImGui::PushStyleColor(ImGuiCol_Button, TabMenu == 7 ? ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] : ImGui::GetStyle().Colors[ImGuiCol_Button]);
                if(ImGui::Button(OBFUSCATE(ICON_FA_WRENCH " Debug"), ImVec2(170, 60))) TabMenu = 7;
                ImGui::PopStyleColor();

                ImGui::NextColumn();

                if(TabMenu == 1){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab1"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

                    ImGui::BeginTable(OBFUSCATE("##split_table1"), 2);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Enable ESP"), &ESP.Enable);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Line"), &ESP.Line);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Box"), &ESP.Box);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Cooldown"), &ESP.Cooldown);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP HP"), &ESP.HP);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Map"), &ESP.Map);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Visible Check"), &ESP.VisibleCheck);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Show Player Info"), &ESP.PlayerInfo);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Alert"), &ESP.Alert);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Show Hero Image"), &ESP.HeroImage);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Minions"), &ESP.Minions);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("ESP Ultimate"), &ESP.Ultimate);
                    ImGui::EndTable();

                    ImGui::EndChild();
                }

                if(TabMenu == 2){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab2"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

                    ImGui::Text(OBFUSCATE("Cam Xa:"));
                    ImGui::Checkbox(OBFUSCATE("##EnableCamV1"), &Camera.V1.Enable); ImGui::SameLine(); ImGui::PushItemWidth(-1); ImGui::SliderInt(OBFUSCATE("##SliderCameraV1"), &Camera.V1.Value, 1, 100); ImGui::PopItemWidth();
                    ImGui::EndChild();
                }

                if(TabMenu == 3){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab3"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

                    ImGui::BeginTable(OBFUSCATE("##split_table2"), 2);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Hack Map"), &MemoryHack.Map);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Hiện Ulti"), &MemoryHack.Unti);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Hiện Tên Cấm Chọn"), &MemoryHack.NameBanPick);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Hiện Avatar"), &MemoryHack.Avatar);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Hiện Lịch Sử Đấu"), &MemoryHack.History);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Hiện Hồi Chiêu"), &MemoryHack.ShowCooldown);
                    ImGui::EndTable();

                    ImGui::Spacing();
                    ImGui::Text(OBFUSCATE("Aimbot Menu"));
                    ImGui::BeginTable(OBFUSCATE("##split_table3"), 2);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Aimbot C2 Elsu"), &MemoryHack.AutoTrungElsu);
                    ImGui::TableNextColumn(); ImGui::Checkbox(OBFUSCATE("Ẩn Tia Elsu"), &MemoryHack.HideLineElsu);
                    ImGui::EndTable();
/*
                    ImGui::Spacing();

                    ImGui::Checkbox(OBFUSCATE("Bật Aim Chiêu 1"), &MemoryHack.Aimbot.C1);
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat(OBFUSCATE("##AimBotC1"), &MemoryHack.Aimbot.Value_1, 0.f, 100.f);
                    ImGui::PopItemWidth();

                    ImGui::Checkbox(OBFUSCATE("Bật Aim Chiêu 2"), &MemoryHack.Aimbot.C2);
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat(OBFUSCATE("##AimBotC2"), &MemoryHack.Aimbot.Value_2, 0.f, 100.f);
                    ImGui::PopItemWidth();

                    ImGui::Checkbox(OBFUSCATE("Bật Aim Chiêu 3"), &MemoryHack.Aimbot.C3);
                    ImGui::PushItemWidth(-1);
                    ImGui::SliderFloat(OBFUSCATE("##AimBotC3"), &MemoryHack.Aimbot.Value_3, 0.f, 100.f);
                    ImGui::PopItemWidth();
*/
                    ImGui::EndChild();
                }


                  if(TabMenu == 6){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab6"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

ImGui::Combo("##ddd", (int*)&Type, "Tắt\0Win\0Lose\0");
    
     ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
     ImGui::TextWrapped("Chọn Win xong rồi tắt để không văng");
     ImGui::PopStyleColor();
     switch (Type)
        {
        case 0: win = false; lose = false; break;
        case 1: win = true; lose = false; break;
        case 2: win = false; lose = false; break;
        }

                    ImGui::EndChild();
                }


                if(TabMenu == 4){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab4"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

                    SaveLoad_GUI();
                    ImGui::Checkbox(OBFUSCATE("Ẩn Icon Menu"), &HideIcon);

                    ImGui::Spacing();
                    ImGui::Text(OBFUSCATE("Minimap Offset"));
                    ImGui::SliderFloat(OBFUSCATE("posX"), &minimapPosX, 0.0f, 150.0f);
                    ImGui::SliderFloat(OBFUSCATE("posY"), &minimapPosY, 0.0f, 250.0f);
                    ImGui::SliderFloat(OBFUSCATE("scale"), &minimapScale, 1.0f, 6.0f);
                    if (ImGui::Button(OBFUSCATE("Reset Minimap"), ImVec2(-1, 0))) {
                        minimapPosX = 41.5f;
                        minimapPosY = 75.5f;
                        minimapScale = 3.1f;
                    }
                    ImGui::Spacing();
                    ImGui::SliderFloat(OBFUSCATE("ESP Depth Ref"), &g_espDepthRef, 10.0f, 200.0f);
                    ImGui::Spacing();
                    ImGui::Text(OBFUSCATE("ESP Ultimate"));
                    ImGui::SliderFloat(OBFUSCATE("Ult Scale"), &g_ultScale, 0.5f, 3.0f);
                    ImGui::SliderFloat(OBFUSCATE("Ult X"), &g_ultPosX, -500.0f, 500.0f);
                    ImGui::SliderFloat(OBFUSCATE("Ult Y"), &g_ultPosY, 0.0f, 500.0f);
                    ImGui::Text("MyCamp: %d (auto)", myPlayerCamp);

                    ImGui::EndChild();
                }

                if(TabMenu == 5){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab5"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

                    ImGui::Text(OBFUSCATE("Version: 2.3 Release - Patch: 1.62.1.4"));
                    ImGui::Text(OBFUSCATE("Auto-Update via UnityInline.h"));

                    ImGui::EndChild();
                }

                if(TabMenu == 7){
                    ImGui::BeginChild(OBFUSCATE("##ChildTab7"), ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y), false);

                    ImGui::TextColored(ImVec4(1,1,0,1), "DEBUG INFO");
                    ImGui::Separator();

                    ImGui::Text("il2cpp: 0x%llx", (unsigned long long)g_il2cpp_base);
                    ImGui::Text("LGameActorMgr: %p", LGameActorMgr);
                    ImGui::Text("Actors: %d", Response.Count);
                    ImGui::Text("MyCamp: %d (detected: %s)", myPlayerCamp, campDetected ? "YES" : "NO");
                    if (Response.Count > 0) {
                        ImGui::Text("P[0] HP: %d/%d", Response.players[0].ActorHP, Response.players[0].ActorHPTotal);
                        ImGui::Text("P[0] Pos: %.1f, %.1f, %.1f", Response.players[0].Position.x, Response.players[0].Position.y, Response.players[0].Position.z);
                        ImGui::Text("P[0] Sc: %.1f, %.1f", Response.players[0].PositionSc.x, Response.players[0].PositionSc.y);
                        ImGui::Text("P[0] Enemy: %d Vis: %d CfgID: %d", Response.players[0].isEnemy, Response.players[0].Visible, Response.players[0].ConfigID);
                    }
                    ImGui::Separator();

                    ImGui::TextColored(ImVec4(0,1,1,1), "METHOD POINTERS");
                    ImGui::Text("get_camera: %p", (void*)get_camera);
                    ImGui::Text("worldToScreen: %p", (void*)worldToScreen);
                    ImGui::Text("get_position: %p", (void*)get_position);
                    ImGui::Text("get_forward: %p", (void*)get_forward);
                    ImGui::Text("get_location: %p", (void*)get_location);
                    ImGui::Text("AsHero: %p", (void*)AsHero);
                    ImGui::Text("GiveMyEnemyCamp: %p", (void*)GiveMyEnemyCamp);
                    ImGui::Text("IsHostPlayer: %p", (void*)IsHostPlayer);
                    ImGui::Text("get_objCamp: %p", (void*)get_objCamp);
                    ImGui::Text("get_actorManager: %p", (void*)get_actorManager);
                    ImGui::Text("GetAllHeros_A: %p", (void*)GetAllHeros_ActorManager);
                    ImGui::Text("GetAllHeros_L: %p", (void*)GetAllHeros_LGameActorMgr);
                    ImGui::Text("actorHP: %p", (void*)actorHP);
                    ImGui::Text("actorMaxHP: %p", (void*)actorMaxHP);
                    ImGui::Text("get_bVisible: %p", (void*)get_bVisible);
                    ImGui::Text("GetHeroWrapSkill: %p", (void*)GetHeroWrapSkillData);
                    ImGui::Separator();

                    ImGui::TextColored(ImVec4(0,1,1,1), "FIELD OFFSETS");
                    ImGui::Text("ValueComponent: 0x%lx", (unsigned long)PlayerESP.ValueComponent);
                    ImGui::Text("ObjLinker: 0x%lx", (unsigned long)PlayerESP.ObjLinker);
                    ImGui::Text("SkillControl: 0x%lx", (unsigned long)g_SkillControlOff);
                    ImGui::Text("SkillSlotArray: 0x%lx", (unsigned long)g_SkillSlotArrayOff);
                    ImGui::Separator();

                    ImGui::TextColored(ImVec4(1,0.5,0,1), "SKILL CD DEBUG");
                    // Show first enemy hero skill data
                    for (int di = 0; di < collected_actor_count; di++) {
                        if (collected_actors[di].isHero && collected_actors[di].enemyCamp == myPlayerCamp && collected_actors[di].hp > 0) {
                            ImGui::Text("Hero cfgID=%d", collected_actors[di].configID);
                            ImGui::Text("  S1: cd=%d unlock=%d", collected_actors[di].skill1CD, collected_actors[di].skill1Unlock);
                            ImGui::Text("  S2: cd=%d unlock=%d", collected_actors[di].skill2CD, collected_actors[di].skill2Unlock);
                            ImGui::Text("  S3: cd=%d unlock=%d", collected_actors[di].skill3CD, collected_actors[di].skill3Unlock);
                            ImGui::Text("  Talent: cd=%d  Heal: cd=%d", collected_actors[di].talentCD, collected_actors[di].healCD);
                            break;
                        }
                    }
                    ImGui::Separator();

                    ImGui::Checkbox(OBFUSCATE("Show All IDs on Screen"), &dbg_showAllIDs);

                    ImGui::EndChild();
                }

                ImGui::EndPopup();
            }
        }else{
            ImGui::Begin(OBFUSCATE(""), 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
            ImGui::SetNextWindowSize(ImVec2(screenWidth * 0.30f, screenHeight * 0.70f), ImGuiCond_Once);
            if (!HideIcon) {
            std::string fpsString = std::to_string(static_cast<int>(ImGui::GetIO().Framerate));
            ImGuiStyle &style = ImGui::GetStyle();
            ImVec4 original_button_color = style.Colors[ImGuiCol_Button];
            float original_frame_rounding = style.FrameRounding;
            ImVec4 original_text_color = style.Colors[ImGuiCol_Text];
            style.Colors[ImGuiCol_Button] = ImVec4(ImGui::ColorConvertU32ToFloat4(IM_COL32(255, 255, 255, 100)));
            style.FrameRounding = 64 * 0.5f;
            style.Colors[ImGuiCol_Text] = ImVec4(0.f, 0.f, 0.f, 1.f);
            if(ImGui::Button(fpsString.c_str(), ImVec2(64, 64))){
                ShowMenu = true;
            }
            style.Colors[ImGuiCol_Button] = original_button_color;
            style.FrameRounding = original_frame_rounding;
            style.Colors[ImGuiCol_Text] = original_text_color;
        }
            ImGui::End();
        }
    
    }
    ImGui::Render();
    ImGui::EndFrame();
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &glWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &glHeight);
    
    io.KeysDown[io.KeyMap[ImGuiKey_UpArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_DownArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_LeftArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_RightArrow]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Enter]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Backspace]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Delete]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Escape]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_Home]] = false;
    io.KeysDown[io.KeyMap[ImGuiKey_End]] = false;
    
    return orig_eglSwapBuffers(dpy, surface);
}


ProcMap unityMap, il2cppMap;
using KittyScanner::RegisterNativeFn;
void *Init_Thread(void *) {
    while (!il2cppMap.isValid()) {
		il2cppMap = KittyMemory::getLibraryBaseMap("libil2cpp.so");
		sleep(1);
	}
	
    InitUnityResolve();
    TouchInput::Init();

    // === ESP Core ===
    get_camera = (void *(*)()) GetMethodOffset("UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "get_main", 0);
    if (!get_camera) get_camera = (void *(*)()) GetMethodOffset("UnityEngine.dll", "UnityEngine", "Camera", "get_main", 0);
    worldToScreen = (Vector3 (*)(void *, Vector3)) GetMethodOffset("UnityEngine.CoreModule.dll", "UnityEngine", "Camera", "WorldToScreenPoint", 1);
    if (!worldToScreen) worldToScreen = (Vector3 (*)(void *, Vector3)) GetMethodOffset("UnityEngine.dll", "UnityEngine", "Camera", "WorldToScreenPoint", 1);
    get_position = (Vector3 (*)(void*)) GetMethodOffset("Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_position", 0);

    get_forward = (VInt3 (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "get_forward", 0);
    get_location = (VInt3 (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "get_location", 0);
    AsHero = (void* (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "AsHero", 0);
    GiveMyEnemyCamp = (int (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "GiveMyEnemyCamp", 0);
    PlayerESP.ValueComponent = (uintptr_t) GetFieldOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "ValueComponent");

    IsHostPlayer = (bool (*)(void *)) GetMethodOffset("Project_d.dll", "Kyrios.Actor", "ActorLinker", "IsHostPlayer", 0);
    get_objCamp = (int (*)(void *)) GetMethodOffset("Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_objCamp", 0);
    PlayerESP.ObjLinker = (uintptr_t) GetFieldOffset("Project_d.dll", "Kyrios.Actor", "ActorLinker", "ObjLinker");
    get_bVisible = (bool (*)(void *)) GetMethodOffset("Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_bVisible", 0);

    // Visibility for minions/monsters now handled by SetVisible hook cache (no extra resolve needed)

    get_IsDeadState = (bool (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LObjWrapper", "get_IsDeadState", 0);
    GetHeroWrapSkillData = (HeroWrapSkillData (*)(void *, int)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LHeroWrapper", "GetHeroWrapSkillData", 1);

    GetAllHeros_LGameActorMgr = (List<void **> *(*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "GetAllHeros", 0);
    if (!GetAllHeros_LGameActorMgr)
        GetAllHeros_LGameActorMgr = (List<void **> *(*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "GetAllHeros", 1);
    GetAllHeros_ActorManager = (List<void **> *(*)(void *)) GetMethodOffset("Project_d.dll", "Kyrios.Actor", "ActorManager", "GetAllHeros", 0);
    if (!GetAllHeros_ActorManager)
        GetAllHeros_ActorManager = (List<void **> *(*)(void *)) GetMethodOffset("Project_d.dll", "Kyrios.Actor", "ActorManager", "GetAllHeros", 1);

    GetAllJungleMonsters_LGameActorMgr = (List<void **> *(*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "GetAllJungleMonsters", 0);
    GetAllMonsters_LGameActorMgr = (List<void **> *(*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "GetAllMonsters", 0);

    get_actorManager = (void *(*)()) GetMethodOffset("Project_d.dll", "Kyrios", "KyriosFramework", "get_actorManager", 0);

    actorHP = (int (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "ValuePropertyComponent", "get_actorHp", 0);
    actorMaxHP = (int (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "ValuePropertyComponent", "get_actorHpTotal", 0);
    get_actorSoulLevel = (int (*)(void *)) GetMethodOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "ValuePropertyComponent", "get_actorSoulLevel", 0);

    // Set globals for Wupdate
    g_ValCompOff = PlayerESP.ValueComponent;
    g_hp = actorHP;
    g_maxhp = actorMaxHP;
    g_level = get_actorSoulLevel;

    __android_log_print(ANDROID_LOG_INFO, "ESP_INIT", "cam=%p w2s=%p pos=%p fwd=%p loc=%p hero=%p enemy=%p host=%p camp=%p mgr=%p",
        get_camera, worldToScreen, get_position, get_forward, get_location, AsHero, GiveMyEnemyCamp, IsHostPlayer, get_objCamp, get_actorManager);
    __android_log_print(ANDROID_LOG_INFO, "ESP_INIT", "allH_L=%p allH_A=%p hp=%p maxhp=%p lvl=%p vis=%p skill=%p valComp=%lu objLink=%lu",
        GetAllHeros_LGameActorMgr, GetAllHeros_ActorManager, actorHP, actorMaxHP, get_actorSoulLevel, get_bVisible, GetHeroWrapSkillData,
        (unsigned long)PlayerESP.ValueComponent, (unsigned long)PlayerESP.ObjLinker);

    // ESP update hooks - DISABLED for crash test
    HOOKAU("Project_d.dll", "", "CameraSystem", "LateUpdate", 0, ESPUpdateResponse, _ESPUpdateResponse);
    if (!_ESPUpdateResponse) HOOKAU("Project_d.dll", "Assets.Scripts.GameSystem", "CameraSystem", "LateUpdate", 0, ESPUpdateResponse, _ESPUpdateResponse);
    HOOKAU("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "UpdateLogic", 1, UpdateLogic_LGameActorMgr, _UpdateLogic_LGameActorMgr);
    HOOKAU("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "FightOver", 0, FightOver_LGameActorMgr, _FightOver_LGameActorMgr);
    HOOKAU("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "DestroyActor", 1, DestroyActor, _DestroyActor);

    HOOKAU("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "UpdateLogic", 1, Wupdate, _Wupdate);

    // ActorLinker hook for camp detection
    HOOKAU("Project_d.dll", "Kyrios.Actor", "ActorLinker", "UpdateLogic", 1, ActorLinkerUpdate, _ActorLinkerUpdate);
    if (!_ActorLinkerUpdate) HOOKAU("Project_d.dll", "", "ActorLinker", "UpdateLogic", 1, ActorLinkerUpdate, _ActorLinkerUpdate);
    if (!_ActorLinkerUpdate) HOOKAU("Project_d.dll", "Kyrios.Actor", "ActorLinker", "Update", 0, ActorLinkerUpdate, _ActorLinkerUpdate);
    if (!_ActorLinkerUpdate) HOOKAU("Project_d.dll", "Kyrios.Actor", "ActorLinker", "LateUpdate", 0, ActorLinkerUpdate, _ActorLinkerUpdate);
    __android_log_print(ANDROID_LOG_INFO, "CAMP_DETECT", "ActorLinkerUpdate hook: %p", (void*)_ActorLinkerUpdate);

    // === FPS Unlock ===
    HOOKAU("Project_d.dll", "Assets.Scripts.Framework", "GameSettings", "get_Supported60FPSMode", 0, TRUE, _TRUE);
    HOOKAU("Project_d.dll", "Assets.Scripts.Framework", "GameSettings", "get_Supported90FPSMode", 0, TRUE, _TRUE);
    HOOKAU("Project_d.dll", "Assets.Scripts.Framework", "GameSettings", "get_Supported120FPSMode", 0, TRUE, _TRUE);
    HOOKAU("Project_d.dll", "Assets.Scripts.Framework", "GameSettings", "get_SupportedBoth60FPS_CameraHeight", 0, TRUE, _TRUE);
    HOOKAU("Project_d.dll", "Assets.Scripts.Framework", "GameSettings", "IsIPadDevice", 0, TRUE, _TRUE);

    // === Camera ===
    HOOKAU("Project_d.dll", "", "CameraSystem", "GetCameraHeightRateValue", 1, GetCameraHeightRateValue, _GetCameraHeightRateValue);
    HOOKAU("Project_d.dll", "", "CameraSystem", "OnCameraHeightChanged", 0, OnCameraHeightChanged, _OnCameraHeightChanged);

    // === Minimap ===
    get_MinimapScale = (Vector2 (*)(void *))GetMethodOffset("Project_d.dll", "Assets.Scripts.GameSystem", "MinimapSys", "get_MinimapScale", 0);
    get_BigMapScale = (Vector2 (*)(void *))GetMethodOffset("Project_d.dll", "Assets.Scripts.GameSystem", "MinimapSys", "get_BigMapScale", 0);
    get_mmFinalScreenSize = (Vector2 (*)(void *))GetMethodOffset("Project_d.dll", "Assets.Scripts.GameSystem", "MinimapSys", "get_mmFinalScreenSize", 0);
    GetMMFianlScreenPos = (Vector2 (*)(void *))GetMethodOffset("Project_d.dll", "Assets.Scripts.GameSystem", "MinimapSys", "GetMMFianlScreenPos", 0);
    HOOKAU("Project_d.dll", "Assets.Scripts.GameSystem", "MinimapSys", "Update", 0, MiniMapSys, _MiniMapSys);

    // === Visibility ===
    HOOKAU("Project.Plugins_d.dll", "NucleusDrive.Logic", "LVActorLinker", "SetVisible", 3, SetVisible, _SetVisible);

    // Find LVActorLinker field pointing to LActorRoot
    {
        const char *fieldNames[] = {"actorRoot", "m_actorRoot", "ActorRoot", "m_ActorRoot",
            "owner", "m_owner", "Actor", "m_actor", "logicActor", "m_logicActor",
            "actorPtr", "m_actorPtr", "root", "m_root", "lActorRoot", "m_lActorRoot",
            "actorObj", "m_actorObj", "hostActor", "wrapper", "m_wrapper",
            "objActor", "m_objActor", "actorLogic", "m_actorLogic",
            "LActorRoot", "m_LActorRoot", "actor", "objRoot", "m_objRoot", nullptr};
        for (int i = 0; fieldNames[i]; i++) {
            uintptr_t off = (uintptr_t)GetFieldOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LVActorLinker", fieldNames[i]);
            if (off > 0 && off < 0x200) {
                __android_log_print(ANDROID_LOG_INFO, "FIELD_DUMP", "FOUND: LVActorLinker.%s = 0x%lx", fieldNames[i], (unsigned long)off);
            }
        }
        // Also try on parent class LObjLinker
        for (int i = 0; fieldNames[i]; i++) {
            uintptr_t off = (uintptr_t)GetFieldOffset("Project.Plugins_d.dll", "NucleusDrive.Logic", "LObjLinker", fieldNames[i]);
            if (off > 0 && off < 0x200) {
                __android_log_print(ANDROID_LOG_INFO, "FIELD_DUMP", "FOUND: LObjLinker.%s = 0x%lx", fieldNames[i], (unsigned long)off);
            }
        }
    }

    __android_log_print(ANDROID_LOG_INFO, "ESP_INIT", "All hooks installed");
    
    
    
    return nullptr;
}


JNIEXPORT jint JNICALL 
JNI_OnLoad(JavaVM *vm, void * reserved) {
	jvm = vm;
	JNIEnv *env;
	
	vm->GetEnv((void **) &env, JNI_VERSION_1_6);
	
    Tools::Hook((void *) DobbySymbolResolver(OBFUSCATE("/system/lib/libandroid.so"), OBFUSCATE("ANativeWindow_getWidth")), (void *) _ANativeWindow_getWidth, (void **) &orig_ANativeWindow_getWidth);
    Tools::Hook((void *) DobbySymbolResolver(OBFUSCATE("/system/lib/libandroid.so"), OBFUSCATE("ANativeWindow_getHeight")), (void *) _ANativeWindow_getHeight, (void **) &orig_ANativeWindow_getHeight);
    Tools::Hook((void *) DobbySymbolResolver(OBFUSCATE("/system/lib/libEGL.so"), OBFUSCATE("eglSwapBuffers")), (void *) _eglSwapBuffers, (void **) &orig_eglSwapBuffers);

	pthread_t myThread;
	pthread_create(&myThread, NULL, Init_Thread, NULL);

	return JNI_VERSION_1_6;
}
