#include <openssl/aes.h>
#include <openssl/rand.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;
bool FoundProfile, AutoLoadProfile;

void DeleteFile(std::string FilePath){
    remove(FilePath.c_str());
}

void SaveConfig(std::string FilePath){
    json config;

    /*

             * HOW TO USE *
    config[" NAME_IN_CONFIG "] = VARIBLE;

    */

    config["PROFILE_AUTO_LOAD_ENABLE"] = AutoLoadProfile;
    config["Enable_ESP"] = ESP.Enable;
    config["ESP_Line"] = ESP.Line;
    config["ESP_Box"] = ESP.Box;
    config["ESP_Cooldown"] = ESP.Cooldown;
    config["ESP_HP"] = ESP.HP;
    config["ESP_Map"] = ESP.Map;
    config["Visible_Check"] = ESP.VisibleCheck;
    config["Player_Info"] = ESP.PlayerInfo;
    config["ESP_Alert"] = ESP.Alert;
    config["Camera_V1_Enable"] = Camera.V1.Enable;
    config["Camera_V1_Value"] = Camera.V1.Value;
    config["Camera_V2_Enable"] = Camera.V2.Enable;
    config["Camera_V2_Value"] = Camera.V2.Value;

    config["MemoryHack_HackMap"] = MemoryHack.Map;
    config["MemoryHack_ShowNameBanPick"] = MemoryHack.NameBanPick;
    config["MemoryHack_ShowAvatar"] = MemoryHack.Avatar;
    config["MemoryHack_ShowHistory"] = MemoryHack.History;
    config["MemoryHack_ShowCooldown"] = MemoryHack.ShowCooldown;
    config["MemoryHack_ShowUltimate"] = MemoryHack.Unti;
    config["MemoryHack_AutoTrungElsu"] = MemoryHack.AutoTrungElsu;
    config["MemoryHack_HideLineElsu"] = MemoryHack.HideLineElsu;
    config["MemoryHack_Aimbot_C1"] = MemoryHack.Aimbot.C1;
    config["MemoryHack_Aimbot_C2"] = MemoryHack.Aimbot.C2;
    config["MemoryHack_Aimbot_C3"] = MemoryHack.Aimbot.C3;
    config["MemoryHack_Aimbot_C1_Value"] = MemoryHack.Aimbot.Value_1;
    config["MemoryHack_Aimbot_C2_Value"] = MemoryHack.Aimbot.Value_2;
    config["MemoryHack_Aimbot_C3_Value"] = MemoryHack.Aimbot.Value_3;
    config["Hide_Icon_Menu"] = HideIcon;


    std::ofstream file(FilePath.c_str());
    if(!file.is_open()) return;
    file << config.dump(4);
    file.close();
}

void Update_Setting(std::string FilePath){
    json config;

    /*

                             * HOW TO USE *
    config[" NAME_IN_CONFIG "] = config.value(" NAME_IN_CONFIG ", VARIBLE);

    */

    config["PROFILE_AUTO_LOAD_ENABLE"] = AutoLoadProfile;
    config["Enable_ESP"] = config.value("Enable_ESP", ESP.Enable);
    config["ESP_Line"] = config.value("ESP_Line", ESP.Line);
    config["ESP_Box"] = config.value("ESP_Box", ESP.Box);
    config["ESP_Cooldown"] = config.value("ESP_Cooldown", ESP.Cooldown);
    config["ESP_HP"] = config.value("ESP_HP", ESP.HP);
    config["ESP_Map"] = config.value("ESP_Map", ESP.Map);
    config["Visible_Check"] = config.value("Visible_Check", ESP.VisibleCheck);
    config["Player_Info"] = config.value("Player_Info", ESP.PlayerInfo);
    config["ESP_Alert"] = config.value("ESP_Alert", ESP.Alert);
    config["Show_Hero_Image"] = config.value("Show_Hero_Image", ESP.HeroImage);
    config["Camera_V1_Enable"] = config.value("Camera_V1_Enable", Camera.V1.Enable);
    config["Camera_V1_Value"] = config.value("Camera_V1_Value", Camera.V1.Value);
    config["Camera_V2_Enable"] = config.value("Camera_V2_Enable", Camera.V2.Enable);
    config["Camera_V2_Value"] = config.value("Camera_V2_Value", Camera.V2.Value);
    config["MemoryHack_HackMap"] = config.value("MemoryHack_HackMap", MemoryHack.Map);
    config["MemoryHack_ShowNameBanPick"] = config.value("MemoryHack_ShowNameBanPick", MemoryHack.NameBanPick);
    config["MemoryHack_ShowAvatar"] = config.value("MemoryHack_ShowAvatar", MemoryHack.Avatar);
    config["MemoryHack_ShowHistory"] = config.value("MemoryHack_ShowHistory", MemoryHack.History);
    config["MemoryHack_ShowCooldown"] = config.value("MemoryHack_ShowCooldown", MemoryHack.ShowCooldown);
    config["MemoryHack_ShowUltimate"] = config.value("MemoryHack_ShowUltimate", MemoryHack.Unti);
    config["MemoryHack_AutoTrungElsu"] = config.value("MemoryHack_AutoTrungElsu", MemoryHack.AutoTrungElsu);
    config["MemoryHack_HideLineElsu"] = config.value("MemoryHack_HideLineElsu", MemoryHack.HideLineElsu);
    config["MemoryHack_Aimbot_C1"] = config.value("MemoryHack_Aimbot_C1", MemoryHack.Aimbot.C1);
    config["MemoryHack_Aimbot_C2"] = config.value("MemoryHack_Aimbot_C2", MemoryHack.Aimbot.C2);
    config["MemoryHack_Aimbot_C3"] = config.value("MemoryHack_Aimbot_C3", MemoryHack.Aimbot.C3);
    config["MemoryHack_Aimbot_C1_Value"] = config.value("MemoryHack_Aimbot_C1_Value", MemoryHack.Aimbot.Value_1);
    config["MemoryHack_Aimbot_C2_Value"] = config.value("MemoryHack_Aimbot_C2_Value", MemoryHack.Aimbot.Value_2);
    config["MemoryHack_Aimbot_C3_Value"] = config.value("MemoryHack_Aimbot_C3_Value", MemoryHack.Aimbot.Value_3);
    config["Hide_Icon_Menu"] = config.value("Hide_Icon_Menu", HideIcon);

    std::ofstream file(FilePath.c_str());
    if (!file.is_open()) return;

    file << config.dump(4);
    file.close();
}

void AutoLoad_Load(std::string FilePath){
    std::ifstream fileStream(FilePath.c_str());

    json config;
    fileStream >> config;
    AutoLoadProfile = config.value("PROFILE_AUTO_LOAD_ENABLE", AutoLoadProfile);
    fileStream.close();
}

void Load_Profile(std::string FilePath){
    std::ifstream fileStream(FilePath.c_str());

    json config;
    fileStream >> config;

    /*

                   * HOW TO USE *
    VARIBLE = config.value(" NAME_IN_CONFIG ", VARIBLE);

    */

    ESP.Enable = config.value("Enable_ESP", ESP.Enable);
    ESP.Line = config.value("ESP_Line", ESP.Line);
    ESP.Box = config.value("ESP_Box", ESP.Box);
    ESP.Cooldown = config.value("ESP_Cooldown", ESP.Cooldown);
    ESP.HP = config.value("ESP_HP", ESP.HP);
    ESP.Map = config.value("ESP_Map", ESP.Map);
    ESP.VisibleCheck = config.value("Visible_Check", ESP.VisibleCheck);
    ESP.PlayerInfo = config.value("Player_Info", ESP.PlayerInfo);
    ESP.Alert = config.value("ESP_Alert", ESP.Alert);
    ESP.HeroImage = config.value("Show_Hero_Image", ESP.HeroImage);
    Camera.V1.Enable = config.value("Camera_V1_Enable", Camera.V1.Enable);
    Camera.V1.Value = config.value("Camera_V1_Value", Camera.V1.Value);
    Camera.V2.Enable = config.value("Camera_V2_Enable", Camera.V2.Enable);
    Camera.V2.Value = config.value("Camera_V2_Value", Camera.V2.Value);
    MemoryHack.Map = config.value("MemoryHack_HackMap", MemoryHack.Map);
    MemoryHack.NameBanPick = config.value("MemoryHack_ShowNameBanPick", MemoryHack.NameBanPick);
    MemoryHack.Avatar = config.value("MemoryHack_ShowAvatar", MemoryHack.Avatar);
    MemoryHack.History = config.value("MemoryHack_ShowHistory", MemoryHack.History);
    MemoryHack.ShowCooldown = config.value("MemoryHack_ShowCooldown", MemoryHack.ShowCooldown);
    MemoryHack.Unti = config.value("MemoryHack_ShowUltimate", MemoryHack.Unti);
    MemoryHack.AutoTrungElsu = config.value("MemoryHack_AutoTrungElsu", MemoryHack.AutoTrungElsu);
    MemoryHack.HideLineElsu = config.value("MemoryHack_HideLineElsu", MemoryHack.HideLineElsu);
    MemoryHack.Aimbot.C1 = config.value("MemoryHack_Aimbot_C1", MemoryHack.Aimbot.C1);
    MemoryHack.Aimbot.C2 = config.value("MemoryHack_Aimbot_C2", MemoryHack.Aimbot.C2);
    MemoryHack.Aimbot.C3 = config.value("MemoryHack_Aimbot_C3", MemoryHack.Aimbot.C3);
    MemoryHack.Aimbot.Value_1 = config.value("MemoryHack_Aimbot_C1_Value", MemoryHack.Aimbot.Value_1);
    MemoryHack.Aimbot.Value_2 = config.value("MemoryHack_Aimbot_C2_Value", MemoryHack.Aimbot.Value_2);
    MemoryHack.Aimbot.Value_3 = config.value("MemoryHack_Aimbot_C3_Value", MemoryHack.Aimbot.Value_3);
    HideIcon = config.value("Hide_Icon_Menu", HideIcon);
    fileStream.close();
}

bool Check_File(std::string FilePath){
    std::ifstream fileStream(FilePath.c_str());
    bool isAlive = fileStream.good();
    fileStream.close();
    return isAlive;
}

void SaveLoad_GUI(){
    if(FoundProfile){
        ImGui::Text(OBFUSCATE("Đã Có Một Profile Được Lưu"));
        if(AutoLoadProfile){
            ImGui::Text(OBFUSCATE("Tự Động Load Profile Đang Được Bật!"));
        }
    }else{
        ImGui::Text(OBFUSCATE("Hiện Không Có Profile Nào Được Lưu :D"));
    }

    // Save Profile
    if (FoundProfile) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }
    if(ImGui::Button(OBFUSCATE("Lưu Profile"), ImVec2(ImGui::GetContentRegionAvail().x, 0))){
        SaveConfig(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
        FoundProfile = true;
        AutoLoadProfile = false;
    }
    if (FoundProfile) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }


    // Del Profile
    if (!FoundProfile) {
        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    if(ImGui::Button(OBFUSCATE("Tải Profile"), ImVec2(ImGui::GetContentRegionAvail().x, 0))){
        Load_Profile(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
    }

    if(ImGui::Button(OBFUSCATE("Xóa Profile"), ImVec2(ImGui::GetContentRegionAvail().x, 0))){
        DeleteFile(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
        FoundProfile = false;
        AutoLoadProfile = false;
    }

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
    if(ImGui::Button(OBFUSCATE("Bật Tự Động Load Profile"), ImVec2(ImGui::GetContentRegionAvail().x, 0))){
        AutoLoadProfile = true;
        Update_Setting(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
    }
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.0f, 0.0f, 1.0f));
    if(ImGui::Button(OBFUSCATE("Tắt Tự Động Load Profile"), ImVec2(ImGui::GetContentRegionAvail().x, 0))){
        AutoLoadProfile = false;
        Update_Setting(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
    }
    ImGui::PopStyleColor();
    if (!FoundProfile) {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }
}

void LoadSaveLoadMenu(){
    if(Check_File(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"))){
        AutoLoad_Load(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
        FoundProfile = true;
        if(AutoLoadProfile){
            sleep(2);
            Load_Profile(OBFUSCATE("/storage/emulated/0/Android/data/lqmhax.online/user_config"));
        }
    }
}
