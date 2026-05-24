#pragma once
#include "Actor.h"
#include "Hero.h"
#include "Includes/Strings.h"
#include "SkillIcon.h"
#include "Talent.h"
#include <cstdlib>
#include <time.h>
// __atomic_store_n / __atomic_load_n are GCC/Clang builtins, no header needed

static float GetTimeSeconds() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (float)ts.tv_sec + (float)ts.tv_nsec / 1000000000.0f;
}

static float LerpF(float a, float b, float t) {
  if (t <= 0.0f)
    return a;
  if (t >= 1.0f)
    return b;
  return a + (b - a) * t;
}

static Vector3 LerpVec3(Vector3 a, Vector3 b, float t) {
  return Vector3(LerpF(a.x, b.x, t), LerpF(a.y, b.y, t), LerpF(a.z, b.z, t));
}

// Lerp interval: how often position data updates (seconds)
static float g_lerpInterval = 0.066f; // ~15fps data update

static std::string EXP = " ";
/*
static int enable_hack;
static char *game_data_dir = NULL;
int isGame(JNIEnv *env, jstring appDataDir);
void *Init_Thread(void *arg);
*/

auto NOP = "1F 20 03 D5";
auto hex_rank =
    "41 02 00 54 88 76 02 F0 08 E1 45 F9 15 53 4D 29 00 01 40 F9 08 FC 44 39";
auto mask_rank = "xxxx??x?x??xxxxxxxxxxxxx";
auto hex_botro =
    "00 0B 00 36 E0 03 14 AA E1 03 1F AA 5E 7E 28 94 1F 04 00 71 6B 0A 00 54";
auto mask_botro = "xxxxxxxxxxxx???xxxxxxxxx";
bool win, lose, offwinlose;

int (*ActorLinker_COM_PLAYERCAMP)(void *instance);
int (*LActorRoot_COM_PLAYERCAMP)(void *instance);

bool (*ActorLinker_IsHostPlayer)(void *instance);
void *Lactor = nullptr;

// LActorRoot
VInt3 (*get_location)(void *instance);
VInt3 (*get_forward)(void *instance);
Vector3 (*get_position)(void *inst); // forward decl for Wupdate
void *(*AsHero)(void *instance);
int (*GiveMyEnemyCamp)(void *instance);

// ActorLinker
bool (*IsHostPlayer)(void *instance);
int (*get_objCamp)(void *instance);
bool (*get_bVisible)(void *instance);
// Visibility cache from SetVisible hook (LVActorLinker ptr → visible state for
// our camp)
struct VisibleCacheEntry {
  void *linkerPtr;
  bool visible;
};
static VisibleCacheEntry g_visibleCache[500];
static int g_visibleCacheCount = 0;

bool (*owner)(void *kk1);
bool (*isMine)(void *instance);
void *(*AsOrgan)(void *instance);
void (*SetHP)(void *lol, int hp);

static int myPlayerCamp = 0;
static bool campDetected = false;
static int campDetectAttempts = 0;

// Forward declarations for Wupdate
static uintptr_t g_ValCompOff = 0;
static int (*g_hp)(void *) = nullptr;
static int (*g_maxhp)(void *) = nullptr;
static int (*g_level)(void *) = nullptr;

// ESP actor collection in Wupdate
struct ActorEntry {
  void *ptr;
  void *linker;
  int hp;
  int maxhp;
  int level;
  int configID;
  int enemyCamp;
  bool isHero;
  bool visible;
  int visibleTimer;
  int skill1CD;
  int skill2CD;
  int skill3CD;
  int talentCD;
  int healCD;
  bool skill1Unlock;
  bool skill2Unlock;
  bool skill3Unlock;
  bool talentUnlock;
  bool healUnlock;
  VInt3 location;
  VInt3 forward;
  VInt3 prevLocation;
  VInt3 prevForward;
  int deadFrames; // count frames with hp<=0, auto-evict after threshold
  unsigned int talentSkillId; // talent/summoner spell ID
  Vector3 visualPos; // render-interpolated position from get_position (updated
                     // in Wupdate)
};
#define MAX_COLLECTED_ACTORS 200
static ActorEntry collected_actors[MAX_COLLECTED_ACTORS] = {0};
static int collected_actor_count = 0;
static float g_espDepthRef = 20.0f; // ESP depth reference for scaling
static float g_ultScale = 1.0f;     // ESP Ultimate scale
static float g_ultPosX = 0.0f;      // ESP Ultimate X offset from center
static float g_ultPosY = 35.0f;     // ESP Ultimate Y position from top

void (*_Wupdate)(void *lol2, int del);
// Skill data collection - resolved offsets cached
static uintptr_t g_SkillControlOff = 0;
static uintptr_t g_SkillSlotArrayOff = 0;
static bool g_skillOffsetsResolved = false;
static int g_skillDbgLogCount = 0;

// HeroWrapSkillData struct (moved here so Wupdate can use it)
struct HeroWrapSkillData {
  unsigned int SkillId; // 0x00
  int skillSlotCDMax;   // 0x04
  int skillLv;          // 0x08
  int _pad0;            // 0x0C (alignment padding for 8-byte pointer)
  void *skillIconPath;  // 0x10 (monoString*, 8 bytes on arm64)
  bool skillSlotUnlock; // 0x18
  bool skillSlotReady;  // 0x19
  char _pad1[2];        // 0x1A-0x1B
  int Skill1SlotCD;     // 0x1C
}; // total 0x20 = 32 bytes
static HeroWrapSkillData (*GetHeroWrapSkillData)(void *instance,
                                                 int skill) = nullptr;

// Forward declare (defined here, used throughout)
static void *LGameActorMgr = NULL;

void Wupdate(void *lol2, int del) {

  if (lol2 != NULL && campDetected && Lactor && LGameActorMgr) {
    // Cache Lactor locally — it may become null mid-execution on another thread
    void *localLactor = Lactor;
    if (!localLactor)
      goto wupdate_done;

    void *as1 = nullptr;
    try {
      as1 = AsOrgan ? AsOrgan(lol2) : nullptr;
    } catch (...) {
      __android_log_print(ANDROID_LOG_ERROR, "CRASH_DBG",
                          "Wupdate: AsOrgan crashed lol2=%p", lol2);
      goto wupdate_done;
    }

    // Win/lose logic only
    static uintptr_t g_valCompOffCached = 0;
    static bool g_valCompOffInit = false;
    if (!g_valCompOffInit) {
      g_valCompOffCached = (uintptr_t)GetFieldOffset(
          "Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot",
          "ValueComponent");
      g_valCompOffInit = true;
    }
    void *cn1 = g_valCompOffCached
                    ? *(void **)((uint64_t)lol2 + g_valCompOffCached)
                    : nullptr;
    if (as1 != NULL && cn1 != NULL) {
      try {
        if (win && isMine && GiveMyEnemyCamp && get_objCamp && !isMine(as1) &&
            localLactor && GiveMyEnemyCamp(lol2) == get_objCamp(localLactor)) {
          if (SetHP)
            SetHP(cn1, 0);
          win = false;
        }
        if (lose && isMine && GiveMyEnemyCamp && get_objCamp && !isMine(as1) &&
            localLactor && GiveMyEnemyCamp(lol2) != get_objCamp(localLactor)) {
          if (SetHP)
            SetHP(cn1, 0);
          lose = false;
        }
      } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "CRASH_DBG",
                            "Wupdate: win/lose logic crashed lol2=%p lactor=%p",
                            lol2, localLactor);
      }
    }
  }

wupdate_done:
  _Wupdate(lol2, del);
}

uintptr_t addr_botro = 0, addr_rank = 0;
static bool HideIcon;
const char *FPS_COUNTER_BTN_FORMAT = "%.1f";
std::string FPS_COUNTER_BTN;
int posXM = 0, posYM = 0;
struct _ShowUnti {
  struct _Slot1 {
    ImVec2 Position = ImVec2(465, 120);
  };
  _Slot1 Slot1{};
  struct _Slot2 {
    ImVec2 Position = ImVec2(530, 120);
  };
  _Slot2 Slot2{};
  struct _Slot3 {
    ImVec2 Position = ImVec2(595, 120);
  };
  _Slot3 Slot3{};
  struct _Slot4 {
    ImVec2 Position = ImVec2(660, 120);
  };
  _Slot4 Slot4{};
  struct _Slot5 {
    ImVec2 Position = ImVec2(725, 120);
  };
  _Slot5 Slot5{};
};
_ShowUnti ShowUnti{};

struct _ESP {
  bool Enable;
  bool Line;
  bool Box;
  bool Cooldown;
  bool HP;
  bool Map;
  bool VisibleCheck;
  bool PlayerInfo;
  bool Alert;
  bool HeroImage;
  bool Minions;
  bool Ultimate;
};
_ESP ESP{};

struct _CAM {
  struct _V1 {
    bool Enable;
    int Value;
  };
  _V1 V1{};

  struct _V2 {
    bool Enable;
    float Value;
  };
  _V2 V2{};
};
_CAM Camera{};

int TabMenu = 1;

// Memory Hooker //
struct _MemoryHack {
  bool Map;
  bool Unti;
  bool NameBanPick;
  bool Avatar;
  bool History;
  bool ShowCooldown;
  bool AutoTrungElsu;
  bool HideLineElsu;
  struct _AimBot {
    bool C1;
    bool C2;
    bool C3;
    float Value_1;
    float Value_2;
    float Value_3;
  };
  _AimBot Aimbot{};
};
_MemoryHack MemoryHack{};

bool (*_TRUE)(void *ins);
bool TRUE(void *ins) { return true; }

bool (*_FALSE)(void *ins);
bool FALSE(void *ins) { return false; }

int (*_MAX)(void *ins);
int MAX(void *ins) { return 1000000000; }

//
bool (*get_IsDeadState)(void *instance);
int (*actorHP)(void *instance);
int (*actorMaxHP)(void *instance);
int (*get_actorSoulLevel)(void *instance);

void (*SetPlayerName)(...);

void *(*get_camera)();
Vector3 (*worldToScreen)(void *cam, Vector3 pos);

// MinimapSys
Vector2 (*get_MinimapScale)(void *instance);
Vector2 (*get_BigMapScale)(void *instance);
Vector2 (*get_mmFinalScreenSize)(void *instance);
Vector2 (*GetMMFianlScreenPos)(void *instance);

List<void **> *(*GetAllHeros_ActorManager)(void *instance);
List<void **> *(*GetAllHeros_LGameActorMgr)(void *instance);
List<void **> *(*GetAllJungleMonsters_LGameActorMgr)(void *instance);
List<void **> *(*GetAllMonsters_LGameActorMgr)(void *instance);
void *(*get_actorManager)();

class ActorConfig {
public:
  int ConfigID() {
    return *(int *)((uintptr_t)this +
                    GetFieldOffset(OBFUSCATE("Project_d.dll"),
                                   OBFUSCATE("Assets.Scripts.GameLogic"),
                                   OBFUSCATE("ActorConfig"),
                                   OBFUSCATE("ConfigID")));
  }
};

// HeroWrapSkillData struct and GetHeroWrapSkillData defined above (before
// Wupdate)

struct Player {
  bool isEnemy;
  bool Visible;
  bool isHeroUnit;
  int Distance;
  int ActorHP;
  int ActorHPTotal;
  int ActorMaxHP;
  int ActorLevel;
  int ConfigID;
  int EnemyCamp;
  void *actorPtr;  // LActorRoot pointer for real-time get_location
  void *linkerPtr; // ActorLinker pointer for get_position
  Vector3 Position;
  Vector3 PrevPosition;
  Vector3 PositionSc;
  Vector3 PrevPositionSc;
  Vector3 My_Position;
  float lastUpdateTime;

  int Skill1Level;
  int Skill2Level;
  int Skill3Level;
  bool Skill1Unlock;
  bool Skill2Unlock;
  bool Skill3Unlock;
  int Skill1CD;
  int Skill2CD;
  int Skill3CD;
  int HPCD;
  int TalentCD;
  unsigned int TalentSkillId;
  int ItemActiveCD;
  unsigned int ItemActiveSkillId;
};

struct _Response {
  Player players[MAX_COLLECTED_ACTORS];
  int Count;
};
// Double buffer: ESPUpdateResponse writes to back, DrawESP reads from front
static _Response ResponseBuf[2] = {};
static int g_responseFront = 0; // index DrawESP reads from
#define Response ResponseBuf[g_responseFront]

int Dem(int num) {
  int div = 1;
  while (num != 0) {
    num = num / 10;
    div = div * 10;
  }
  return div;
}

Vector3 VInt2Vector(VInt3 location, VInt3 forward) {
  // Simple fixed-point conversion: VInt3 stores position in milliunits
  float x = (float)location.X / 1000.0f;
  float y = (float)location.Y / 1000.0f;
  float z = (float)location.Z / 1000.0f;
  return Vector3(x, y, z);
}

bool isOutsideScreen(ImVec2 pos, ImVec2 screen) {
  if (pos.y < 0) {
    return true;
  }
  if (pos.x > screen.x) {
    return true;
  }
  if (pos.y > screen.y) {
    return true;
  }
  return pos.x < 0;
}

void DrawCircleHealthAlert(ImVec2 position, int health, int max_health,
                           float radius, ImDrawList *draw) {
  float a_max = ((3.14159265359f * 2.0f));
  ImU32 healthColor = IM_COL32(45, 180, 45, 255);
  if (health <= (max_health * 0.6)) {
    healthColor = IM_COL32(180, 180, 45, 255);
  }
  if (health < (max_health * 0.3)) {
    healthColor = IM_COL32(180, 45, 45, 255);
  }
  draw->PathArcTo(position, radius,
                  (-(a_max / 4.0f)) +
                      (a_max / max_health) * (max_health - health),
                  a_max - (a_max / 4.0f));
  draw->PathStroke(healthColor, ImDrawFlags_None, 6);
}

void DrawCircleHealth(int currentHealth, int maxHealth, float radius,
                      ImVec2 position, ImDrawList *draw) {
  float a_max = ((3.14159265359f * 2.0f));
  draw->PathArcTo(position, radius,
                  (-(a_max / 4.0f)) +
                      (a_max / maxHealth) * (maxHealth - currentHealth),
                  a_max - (a_max / 4.0f));
  draw->PathStroke(ImColor(255, 0, 0, 255), ImDrawFlags_None, 4);
}

void DrawCircleWithImageAndText(ImDrawList *drawList, ImVec2 position,
                                bool isGreen, int number, ImTextureID texture) {
  // Vẽ hình tròn lớn
  float bigCircleRadius = 30.0f;
  drawList->AddCircle(position, bigCircleRadius, IM_COL32(255, 0, 0, 255), 12,
                      2.0f);

  // Nếu có texture, thêm hình ảnh trong hình tròn lớn
  if (texture) {
    drawList->AddImageRounded(
        texture,
        ImVec2(position.x - bigCircleRadius, position.y - bigCircleRadius),
        ImVec2(position.x + bigCircleRadius, position.y + bigCircleRadius),
        ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255),
        bigCircleRadius);

    // Vẽ stroke cho hình ảnh
    drawList->AddCircle(position, bigCircleRadius, IM_COL32(255, 0, 0, 255), 12,
                        2.0f);
  }

  // Vẽ hình tròn nhỏ
  float smallCircleRadius = 12.0f;
  ImVec2 smallCirclePos = ImVec2(position.x, position.y - bigCircleRadius -
                                                 smallCircleRadius + 12.0f);

  if (isGreen) {
    // Nếu isGreen là true, vẽ hình tròn nhỏ màu xanh
    drawList->AddCircleFilled(smallCirclePos, smallCircleRadius,
                              IM_COL32(0, 255, 0, 255), 12);
  } else {
    // Nếu isGreen là false, vẽ hình tròn nhỏ màu xám
    drawList->AddCircleFilled(smallCirclePos, smallCircleRadius,
                              IM_COL32(192, 192, 192, 255), 12);

    // Vẽ số nguyên lên hình tròn nhỏ
    char textBuffer[16];
    snprintf(textBuffer, sizeof(textBuffer), "%d", number);
    ImVec2 textSize = ImGui::CalcTextSize(textBuffer);
    ImVec2 textPos = ImVec2(smallCirclePos.x - textSize.x * 0.5f,
                            smallCirclePos.y - textSize.y * 0.5f);
    drawList->AddText(textPos, IM_COL32(0, 0, 0, 255), textBuffer);
  }

  // Thêm viền ngoài màu đỏ cho hình tròn nhỏ
  drawList->AddCircle(smallCirclePos, smallCircleRadius,
                      IM_COL32(255, 0, 0, 255), 12, 2.0f);
}

ImVec2 pushToScreenBorder(ImVec2 Pos, ImVec2 screen, int offset) {
  int x = (int)Pos.x;
  int y = (int)Pos.y;

  if (Pos.y < 0) {
    y = -offset;
  }

  if (Pos.x > screen.x) {
    x = (int)screen.x + offset;
  }

  if (Pos.y > screen.y) {
    y = (int)screen.y + offset;
  }

  if (Pos.x < 0) {
    x = -offset;
  }
  return ImVec2(x, y);
}

Vector3 Add(Vector3 v1, Vector3 v2) {
  return Vector3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

struct _PlayerESP {
  uintptr_t ActorControl;
  uintptr_t ValueComponent;
  uintptr_t ObjLinker;
};
_PlayerESP PlayerESP{};

void *getMyActorLinker(List<void **> *_GetAllHeros) {
  void **_ActorLinkerItems = (void **)_GetAllHeros->getItems();
  for (int i = 0; i < _GetAllHeros->getSize(); i++) {
    void *_ActorLinker = _ActorLinkerItems[i * 2 + 1];
    if (IsHostPlayer && IsHostPlayer(_ActorLinker)) {
      return _ActorLinker;
    }
  }
  return _GetAllHeros->getItems()[1];
}

// Linker-to-configID map built in ActorLinkerUpdate
struct LinkerMapEntry {
  void *linker; // ActorLinker pointer
  int configID;
  int camp;
  bool bVisible; // cached visibility state
};
static LinkerMapEntry linkerMap[300];
static int linkerMapCount = 0;

void (*_DestroyActor)(void *instance);
void DestroyActor(void *instance) {
  if (instance != NULL) {
    // Reset state when match ends (no more live heroes in response buffer)
    bool anyHero = false;
    int bufCount = ResponseBuf[g_responseFront].Count;
    if (bufCount < 0 || bufCount > MAX_COLLECTED_ACTORS)
      bufCount = 0;
    for (int i = 0; i < bufCount; i++) {
      if (ResponseBuf[g_responseFront].players[i].isHeroUnit &&
          ResponseBuf[g_responseFront].players[i].ActorHP > 0) {
        anyHero = true;
        break;
      }
    }
    if (!anyHero) {
      __android_log_print(ANDROID_LOG_INFO, "CRASH_DBG",
                          "DestroyActor: clearing ESP state instance=%p",
                          instance);
      ResponseBuf[0].Count = 0;
      ResponseBuf[1].Count = 0;
      myPlayerCamp = 0;
      campDetected = false;
      linkerMapCount = 0;
      g_visibleCacheCount = 0;

      campDetectAttempts = 0;
      Lactor = nullptr;
    }
  }
  _DestroyActor(instance);
}

// LGameActorMgr defined above (before Wupdate)
void (*_UpdateLogic_LGameActorMgr)(void *instance, int delta);
void UpdateLogic_LGameActorMgr(void *instance, int delta) {
  if (instance != NULL)
    LGameActorMgr = instance;
  if (instance == NULL) {
    LGameActorMgr = NULL;
    // Match ended — clear ESP
    ResponseBuf[0].Count = 0;
    ResponseBuf[1].Count = 0;
    campDetected = false;
    linkerMapCount = 0;
    g_visibleCacheCount = 0;
    myPlayerCamp = 0;
    Lactor = nullptr;
  }
  _UpdateLogic_LGameActorMgr(instance, delta);
}

// Match end: clear ESP immediately on win/lose
void (*_FightOver_LGameActorMgr)(void *instance);
void FightOver_LGameActorMgr(void *instance) {
  __android_log_print(ANDROID_LOG_INFO, "CRASH_DBG",
                      "FightOver: START instance=%p", instance);
  Lactor = nullptr; // Clear first to prevent other hooks from using stale ptr
  LGameActorMgr = NULL;
  campDetected = false;
  ResponseBuf[0].Count = 0;
  ResponseBuf[1].Count = 0;
  linkerMapCount = 0;
  g_visibleCacheCount = 0;
  myPlayerCamp = 0;
  __android_log_print(ANDROID_LOG_INFO, "CRASH_DBG",
                      "FightOver: calling original");
  _FightOver_LGameActorMgr(instance);
}

// Hook on ActorLinker layer to detect host player camp + cache visibility
void (*_ActorLinkerUpdate)(void *instance, int delta);
void ActorLinkerUpdate(void *instance, int delta) {
  if (instance != NULL) {
    // Camp detection
    // Camp detection — only when in active match
    if (!campDetected && LGameActorMgr && IsHostPlayer && get_objCamp) {
      bool isHost = false;
      try {
        isHost = IsHostPlayer(instance);
      } catch (...) {
      }
      if (isHost) {
        int camp = 0;
        try {
          camp = get_objCamp(instance);
        } catch (...) {
        }
        if (camp > 0) {
          myPlayerCamp = camp;
          campDetected = true;
          Lactor = instance;
          __android_log_print(ANDROID_LOG_INFO, "CAMP_DETECT",
                              "HOST ActorLinker found! camp=%d linker=%p", camp,
                              instance);
        }
      }
    }
    // Build linker→configID map for SetVisible hook matching
    if (PlayerESP.ObjLinker > 0 && campDetected && LGameActorMgr) {
      try {
        int aCamp = get_objCamp ? get_objCamp(instance) : 0;
        if (aCamp >= 0) {
          void *objLinker =
              *(void **)((uint64_t)instance + PlayerESP.ObjLinker);
          if (objLinker && (uint64_t)objLinker > 0x1000000) {
            static uintptr_t cfgIDOff = 0;
            if (cfgIDOff == 0) {
              cfgIDOff = (uintptr_t)GetFieldOffset("Project_d.dll",
                                                   "Assets.Scripts.GameLogic",
                                                   "ActorConfig", "ConfigID");
            }
            int cfgID = cfgIDOff ? *(int *)((uint64_t)objLinker + cfgIDOff) : 0;
            if (cfgID > 0) {
              int slot = -1;
              for (int i = 0; i < linkerMapCount; i++) {
                if (linkerMap[i].linker == instance) {
                  slot = i;
                  break;
                }
              }
              if (slot < 0 && linkerMapCount < 300)
                slot = linkerMapCount++;
              if (slot >= 0) {
                linkerMap[slot].linker = instance;
                linkerMap[slot].configID = cfgID;
                linkerMap[slot].camp = aCamp;
                if (LGameActorMgr && get_bVisible)
                  linkerMap[slot].bVisible = get_bVisible(instance);
                else
                  linkerMap[slot].bVisible = false;
              }
            }
          }
        }
      } catch (...) {
      }
    }
  }
  _ActorLinkerUpdate(instance, delta);
}

bool checkdotinrange(ImVec2 pos, ImVec2 dot1, ImVec2 dot2, ImVec2 dot3,
                     ImVec2 dot4) {
  if ((pos.x >= dot1.x) && (pos.x <= dot2.x) && (pos.y >= dot1.y) &&
      (pos.y <= dot4.y))
    return true;
  return false;
}

Vector2 MinimapScale, BigmapScale, MinimapPosSc, MinimapScSize;
static float minimapPosX = 41.5f;
static float minimapPosY = 75.5f;
static float minimapScale = 3.1f;
void (*_MiniMapSys)(void *instance);
void MiniMapSys(void *instance) {
  if (instance != NULL) {
    MinimapScale = get_MinimapScale(instance);
    BigmapScale = get_BigMapScale(instance);
    MinimapScSize = get_mmFinalScreenSize(instance);
    MinimapPosSc = GetMMFianlScreenPos(instance);
  }
  _MiniMapSys(instance);
}

void drawTextInt(ImVec2 position, int value, ImDrawList *draw) {
  char format_text[1024];
  sprintf(format_text, "%d", value);
  draw->AddText({(position.x - ImGui::CalcTextSize(format_text).x / 2.0f),
                 (position.y - ImGui::CalcTextSize(format_text).y / 2.0f)},
                ImGui::ColorConvertFloat4ToU32({1, 1, 1, 1.0f}), format_text);
}
float (*_GetCameraHeightRateValue)(...);
float GetCameraHeightRateValue(void *huhu, int a1) {
  if (huhu != NULL && Camera.V1.Value > 0)
    return 1 + (Camera.V1.Value - 1) * 0.05f;
  return _GetCameraHeightRateValue(huhu, a1);
}

void (*_OnCameraHeightChanged)(void *instance);
void OnCameraHeightChanged(void *instance) { _OnCameraHeightChanged(instance); }
static int dbg_esp_step = 7;
static int dbg_lHeros_size = 0;
static bool dbg_showAllIDs = false;

void (*_ESPUpdateResponse)(void *instance);
void ESPUpdateResponse(void *instance) {
  if (instance == NULL)
    goto esp_done;

  {
    // === Resolve LActorRoot field offsets (once, auto-update after patch) ===
    static uintptr_t g_LAR_locationOff = 0;  // LActorRoot::_location (VInt3)
    static uintptr_t g_LAR_actorMetaOff = 0; // LActorRoot::TheActorMeta
    static uintptr_t g_LAR_valueCompOff = 0; // LActorRoot::ValueComponent
    static uintptr_t g_LAR_objLinkerOff =
        0; // LActorRoot::ObjLinker (LVActorLinker)
    static bool g_larOffsetsResolved = false;
    static int (*LAR_GiveMyEnemyCamp)(void *) = nullptr;
    static bool (*LAR_get_IsDeadState_local)(void *) = nullptr;

    if (!g_larOffsetsResolved) {
      g_LAR_locationOff = (uintptr_t)GetFieldOffset("Project.Plugins_d.dll",
                                                    "NucleusDrive.Logic",
                                                    "LActorRoot", "_location");
      if (!g_LAR_locationOff)
        g_LAR_locationOff = (uintptr_t)GetFieldOffset("Project.Plugins_d.dll",
                                                      "NucleusDrive.Logic",
                                                      "LActorRoot", "location");
      g_LAR_actorMetaOff = (uintptr_t)GetFieldOffset(
          "Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot",
          "TheActorMeta");
      g_LAR_valueCompOff = (uintptr_t)GetFieldOffset(
          "Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot",
          "ValueComponent");
      g_LAR_objLinkerOff = (uintptr_t)GetFieldOffset("Project.Plugins_d.dll",
                                                     "NucleusDrive.Logic",
                                                     "LActorRoot", "ObjLinker");
      LAR_GiveMyEnemyCamp = (int (*)(void *))GetMethodOffset(
          "Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot",
          "GiveMyEnemyCamp", 0);
      LAR_get_IsDeadState_local = (bool (*)(void *))GetMethodOffset(
          "Project.Plugins_d.dll", "NucleusDrive.Logic", "LObjWrapper",
          "get_IsDeadState", 0);
      g_larOffsetsResolved = true;
      __android_log_print(
          ANDROID_LOG_INFO, "ESP_LAR",
          "Offsets: loc=0x%lx meta=0x%lx val=0x%lx linker=0x%lx",
          (unsigned long)g_LAR_locationOff, (unsigned long)g_LAR_actorMetaOff,
          (unsigned long)g_LAR_valueCompOff, (unsigned long)g_LAR_objLinkerOff);
    }

    // === Get hero list from LGameActorMgr (returns LActorRoot objects) ===
    void *heroListRaw = nullptr;
    if (LGameActorMgr && GetAllHeros_LGameActorMgr) {
      heroListRaw = (void *)GetAllHeros_LGameActorMgr(LGameActorMgr);
    }

    // Fallback: try ActorManager (ActorLinker layer) if LGameActorMgr
    // unavailable
    bool usingActorLinkerFallback = false;
    if (!heroListRaw) {
      void *mgr = get_actorManager ? get_actorManager() : nullptr;
      if (mgr && GetAllHeros_ActorManager) {
        heroListRaw = (void *)GetAllHeros_ActorManager(mgr);
        usingActorLinkerFallback = true;
      }
    }

    if (!heroListRaw) {
      // Match likely ended — clear ESP state
      if (ResponseBuf[0].Count > 0 || ResponseBuf[1].Count > 0) {
        ResponseBuf[0].Count = 0;
        ResponseBuf[1].Count = 0;
        campDetected = false;
        linkerMapCount = 0;
        g_visibleCacheCount = 0;

        myPlayerCamp = 0;
        Lactor = nullptr;
      }
      goto esp_done;
    }

    // List<T> layout: [0x08] = _items array ptr, [0x10] = _size
    void *arrPtr = *(void **)((uint64_t)heroListRaw + 0x08);
    int listSize = *(int *)((uint64_t)heroListRaw + 0x10);

    if (!arrPtr || (uint64_t)arrPtr < 0x1000000 || listSize <= 0)
      goto esp_done;

    void *cam = (get_camera && worldToScreen) ? get_camera() : nullptr;
    if (!cam)
      goto esp_done;

    int backIdx = 1 - g_responseFront;
    _Response &back = ResponseBuf[backIdx];
    int newCount = 0;
    float now = GetTimeSeconds();

    // === ActorLinker fallback path (old behavior) ===
    if (usingActorLinkerFallback) {
      void **items = (void **)((uint64_t)arrPtr + 0x18);
      static int (*AL_get_configId)(void *) = nullptr;
      static bool g_alResolved = false;
      if (!g_alResolved) {
        AL_get_configId = (int (*)(void *))GetMethodOffset(
            "Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_configId", 0);
        if (!AL_get_configId)
          AL_get_configId = (int (*)(void *))GetMethodOffset(
              "Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_ConfigId",
              0);
        if (!AL_get_configId)
          AL_get_configId = (int (*)(void *))GetMethodOffset(
              "Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_configID",
              0);
        if (!AL_get_configId)
          AL_get_configId = (int (*)(void *))GetMethodOffset(
              "Project_d.dll", "Kyrios.Actor", "ActorLinker", "get_ConfigID",
              0);
        g_alResolved = true;
      }
      for (int i = 0; i < listSize && newCount < MAX_COLLECTED_ACTORS; i++) {
        void *actorLinker = items[i * 2 + 1];
        if (!actorLinker || (uint64_t)actorLinker < 0x1000000)
          continue;
        uint64_t vtable = *(uint64_t *)actorLinker;
        if ((vtable & 0x0000FFFFFFFFFFFF) < 0x1000000)
          continue;
        try {
          int camp = get_objCamp ? get_objCamp(actorLinker) : 0;
          if (camp <= 0)
            continue;
          bool isHost = IsHostPlayer ? IsHostPlayer(actorLinker) : false;
          if (isHost && !campDetected) {
            myPlayerCamp = camp;
            campDetected = true;
            Lactor = actorLinker;
          }
          if (isHost)
            continue;
          bool isEnemy = (campDetected && myPlayerCamp > 0)
                             ? (camp != myPlayerCamp)
                             : true;
          int configID = AL_get_configId ? AL_get_configId(actorLinker) : 0;
          if (configID == 0)
            continue;
          if (configID >= 7500 && configID <= 7521)
            continue;
          Vector3 pos =
              get_position ? get_position(actorLinker) : Vector3{0, 0, 0};
          if (pos.x == 0 && pos.y == 0 && pos.z == 0)
            continue;
          Vector3 sc = worldToScreen(cam, pos);
          if (sc.z <= 0)
            continue;
          if (sc.x < -2000 || sc.x > glWidth + 2000)
            continue;
          if (sc.y < -2000 || sc.y > glHeight + 2000)
            continue;
          int idx = newCount;
          back.players[idx].ConfigID = configID;
          back.players[idx].ActorHP = 1;
          back.players[idx].ActorHPTotal = 1;
          back.players[idx].ActorMaxHP = 1;
          back.players[idx].ActorLevel = 0;
          back.players[idx].isEnemy = isEnemy;
          back.players[idx].isHeroUnit = true;
          back.players[idx].actorPtr = actorLinker;
          back.players[idx].linkerPtr = actorLinker;
          back.players[idx].EnemyCamp = camp;
          back.players[idx].Position = pos;
          back.players[idx].PrevPosition = pos;
          back.players[idx].PositionSc = sc;
          back.players[idx].lastUpdateTime = now;
          back.players[idx].Visible =
              get_bVisible ? get_bVisible(actorLinker) : false;
          newCount++;
        } catch (...) {
          continue;
        }
      }
      goto esp_finalize;
    }

    // === PRIMARY PATH: LActorRoot layer (real-time position) ===
    {
      // PoolObjHandle<LActorRoot> layout in array: [handleSeq(4) + pad(4) +
      // handleObj(8)] = 16 bytes per element Array object: +0x10 = length,
      // +0x18 = first element (after Il2Cpp array header) Actually for
      // List<T>._items: items array is a managed array Managed array on arm64
      // il2cpp: [klass(8) + monitor(8) + bounds(8) + max_length(8)] then
      // elements Elements start at arrPtr + 0x20 Each PoolObjHandle<LActorRoot>
      // = 16 bytes: [uint _handleSeq (4)] [pad (4)] [T* _handleObj (8)]

      // Debug: log once per second
      static float g_lastDbgLog = 0;
      float nowDbg = GetTimeSeconds();
      if (nowDbg - g_lastDbgLog > 2.0f) {
        g_lastDbgLog = nowDbg;
        __android_log_print(ANDROID_LOG_INFO, "ESP_LAR",
                            "LGameActorMgr=%p heroList=%p arrPtr=%p "
                            "listSize=%d campDet=%d myCamp=%d",
                            LGameActorMgr, heroListRaw, arrPtr, listSize,
                            campDetected, myPlayerCamp);
        // Log first element raw bytes
        if (listSize > 0) {
          uint64_t elem0 = (uint64_t)arrPtr + 0x18;
          void *obj0_at0 = *(void **)(elem0 + 0x00);
          void *obj0_at8 = *(void **)(elem0 + 0x08);
          uint32_t raw4 = *(uint32_t *)(elem0 + 0x04);
          __android_log_print(ANDROID_LOG_INFO, "ESP_LAR",
                              "elem[0]: at0=%p at8=%p raw4=0x%x", obj0_at0,
                              obj0_at8, raw4);
          void *obj0 =
              obj0_at8; // object at +8 (matches old items[i*2+1] pattern)
          if ((uint64_t)obj0 < 0x1000000)
            obj0 = obj0_at0; // fallback to +0
          if (obj0 && (uint64_t)obj0 > 0x1000000) {
            uint64_t vt = *(uint64_t *)obj0;
            VInt3 rawLoc = *(VInt3 *)((uint64_t)obj0 + g_LAR_locationOff);
            uint64_t metaB = (uint64_t)obj0 + g_LAR_actorMetaOff;
            int id0 = *(int *)(metaB + 0x00);
            int id0b = *(int *)(metaB - 0x08);
            int camp0a = *(int *)(metaB + 0x14);
            int camp0b = *(int *)(metaB + 0x0C);
            __android_log_print(ANDROID_LOG_INFO, "ESP_LAR",
                                "elem[0] vt=%p loc={%d,%d,%d} meta+0=%d "
                                "meta-8=%d camp+14=%d camp+C=%d",
                                (void *)vt, rawLoc.X, rawLoc.Y, rawLoc.Z, id0,
                                id0b, camp0a, camp0b);
          }
        }
      }

      for (int i = 0; i < listSize && newCount < MAX_COLLECTED_ACTORS; i++) {
        // Read LActorRoot pointer from PoolObjHandle
        uint64_t elemBase = (uint64_t)arrPtr + 0x18 + (i * 0x10);
        void *actorRoot =
            *(void **)(elemBase +
                       0x08); // _handleObj at +8 (matches items[i*2+1])
        if (!actorRoot || (uint64_t)actorRoot < 0x1000000)
          actorRoot = *(void **)(elemBase + 0x00); // fallback: try +0

        if (!actorRoot || (uint64_t)actorRoot < 0x1000000)
          continue;

        // Validate vtable
        uint64_t vtable = *(uint64_t *)actorRoot;
        if ((vtable & 0x0000FFFFFFFFFFFF) < 0x1000000)
          continue;

        try {
          // === Read _location (VInt3) directly from LActorRoot field —
          // REAL-TIME ===
          VInt3 loc = {0, 0, 0};
          if (g_LAR_locationOff > 0 && g_LAR_locationOff < 0x500) {
            loc = *(VInt3 *)((uint64_t)actorRoot + g_LAR_locationOff);
          } else if (get_location) {
            loc = get_location(actorRoot);
          }

          // Convert VInt3 to Vector3 (milliunits → units)
          Vector3 pos;
          pos.x = (float)loc.X / 1000.0f;
          pos.y = (float)loc.Y / 1000.0f;
          pos.z = (float)loc.Z / 1000.0f;

          if (pos.x == 0.0f && pos.y == 0.0f && pos.z == 0.0f)
            continue;

          // === Read ConfigId and ActorCamp from TheActorMeta ===
          int configID = 0;
          int actorCamp = 0;
          if (g_LAR_actorMetaOff > 0 && g_LAR_actorMetaOff < 0x500) {
            uint64_t metaBase = (uint64_t)actorRoot + g_LAR_actorMetaOff;
            // GetFieldOffset may return offset to struct start (ConfigId at +0)
            // or to internal ref point (ConfigId at -8). Try +0 first,
            // validate.
            int tryId = *(int *)(metaBase + 0x00);
            if (tryId >= 100 && tryId < 10000) {
              // Looks like ConfigId is at struct start
              configID = tryId;
              actorCamp = *(int *)(metaBase + 0x14); // ActorCamp
            } else {
              // Try dump-style: ConfigId at -8 from ref
              tryId = *(int *)(metaBase - 0x08);
              if (tryId >= 100 && tryId < 10000) {
                configID = tryId;
                actorCamp = *(int *)(metaBase + 0x0C); // ActorCamp
              }
            }
          }

          if (configID == 0)
            continue;
          if (configID >= 7500 && configID <= 7521)
            continue; // filter non-hero

          // Camp detection from LActorRoot
          int camp = actorCamp;
          if (camp <= 0 && LAR_GiveMyEnemyCamp) {
            camp = LAR_GiveMyEnemyCamp(actorRoot);
          }
          if (camp <= 0)
            continue;

          // Host/ally detection: skip actors on our team
          // Camp is already detected by ActorLinkerUpdate hook
          // If not yet detected, try to detect from first hero with valid camp
          if (!campDetected) {
            // Can't determine friend/foe yet — try ActorLinkerUpdate hook first
            // For now, assume first iteration; ActorLinkerUpdate will set
            // campDetected Skip this frame if camp not detected
            continue;
          }

          // Skip allies (same camp as host player)
          // DEBUG: temporarily disabled to test on self
          // if (camp == myPlayerCamp)
          //   continue;

          bool isEnemy = (camp != myPlayerCamp);

          // === Smooth position: lerp from previous frame to reduce jitter ===
          Vector3 prevPos = {0, 0, 0};
          float prevTime = 0;
          for (int p = 0; p < ResponseBuf[g_responseFront].Count; p++) {
            if (ResponseBuf[g_responseFront].players[p].ConfigID == configID) {
              prevPos = ResponseBuf[g_responseFront].players[p].Position;
              prevTime = ResponseBuf[g_responseFront].players[p].lastUpdateTime;
              break;
            }
          }

          // Smooth position: lerp from previous frame
          if (prevPos.x != 0 || prevPos.y != 0 || prevPos.z != 0) {
            float dx = pos.x - prevPos.x;
            float dy = pos.y - prevPos.y;
            float dz = pos.z - prevPos.z;
            float distSq = dx * dx + dy * dy + dz * dz;
            // Small movement: lerp for smoothness
            if (distSq > 0.01f && distSq < 2500.0f) {
              float elapsed = now - prevTime;
              float t = elapsed / g_lerpInterval;
              if (t > 1.0f)
                t = 1.0f;
              if (t < 0.0f)
                t = 0.0f;
              pos = LerpVec3(prevPos, pos, t);
            }
            // distSq >= 2500 = teleport/respawn, accept new pos as-is
          }

          // === WorldToScreen ===
          Vector3 sc = worldToScreen(cam, pos);
          // Don't filter here — store all actors, let DrawESP handle visibility
          // sc.z <= 0 means behind camera, but actor still exists and should be
          // tracked

          // === Read HP/Level from ValueComponent ===
          int hp = 1, maxhp = 1, level = 0;
          if (g_LAR_valueCompOff > 0 && g_LAR_valueCompOff < 0x500) {
            void *valComp =
                *(void **)((uint64_t)actorRoot + g_LAR_valueCompOff);
            if (valComp && (uint64_t)valComp > 0x1000000) {
              if (g_hp)
                hp = g_hp(valComp);
              if (g_maxhp)
                maxhp = g_maxhp(valComp);
              if (g_level)
                level = g_level(valComp);
            }
          }

          // === Read visibility from linkerMap (match by configID + enemy camp)
          // ===
          bool visible = false;
          for (int lm = 0; lm < linkerMapCount; lm++) {
            if (linkerMap[lm].configID == configID &&
                linkerMap[lm].camp == camp) {
              visible = linkerMap[lm].bVisible;
              break;
            }
          }

          // === Fill response buffer ===
          int idx = newCount;
          back.players[idx].ConfigID = configID;
          back.players[idx].ActorHP = hp;
          back.players[idx].ActorHPTotal = maxhp;
          back.players[idx].ActorMaxHP = maxhp;
          back.players[idx].ActorLevel = level;
          back.players[idx].isEnemy = isEnemy;
          back.players[idx].isHeroUnit = (configID >= 100 && configID < 700);
          back.players[idx].actorPtr = actorRoot;
          back.players[idx].linkerPtr = actorRoot;
          back.players[idx].EnemyCamp = camp;
          back.players[idx].Position = pos;
          back.players[idx].PrevPosition = prevPos;
          back.players[idx].PositionSc = sc;
          back.players[idx].lastUpdateTime = now;
          back.players[idx].Visible = visible;

          // === Read skill CD data from GetHeroWrapSkillData ===
          if (GetHeroWrapSkillData) {
            // GetHeroWrapSkillData is a method on LHeroWrapper, not LActorRoot
            // Use LActorRoot::AsHero() to get LHeroWrapper pointer
            static void *(*AsHero_fn)(void *) = nullptr;
            static bool asHeroResolved = false;
            if (!asHeroResolved) {
              AsHero_fn = (void *(*)(void *))GetMethodOffset(
                  "Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot",
                  "AsHero", 0);
              asHeroResolved = true;
            }
            void *heroWrapper = AsHero_fn ? AsHero_fn(actorRoot) : nullptr;
            if (heroWrapper && (uint64_t)heroWrapper > 0x1000000) {
              HeroWrapSkillData s1 = GetHeroWrapSkillData(heroWrapper, 1);
              HeroWrapSkillData s2 = GetHeroWrapSkillData(heroWrapper, 2);
              HeroWrapSkillData s3 = GetHeroWrapSkillData(heroWrapper, 3);
              HeroWrapSkillData talent = GetHeroWrapSkillData(heroWrapper, 5);

              back.players[idx].Skill1CD = s1.Skill1SlotCD;
              back.players[idx].Skill2CD = s2.Skill1SlotCD;
              back.players[idx].Skill3CD = s3.Skill1SlotCD;
              back.players[idx].Skill1Unlock = s1.skillSlotUnlock;
              back.players[idx].Skill2Unlock = s2.skillSlotUnlock;
              back.players[idx].Skill3Unlock = s3.skillSlotUnlock;
              back.players[idx].Skill1Level = s1.skillLv;
              back.players[idx].Skill2Level = s2.skillLv;
              back.players[idx].Skill3Level = s3.skillLv;
              back.players[idx].TalentCD = talent.Skill1SlotCD;
              back.players[idx].TalentSkillId = talent.SkillId;

              // Item active (slot 7)
              HeroWrapSkillData itemActive = GetHeroWrapSkillData(heroWrapper, 7);
              back.players[idx].ItemActiveCD = itemActive.Skill1SlotCD;
              back.players[idx].ItemActiveSkillId = itemActive.SkillId;

              // DEBUG: log all slots to find item active
              static float g_lastSlotLog = 0;
              if (now - g_lastSlotLog > 2.0f && idx == 0) {
                g_lastSlotLog = now;
                for (int sl = 0; sl <= 12; sl++) {
                  HeroWrapSkillData sd = GetHeroWrapSkillData(heroWrapper, sl);
                  if (sd.SkillId > 0 || sd.Skill1SlotCD > 0) {
                    __android_log_print(ANDROID_LOG_INFO, "SLOT_DBG",
                        "slot=%d id=%u cd=%d lv=%d unlock=%d",
                        sl, sd.SkillId, sd.Skill1SlotCD, sd.skillLv, sd.skillSlotUnlock);
                  }
                }
              }
            }
          }

          newCount++;
        } catch (...) {
          continue;
        }
      }
    }

    // === JUNGLE MONSTERS + ORGANS (Dragon/Slayer) ===
    if (LGameActorMgr && campDetected && g_LAR_locationOff > 0) {
      for (int listType = 0; listType < 2 && newCount < MAX_COLLECTED_ACTORS;
           listType++) {
        void *monsterListRaw = nullptr;
        if (listType == 0 && GetAllJungleMonsters_LGameActorMgr)
          monsterListRaw =
              (void *)GetAllJungleMonsters_LGameActorMgr(LGameActorMgr);
        else if (listType == 1 && GetAllMonsters_LGameActorMgr)
          monsterListRaw = (void *)GetAllMonsters_LGameActorMgr(LGameActorMgr);
        if (!monsterListRaw)
          continue;

        void *mArrPtr = *(void **)((uint64_t)monsterListRaw + 0x08);
        int mListSize = *(int *)((uint64_t)monsterListRaw + 0x10);
        if (!mArrPtr || mListSize <= 0 || mListSize > 200)
          continue;

        for (int i = 0; i < mListSize && newCount < MAX_COLLECTED_ACTORS; i++) {
          uint64_t elemBase = (uint64_t)mArrPtr + 0x18 + (i * 0x10);
          void *actorRoot = *(void **)(elemBase + 0x08);
          if (!actorRoot || (uint64_t)actorRoot < 0x1000000)
            continue;
          try {
            VInt3 loc = *(VInt3 *)((uint64_t)actorRoot + g_LAR_locationOff);
            if (loc.X == 0 && loc.Y == 0 && loc.Z == 0)
              continue;
            Vector3 pos = {(float)loc.X / 1000.0f, (float)loc.Y / 1000.0f,
                           (float)loc.Z / 1000.0f};

            int configID = 0, actorCamp = 0;
            if (g_LAR_actorMetaOff > 0 && g_LAR_actorMetaOff < 0x500) {
              uint64_t metaBase = (uint64_t)actorRoot + g_LAR_actorMetaOff;
              int tryId = *(int *)(metaBase + 0x00);
              if (tryId >= 1 && tryId < 100000) {
                configID = tryId;
                actorCamp = *(int *)(metaBase + 0x14);
              } else {
                tryId = *(int *)(metaBase - 0x08);
                if (tryId >= 1 && tryId < 100000) {
                  configID = tryId;
                  actorCamp = *(int *)(metaBase + 0x0C);
                }
              }
            }
            if (configID == 0)
              continue;
            if (actorCamp == myPlayerCamp && actorCamp > 0)
              continue;

            int hp = 0, maxhp = 1;
            if (g_LAR_valueCompOff > 0 && g_LAR_valueCompOff < 0x500) {
              void *valComp =
                  *(void **)((uint64_t)actorRoot + g_LAR_valueCompOff);
              if (valComp && (uint64_t)valComp > 0x1000000) {
                if (g_hp)
                  hp = g_hp(valComp);
                if (g_maxhp)
                  maxhp = g_maxhp(valComp);
              }
            }
            if (hp <= 0)
              continue;
            // Filter dying actors (death animation, HP not yet zeroed)
            if (get_IsDeadState && get_IsDeadState(actorRoot))
              continue;

            // Lookup visibility from SetVisible hook cache via ObjLinker
            bool visible = false;
            if (g_LAR_objLinkerOff > 0 && g_LAR_objLinkerOff < 0x500) {
              void *objLinker =
                  *(void **)((uint64_t)actorRoot + g_LAR_objLinkerOff);
              if (objLinker && (uint64_t)objLinker > 0x1000000) {
                for (int vc = 0; vc < g_visibleCacheCount; vc++) {
                  if (g_visibleCache[vc].linkerPtr == objLinker) {
                    visible = g_visibleCache[vc].visible;
                    break;
                  }
                }
              }
            }

            Vector3 sc = worldToScreen(cam, pos);
            int idx = newCount;
            back.players[idx].ConfigID = configID;
            back.players[idx].ActorHP = hp;
            back.players[idx].ActorHPTotal = maxhp;
            back.players[idx].ActorMaxHP = maxhp;
            back.players[idx].ActorLevel = 0;
            back.players[idx].isEnemy = true;
            back.players[idx].isHeroUnit = false;
            back.players[idx].actorPtr = actorRoot;
            back.players[idx].linkerPtr = actorRoot;
            back.players[idx].EnemyCamp = actorCamp;
            back.players[idx].Position = pos;
            back.players[idx].PrevPosition = pos;
            back.players[idx].PositionSc = sc;
            back.players[idx].lastUpdateTime = now;
            back.players[idx].Visible = visible;
            newCount++;
          } catch (...) {
            continue;
          }
        }
      }
    }

  esp_finalize: {
    if (newCount > 0) {
      back.Count = newCount;
      dbg_lHeros_size = newCount;
      g_responseFront = backIdx;
    } else if (campDetected) {
      // Camp detected but no enemies found — match likely ended, clear
      // immediately
      ResponseBuf[0].Count = 0;
      ResponseBuf[1].Count = 0;
      dbg_lHeros_size = 0;
    }
    // If !campDetected, keep old buffer (waiting for camp detection)
  }
  }

esp_done:
  if (Camera.V1.Enable && _OnCameraHeightChanged) {
    OnCameraHeightChanged(instance);
  }
  if (_ESPUpdateResponse)
    _ESPUpdateResponse(instance);
}

TextureInfo HeroImage[700];

void GetIconHero() {
  HeroImage[105] = createTexturePNGFromMem(Toro_h, sizeof(Toro_h));
  HeroImage[106] = createTexturePNGFromMem(Krixi_h, sizeof(Krixi_h));
  HeroImage[107] = createTexturePNGFromMem(Zephys_h, sizeof(Zephys_h));
  HeroImage[108] = createTexturePNGFromMem(Gildur_h, sizeof(Gildur_h));
  HeroImage[109] = createTexturePNGFromMem(Veera_h, sizeof(Veera_h));
  HeroImage[110] = createTexturePNGFromMem(Kahi_h, sizeof(Kahi_h));
  HeroImage[111] = createTexturePNGFromMem(Violet_h, sizeof(Violet_h));
  HeroImage[112] = createTexturePNGFromMem(Yorn_h, sizeof(Yorn_h));
  HeroImage[113] = createTexturePNGFromMem(Chaugnar_h, sizeof(Chaugnar_h));
  HeroImage[114] = createTexturePNGFromMem(Omega_h, sizeof(Omega_h));
  HeroImage[115] = createTexturePNGFromMem(Jinna_h, sizeof(Jinna_h));
  HeroImage[116] = createTexturePNGFromMem(Butterfly_h, sizeof(Butterfly_h));
  HeroImage[117] = createTexturePNGFromMem(Ormarr_h, sizeof(Ormarr_h));
  HeroImage[118] = createTexturePNGFromMem(Alice_h, sizeof(Alice_h));
  HeroImage[119] = createTexturePNGFromMem(Mganga_h, sizeof(Mganga_h));
  HeroImage[120] = createTexturePNGFromMem(Mina_h, sizeof(Mina_h));
  HeroImage[121] = createTexturePNGFromMem(Marja_h, sizeof(Marja_h));
  HeroImage[123] = createTexturePNGFromMem(Maloch_h, sizeof(Maloch_h));
  HeroImage[124] = createTexturePNGFromMem(Ignis_h, sizeof(Ignis_h));
  HeroImage[126] = createTexturePNGFromMem(Arduin_h, sizeof(Arduin_h));
  HeroImage[127] = createTexturePNGFromMem(AzzenKa_h, sizeof(AzzenKa_h));
  HeroImage[128] = createTexturePNGFromMem(LuBo_h, sizeof(LuBo_h));
  HeroImage[129] = createTexturePNGFromMem(TrieuVan_h, sizeof(TrieuVan_h));
  HeroImage[130] = createTexturePNGFromMem(Airi_h, sizeof(Airi_h));
  HeroImage[131] = createTexturePNGFromMem(Murad_h, sizeof(Murad_h));
  HeroImage[132] = createTexturePNGFromMem(Hayate_h, sizeof(Hayate_h));
  HeroImage[133] = createTexturePNGFromMem(Valhein_h, sizeof(Valhein_h));
  HeroImage[134] = createTexturePNGFromMem(Skud_h, sizeof(Skud_h));
  HeroImage[135] = createTexturePNGFromMem(Thane_h, sizeof(Thane_h));
  HeroImage[136] = createTexturePNGFromMem(Ilumia_h, sizeof(Ilumia_h));
  HeroImage[137] = createTexturePNGFromMem(Paine_h, sizeof(Paine_h));
  HeroImage[139] = createTexturePNGFromMem(KilGroth_h, sizeof(KilGroth_h));
  HeroImage[140] = createTexturePNGFromMem(SuperMan_h, sizeof(SuperMan_h));
  HeroImage[141] = createTexturePNGFromMem(Lauriel_h, sizeof(Lauriel_h));
  HeroImage[142] = createTexturePNGFromMem(Natalya_h, sizeof(Natalya_h));
  HeroImage[144] = createTexturePNGFromMem(Taara_h, sizeof(Taara_h));
  HeroImage[146] = createTexturePNGFromMem(Zill_h, sizeof(Zill_h));
  HeroImage[148] = createTexturePNGFromMem(Preyta_h, sizeof(Preyta_h));
  HeroImage[149] = createTexturePNGFromMem(Xeniel_h, sizeof(Xeniel_h));
  HeroImage[150] = createTexturePNGFromMem(Nakroth_h, sizeof(Nakroth_h));
  HeroImage[152] = createTexturePNGFromMem(DieuThuyen_h, sizeof(DieuThuyen_h));
  HeroImage[153] = createTexturePNGFromMem(Kaine_h, sizeof(Kaine_h));
  HeroImage[154] = createTexturePNGFromMem(Yena_h, sizeof(Yena_h));
  HeroImage[156] = createTexturePNGFromMem(Aleister_h, sizeof(Aleister_h));
  HeroImage[157] = createTexturePNGFromMem(Raz_h, sizeof(Raz_h));
  HeroImage[162] = createTexturePNGFromMem(Kriknak_h, sizeof(Kriknak_h));
  HeroImage[163] = createTexturePNGFromMem(Ryoma_h, sizeof(Ryoma_h));
  HeroImage[166] = createTexturePNGFromMem(Arthur_h, sizeof(Arthur_h));
  HeroImage[167] = createTexturePNGFromMem(NgoKhong_h, sizeof(NgoKhong_h));
  HeroImage[168] = createTexturePNGFromMem(Lumburr_h, sizeof(Lumburr_h));
  HeroImage[169] = createTexturePNGFromMem(Slimz_h, sizeof(Slimz_h));
  HeroImage[170] = createTexturePNGFromMem(Moren_h, sizeof(Moren_h));
  HeroImage[171] = createTexturePNGFromMem(Cresht_h, sizeof(Cresht_h));
  HeroImage[173] = createTexturePNGFromMem(Fennik_h, sizeof(Fennik_h));
  HeroImage[174] = createTexturePNGFromMem(Joker_h, sizeof(Joker_h));
  HeroImage[175] = createTexturePNGFromMem(Grakk_h, sizeof(Grakk_h));
  HeroImage[177] = createTexturePNGFromMem(Lindis_h, sizeof(Lindis_h));
  HeroImage[180] = createTexturePNGFromMem(Max_h, sizeof(Max_h));
  HeroImage[184] = createTexturePNGFromMem(Helen_h, sizeof(Helen_h));
  HeroImage[186] = createTexturePNGFromMem(TeeMee_h, sizeof(TeeMee_h));
  HeroImage[187] = createTexturePNGFromMem(Arum_h, sizeof(Arum_h));
  HeroImage[189] = createTexturePNGFromMem(Krizzix_h, sizeof(Krizzix_h));
  HeroImage[190] = createTexturePNGFromMem(Tulen_h, sizeof(Tulen_h));
  HeroImage[191] = createTexturePNGFromMem(Rouie_h, sizeof(Rouie_h));
  HeroImage[192] = createTexturePNGFromMem(Celica_h, sizeof(Celica_h));
  HeroImage[193] = createTexturePNGFromMem(Amily_h, sizeof(Amily_h));
  HeroImage[194] = createTexturePNGFromMem(Wiro_h, sizeof(Wiro_h));
  HeroImage[195] = createTexturePNGFromMem(Enzo_h, sizeof(Enzo_h));
  HeroImage[196] = createTexturePNGFromMem(Elsu_h, sizeof(Elsu_h));
  HeroImage[199] = createTexturePNGFromMem(Elandorr_h, sizeof(Elandorr_h));
  HeroImage[501] = createTexturePNGFromMem(TelAnnas_h, sizeof(TelAnnas_h));
  HeroImage[502] = createTexturePNGFromMem(Asrid_h, sizeof(Asrid_h));
  HeroImage[503] = createTexturePNGFromMem(Zuka_h, sizeof(Zuka_h));
  HeroImage[504] =
      createTexturePNGFromMem(WonderWoman_h, sizeof(WonderWoman_h));
  HeroImage[505] = createTexturePNGFromMem(Baldum_h, sizeof(Baldum_h));
  HeroImage[506] = createTexturePNGFromMem(Omen_h, sizeof(Omen_h));
  HeroImage[507] = createTexturePNGFromMem(Flash_h, sizeof(Flash_h));
  HeroImage[508] = createTexturePNGFromMem(Wisp_h, sizeof(Wisp_h));
  HeroImage[509] = createTexturePNGFromMem(Ybneth_h, sizeof(Ybneth_h));
  HeroImage[510] = createTexturePNGFromMem(Liliana_h, sizeof(Liliana_h));
  HeroImage[511] = createTexturePNGFromMem(Ata_h, sizeof(Ata_h));
  HeroImage[512] = createTexturePNGFromMem(Rourke_h, sizeof(Rourke_h));
  HeroImage[513] = createTexturePNGFromMem(Zata_h, sizeof(Zata_h));
  HeroImage[514] = createTexturePNGFromMem(Roxie_h, sizeof(Roxie_h));
  HeroImage[515] = createTexturePNGFromMem(Richter_h, sizeof(Richter_h));
  HeroImage[518] = createTexturePNGFromMem(Quillen_h, sizeof(Quillen_h));
  HeroImage[519] = createTexturePNGFromMem(Annette_h, sizeof(Annette_h));
  HeroImage[520] = createTexturePNGFromMem(Veres_h, sizeof(Veres_h));
  HeroImage[521] = createTexturePNGFromMem(Florentino_h, sizeof(Florentino_h));
  HeroImage[522] = createTexturePNGFromMem(Errol_h, sizeof(Errol_h));
  HeroImage[523] = createTexturePNGFromMem(Darcy_h, sizeof(Darcy_h));
  HeroImage[524] = createTexturePNGFromMem(Capheny_h, sizeof(Capheny_h));
  HeroImage[525] = createTexturePNGFromMem(Zip_h, sizeof(Zip_h));
  HeroImage[526] = createTexturePNGFromMem(Ishar_h, sizeof(Ishar_h));
  HeroImage[527] = createTexturePNGFromMem(Sephera_h, sizeof(Sephera_h));
  HeroImage[528] = createTexturePNGFromMem(Qi_h, sizeof(Qi_h));
  HeroImage[529] = createTexturePNGFromMem(Volkath_h, sizeof(Volkath_h));
  HeroImage[530] = createTexturePNGFromMem(Dirak_h, sizeof(Dirak_h));
  HeroImage[531] = createTexturePNGFromMem(Keera_h, sizeof(Keera_h));
  HeroImage[532] = createTexturePNGFromMem(Thorne_h, sizeof(Thorne_h));
  HeroImage[533] = createTexturePNGFromMem(Laville_h, sizeof(Laville_h));
  HeroImage[534] = createTexturePNGFromMem(Dextra_h, sizeof(Dextra_h));
  HeroImage[535] = createTexturePNGFromMem(Sinestrea_h, sizeof(Sinestrea_h));
  HeroImage[536] = createTexturePNGFromMem(Aoi_h, sizeof(Aoi_h));
  HeroImage[537] = createTexturePNGFromMem(Allain_h, sizeof(Allain_h));
  HeroImage[538] = createTexturePNGFromMem(Iggy_h, sizeof(Iggy_h));
  HeroImage[539] = createTexturePNGFromMem(Laurion_h, sizeof(Laurion_h));
  HeroImage[540] = createTexturePNGFromMem(Bright_h, sizeof(Bright_h));
  HeroImage[541] = createTexturePNGFromMem(Bonie_h, sizeof(Bonie_h));
  HeroImage[542] = createTexturePNGFromMem(Tachi_h, sizeof(Tachi_h));
  HeroImage[543] = createTexturePNGFromMem(Aya_h, sizeof(Aya_h));
  HeroImage[544] = createTexturePNGFromMem(Yan_h, sizeof(Yan_h));
  HeroImage[545] = createTexturePNGFromMem(Yue_h, sizeof(Yue_h));
  HeroImage[546] = createTexturePNGFromMem(Terri_h, sizeof(Terri_h));
  HeroImage[548] = createTexturePNGFromMem(Bijan_h, sizeof(Bijan_h));
  HeroImage[568] = createTexturePNGFromMem(Ming_h, sizeof(Ming_h));
  HeroImage[159] = createTexturePNGFromMem(hero_159, sizeof(hero_159));
  HeroImage[206] = createTexturePNGFromMem(hero_596, sizeof(hero_596));
  HeroImage[504] = createTexturePNGFromMem(hero_504, sizeof(hero_504));
  HeroImage[563] = createTexturePNGFromMem(hero_563, sizeof(hero_563));
  HeroImage[567] = createTexturePNGFromMem(hero_567, sizeof(hero_567));
  HeroImage[577] = createTexturePNGFromMem(hero_577, sizeof(hero_577));
  HeroImage[582] = createTexturePNGFromMem(hero_584, sizeof(hero_584));
  HeroImage[584] = createTexturePNGFromMem(hero_582, sizeof(hero_582));
  HeroImage[595] = createTexturePNGFromMem(hero_595, sizeof(hero_595));
  HeroImage[596] = createTexturePNGFromMem(hero_206, sizeof(hero_206));
  HeroImage[597] = createTexturePNGFromMem(hero_597, sizeof(hero_597));
  HeroImage[598] = createTexturePNGFromMem(hero_598, sizeof(hero_598));
  HeroImage[599] = createTexturePNGFromMem(hero_599, sizeof(hero_599));
  LoadTalentImages();
  LoadSkillImages();
}

ImVec2 rectSize;
ImVec2 rectMax;
ImVec2 DrawMap;

void DrawESP(ImDrawList *draw) {
  // Cache front buffer index for this entire frame (prevent mid-frame swap
  // tearing) — acquire semantics pairs with release in ESPUpdateResponse
  const int _espFrontIdx = g_responseFront;
#undef Response
#define Response ResponseBuf[_espFrontIdx]

  // Stale data detection: if update hook stopped firing (match ended), clear
  // ESP
  if (Response.Count > 0) {
    float now = GetTimeSeconds();
    float lastUpdate = Response.players[0].lastUpdateTime;
    if (lastUpdate > 0 && now - lastUpdate > 2.0f) {
      ResponseBuf[0].Count = 0;
      ResponseBuf[1].Count = 0;
      campDetected = false;
      linkerMapCount = 0;
      g_visibleCacheCount = 0;

      myPlayerCamp = 0;
      Lactor = nullptr;
      return; // skip drawing this frame
    }
  }

  char tmp_watermark[256];
  std::sprintf(tmp_watermark,
               "ESP Build: " __DATE__ " " __TIME__
               " - %.1f FPS | Actors:%d Step:%d",
               ImGui::GetIO().Framerate, Response.Count, dbg_esp_step);
  draw->AddText(ImVec2(40.0f, ImGui::GetIO().DisplaySize.y - 30.0f),
                IM_COL32(255, 255, 255, 255), tmp_watermark);

  // Debug: show all actor IDs on screen (from Response buffer)
  if (dbg_showAllIDs) {
    for (int i = 0; i < Response.Count; i++) {
      Vector3 sc = Response.players[i].PositionSc;
      float sx = sc.x;
      float sy = glHeight - sc.y;
      if (sx > 0 && sx < glWidth && sy > 0 && sy < glHeight) {
        char idText[128];
        std::sprintf(
            idText, "ID:%d EC:%d H:%d V:%d HP:%d/%d",
            Response.players[i].ConfigID, Response.players[i].EnemyCamp,
            Response.players[i].isHeroUnit, Response.players[i].Visible,
            Response.players[i].ActorHP, Response.players[i].ActorMaxHP);
        ImU32 textCol = Response.players[i].ActorHP > 0
                            ? IM_COL32(0, 255, 255, 255)
                            : IM_COL32(255, 128, 0, 255);
        draw->AddText(ImVec2(sx, sy - 30), textCol, idText);
      }
    }
  }

  // Debug: draw first player info on screen
  if (Response.Count > 0) {
    char dbg[256];
    std::sprintf(
        dbg, "P0: HP=%d/%d Sc={%.0f,%.0f} Pos={%.1f,%.1f,%.1f} ID=%d Cnt=%d",
        Response.players[0].ActorHP, Response.players[0].ActorHPTotal,
        Response.players[0].PositionSc.x, Response.players[0].PositionSc.y,
        Response.players[0].Position.x, Response.players[0].Position.y,
        Response.players[0].Position.z, Response.players[0].ConfigID,
        Response.Count);
    draw->AddText(ImVec2(40.0f, ImGui::GetIO().DisplaySize.y - 60.0f),
                  IM_COL32(255, 255, 0, 255), dbg);
  }

  // === ESP ULTIMATE HUD (top center) ===
  if (ESP.Ultimate && Response.Count > 0) {
    float iconRadius = 22.0f * g_ultScale;
    float spacing = 58.0f * g_ultScale;
    float startY = g_ultPosY;

    // Count heroes (DEBUG: include allies for testing)
    int enemyHeroCount = 0;
    for (int i = 0; i < Response.Count; i++) {
      if (Response.players[i].isHeroUnit) {
        enemyHeroCount++;
      }
    }

    if (enemyHeroCount > 0) {
      float totalWidth = enemyHeroCount * spacing;
      float startX = (glWidth - totalWidth) / 2.0f + spacing / 2.0f + g_ultPosX;
      int heroIdx = 0;

      for (int i = 0; i < Response.Count; i++) {
        // DEBUG: temporarily show all heroes (including allies)
        if (!Response.players[i].isHeroUnit)
          continue;

        bool isDead = (Response.players[i].ActorHP <= 0);
        float cx = startX + heroIdx * spacing;
        int cid = Response.players[i].ConfigID;
        int ultCD = Response.players[i].Skill3CD;
        bool ultUnlock = Response.players[i].Skill3Unlock;
        int talentCD = Response.players[i].TalentCD;
        int curHP = Response.players[i].ActorHP;
        int maxHP = Response.players[i].ActorMaxHP;

        // Hero portrait (circular image)
        float imgY = startY + iconRadius;
        float ultR = 10.0f * g_ultScale;
        float ultY = imgY - iconRadius; // center at top edge of portrait

        // Draw portrait first (so diamond renders on top)
        draw->AddCircleFilled(ImVec2(cx, imgY), iconRadius + 2,
                              IM_COL32(30, 30, 30, 255));
        TextureInfo texInfo{};
        if (cid > 0 && cid < 700)
          texInfo = HeroImage[cid];
        if (texInfo.textureId) {
          draw->AddImageRounded(texInfo.textureId,
                                ImVec2(cx - iconRadius, imgY - iconRadius),
                                ImVec2(cx + iconRadius, imgY + iconRadius),
                                ImVec2(0, 0), ImVec2(1, 1),
                                isDead ? IM_COL32(100, 100, 100, 200)
                                       : IM_COL32(255, 255, 255, 255),
                                iconRadius);
        } else {
          draw->AddCircleFilled(ImVec2(cx, imgY), iconRadius,
                                IM_COL32(60, 60, 60, 255));
        }
        // Dead overlay (X mark)
        if (isDead) {
          draw->AddCircleFilled(ImVec2(cx, imgY), iconRadius,
                                IM_COL32(0, 0, 0, 150));
          float xSize = iconRadius * 0.5f;
          draw->AddLine(ImVec2(cx - xSize, imgY - xSize),
                        ImVec2(cx + xSize, imgY + xSize),
                        IM_COL32(255, 50, 50, 255), 2.0f);
          draw->AddLine(ImVec2(cx + xSize, imgY - xSize),
                        ImVec2(cx - xSize, imgY + xSize),
                        IM_COL32(255, 50, 50, 255), 2.0f);
        }

        // Ult indicator: diamond half inside portrait top (drawn AFTER
        // portrait)
        if (!isDead) {
          if (ultUnlock && ultCD == 0) {
            // Ready: green diamond
            ImVec2 diamond[4] = {
                ImVec2(cx, ultY - ultR), ImVec2(cx + ultR, ultY),
                ImVec2(cx, ultY + ultR), ImVec2(cx - ultR, ultY)};
            draw->AddConvexPolyFilled(diamond, 4, IM_COL32(0, 220, 0, 255));
          } else if (ultUnlock && ultCD > 0) {
            // On CD: show seconds
            draw->AddCircleFilled(ImVec2(cx, ultY), ultR,
                                  IM_COL32(80, 80, 80, 220));
            char cdTxt[8];
            std::sprintf(cdTxt, "%d", (ultCD + 999) / 1000);
            ImVec2 ts = ImGui::CalcTextSize(cdTxt);
            draw->AddText(ImVec2(cx - ts.x / 2, ultY - ts.y / 2),
                          IM_COL32(255, 255, 255, 255), cdTxt);
          } else {
            // Locked: dark circle
            draw->AddCircleFilled(ImVec2(cx, ultY), ultR,
                                  IM_COL32(40, 40, 40, 200));
          }
        }

        // HP bar below portrait
        float hpBarY = imgY + iconRadius + 2 * g_ultScale;
        float hpBarW = iconRadius * 2.0f;
        float hpBarH = 4.0f * g_ultScale;
        float hpRatio = (maxHP > 0) ? (float)curHP / (float)maxHP : 0.0f;
        if (hpRatio < 0)
          hpRatio = 0;
        if (hpRatio > 1)
          hpRatio = 1;
        // BG
        draw->AddRectFilled(ImVec2(cx - hpBarW / 2, hpBarY),
                            ImVec2(cx + hpBarW / 2, hpBarY + hpBarH),
                            IM_COL32(40, 40, 40, 200));
        // HP fill
        ImU32 hpColor =
            isDead ? IM_COL32(80, 80, 80, 200)
                   : (hpRatio > 0.5f
                          ? IM_COL32(0, 200, 0, 255)
                          : (hpRatio > 0.25f ? IM_COL32(255, 165, 0, 255)
                                             : IM_COL32(255, 50, 50, 255)));
        draw->AddRectFilled(
            ImVec2(cx - hpBarW / 2, hpBarY),
            ImVec2(cx - hpBarW / 2 + hpBarW * hpRatio, hpBarY + hpBarH),
            hpColor);

        // Talent icon (below HP bar)
        float talentY = hpBarY + hpBarH + 2 * g_ultScale;
        float talentR = 15.0f * g_ultScale;
        unsigned int talentSID = Response.players[i].TalentSkillId;
        TextureInfo talentTex = GetTalentTexture(talentSID);

        // Draw talent icon circle
        if (talentTex.textureId) {
          draw->AddImageRounded(
              talentTex.textureId, ImVec2(cx - talentR, talentY),
              ImVec2(cx + talentR, talentY + talentR * 2), ImVec2(0, 0),
              ImVec2(1, 1), IM_COL32(255, 255, 255, 255), talentR);
        } else {
          draw->AddCircleFilled(ImVec2(cx, talentY + talentR), talentR,
                                IM_COL32(60, 60, 60, 255));
        }
        // CD overlay on talent
        if (talentCD > 0) {
          draw->AddCircleFilled(ImVec2(cx, talentY + talentR), talentR,
                                IM_COL32(0, 0, 0, 150));
          char tcdTxt[8];
          std::sprintf(tcdTxt, "%d", (talentCD + 999) / 1000);
          ImVec2 tts = ImGui::CalcTextSize(tcdTxt);
          draw->AddText(ImVec2(cx - tts.x / 2, talentY + talentR - tts.y / 2),
                        IM_COL32(255, 255, 255, 255), tcdTxt);
        }
        // Green border when ready
        if (talentCD == 0) {
          draw->AddCircle(ImVec2(cx, talentY + talentR), talentR + 1,
                          IM_COL32(0, 220, 0, 255), 0, 2.0f);
        }

        // Item Active icon (below talent)
        int itemActiveCD = Response.players[i].ItemActiveCD;
        unsigned int itemActiveSID = Response.players[i].ItemActiveSkillId;
        if (itemActiveSID > 0) {
          float itemY = talentY + talentR * 2 + 4 * g_ultScale;
          float itemR = 15.0f * g_ultScale;

          // Draw item active icon circle (gray bg, no texture for now)
          draw->AddCircleFilled(ImVec2(cx, itemY + itemR), itemR,
                                IM_COL32(80, 60, 20, 255));
          // Border
          draw->AddCircle(ImVec2(cx, itemY + itemR), itemR + 1,
                          itemActiveCD == 0 ? IM_COL32(0, 220, 0, 255)
                                            : IM_COL32(100, 100, 100, 255),
                          0, 2.0f);
          // CD overlay
          if (itemActiveCD > 0) {
            draw->AddCircleFilled(ImVec2(cx, itemY + itemR), itemR,
                                  IM_COL32(0, 0, 0, 150));
            char icdTxt[8];
            std::sprintf(icdTxt, "%d", (itemActiveCD + 999) / 1000);
            ImVec2 its = ImGui::CalcTextSize(icdTxt);
            draw->AddText(
                ImVec2(cx - its.x / 2, itemY + itemR - its.y / 2),
                IM_COL32(255, 255, 255, 255), icdTxt);
          }
        }

        heroIdx++;
      }
    }
  }

  if (ESP.Enable) {
    try {
      for (int i = 0; i < Response.Count; i++) {
        if (Response.players[i].ActorHP > 0) {
          Vector3 EnemyPosition = Response.players[i].Position;
          if (EnemyPosition.x == 0 && EnemyPosition.y == 0 &&
              EnemyPosition.z == 0)
            continue;

          Vector3 EnemyPositionSc = Response.players[i].PositionSc;

          bool canDrawOnScreen =
              (EnemyPositionSc.z > 0) &&
              (EnemyPositionSc.x > -500 && EnemyPositionSc.x < glWidth + 500) &&
              (EnemyPositionSc.y > -500 && EnemyPositionSc.y < glHeight + 500);

          Vector2 EnemyRootSc =
              Vector2(EnemyPositionSc.x, glHeight - EnemyPositionSc.y);

          // Minions/monsters: dot + HP number only
          if (!Response.players[i].isHeroUnit) {
            // Smooth position using lerp cache
            static struct {
              void *ptr;
              float x, y;
            } minionPosCache[200];
            static int minionCacheCount = 0;
            void *actPtr = Response.players[i].actorPtr;
            int cacheIdx = -1;
            for (int c = 0; c < minionCacheCount; c++) {
              if (minionPosCache[c].ptr == actPtr) {
                cacheIdx = c;
                break;
              }
            }
            if (cacheIdx < 0 && minionCacheCount < 200) {
              cacheIdx = minionCacheCount++;
              minionPosCache[cacheIdx].ptr = actPtr;
              minionPosCache[cacheIdx].x = EnemyRootSc.x;
              minionPosCache[cacheIdx].y = EnemyRootSc.y;
            }
            if (cacheIdx >= 0) {
              float lerpT =
                  0.3f; // smooth factor (lower = smoother but laggier)
              minionPosCache[cacheIdx].x +=
                  (EnemyRootSc.x - minionPosCache[cacheIdx].x) * lerpT;
              minionPosCache[cacheIdx].y +=
                  (EnemyRootSc.y - minionPosCache[cacheIdx].y) * lerpT;
              EnemyRootSc.x = minionPosCache[cacheIdx].x;
              EnemyRootSc.y = minionPosCache[cacheIdx].y;
            }
            if (ESP.Minions && Response.players[i].isEnemy) {
              // Visible check: hide when in line of sight
              if (ESP.VisibleCheck && Response.players[i].Visible)
                continue;
              // Color based on configID
              ImU32 dotColor = IM_COL32(
                  255, 255, 0, 255); // default yellow for minions/monsters
              ImU32 mapColor = IM_COL32(255, 255, 0, 200);
              int cid = Response.players[i].ConfigID;
              if (cid == 7010) {
                dotColor = IM_COL32(0, 100, 255, 255); // blue buff
                mapColor = IM_COL32(0, 100, 255, 200);
              } else if (cid == 7011) {
                dotColor = IM_COL32(255, 0, 0, 255); // red buff
                mapColor = IM_COL32(255, 0, 0, 200);
              } else if (cid == 7109 || (cid >= 71093 && cid <= 71097)) {
                dotColor = IM_COL32(180, 0, 255, 255); // purple - super minion
                mapColor = IM_COL32(180, 0, 255, 200);
              }
              // On-screen dot + HP (only if visible on screen)
              if (canDrawOnScreen) {
                draw->AddCircleFilled(ImVec2(EnemyRootSc.x, EnemyRootSc.y), 6,
                                      dotColor);
                char hpText[32];
                std::sprintf(hpText, "%d", Response.players[i].ActorHP);
                ImVec2 hpSize = ImGui::CalcTextSize(hpText);
                draw->AddText(
                    ImVec2(EnemyRootSc.x - hpSize.x / 2, EnemyRootSc.y - 20),
                    IM_COL32(255, 255, 255, 255), hpText);
              }

              // Minimap dot for minions (always render)
              if (ESP.Map && MinimapPosSc.y > 0) {
                float scaleX = minimapScale, scaleY = minimapScale;
                if (myPlayerCamp == 2) {
                  scaleX = -minimapScale;
                  scaleY = -minimapScale;
                }
                float posX = minimapPosX;
                float posY =
                    MinimapPosSc.y < 860 ? minimapPosY : minimapPosY + 77.0f;
                ImVec2 rectPosition =
                    MinimapPosSc.y < 860
                        ? ImVec2(MinimapScSize.x - MinimapPosSc.x + 80, 94)
                        : ImVec2(MinimapScSize.x - MinimapPosSc.x + 80, 0);
                ImVec2 rSize = ImVec2(MinimapScale.x * MinimapScSize.x,
                                      MinimapScale.y * MinimapScSize.y);
                ImVec2 minionMap =
                    ImVec2((rSize.x + EnemyPosition.x * scaleX) - posX,
                           (rSize.y - EnemyPosition.z * scaleY) - posY);
                draw->AddCircleFilled(minionMap, 5, mapColor);
              }
            }
            continue;
          }

          // Heroes below
          Vector3 MyPosition = Response.players[i].My_Position;

          // Depth-based scale: ESP stays readable at all distances
          float depthScale = 1.0f;
          if (canDrawOnScreen && EnemyPositionSc.z > 0) {
            depthScale = g_espDepthRef / EnemyPositionSc.z;
            if (depthScale > 3.0f)
              depthScale = 3.0f;
            if (depthScale < 0.7f)
              depthScale = 0.7f;
          }

          Vector2 EnemyHeadSc = Vector2(
              glWidth - (glWidth - EnemyPositionSc.x) + 5,
              glHeight - EnemyPositionSc.y - (glHeight / 6.35f) * depthScale);

          // ESP BOX //
          float boxHeight = abs(EnemyHeadSc.y - EnemyRootSc.y);
          float boxWidth = boxHeight * 0.75f;
          ImVec2 vBoxStart = {EnemyHeadSc.x - (boxWidth / 2.f), EnemyHeadSc.y};
          ImVec2 vBoxEnd = {vBoxStart.x + boxWidth, vBoxStart.y + boxHeight};
          // ESP INFO //
          float InfoHeight = 30.f * depthScale;
          float InfoWidth = boxHeight * 0.75f;
          ImVec2 vInfoStart = {EnemyRootSc.x - (InfoWidth / 2.f),
                               EnemyRootSc.y + 15.0f * depthScale};
          ImVec2 vInfoEnd = {vInfoStart.x + InfoWidth,
                             vInfoStart.y + InfoHeight};
          // HERO INFO //
          int EnemyCamp = Response.players[i].EnemyCamp;
          bool UnlockC1 = Response.players[i].Skill1Unlock;
          bool UnlockC2 = Response.players[i].Skill2Unlock;
          bool UnlockC3 = Response.players[i].Skill3Unlock;
          int C1_LV = Response.players[i].Skill1Level;
          int C2_LV = Response.players[i].Skill2Level;
          int C3_LV = Response.players[i].Skill3Level;
          int C1_CD = Response.players[i].Skill1CD;
          int C2_CD = Response.players[i].Skill2CD;
          int C3_CD = Response.players[i].Skill3CD;
          int HP_CD = Response.players[i].HPCD;
          int Talent_CD = Response.players[i].TalentCD;

          if (Response.players[i].isEnemy) {

            bool skipPosESP = false;
            if (ESP.VisibleCheck && Response.players[i].Visible) {
              skipPosESP = true;
            }

            if (canDrawOnScreen && !skipPosESP) {
              if (ESP.Line) {
                draw->AddLine(ImVec2(glWidth / 2, 80),
                              ImVec2(EnemyHeadSc.x, EnemyHeadSc.y),
                              IM_COL32(255, 255, 255, 255), 1.7f);
                draw->AddCircleFilled(ImVec2(EnemyHeadSc.x, EnemyHeadSc.y), 8,
                                      IM_COL32(255, 255, 255, 255));
                draw->AddCircleFilled(ImVec2(glWidth / 2, 80), 8,
                                      IM_COL32(255, 255, 255, 255));
              }

              if (ESP.Box) {
                draw->AddRect(vBoxStart, vBoxEnd, IM_COL32(255, 255, 255, 255),
                              0, 240, 1.7f);
              }

              if (ESP.PlayerInfo) {
                draw->AddRectFilled(vInfoStart, vInfoEnd,
                                    IM_COL32(255, 255, 255, 255), 10.f,
                                    ImDrawFlags_RoundCornersAll);
                drawRhombus(draw,
                            ImVec2(EnemyRootSc.x - (InfoWidth / 2.f),
                                   EnemyRootSc.y + 30.f),
                            20.f);

                std::string stdHeroName =
                    GetNameActors(Response.players[i].ConfigID);
                std::string stdHeroLevel =
                    to_string(Response.players[i].ActorLevel);
                ImVec2 HeroName = ImGui::CalcTextSize(stdHeroName.c_str());

                draw->AddText(NULL, ((float)glHeight / 48.0f),
                              {(EnemyRootSc.x - HeroName.x / 2) + 8.f,
                               EnemyRootSc.y + 20.f},
                              ImColor(0, 0, 0, 255), stdHeroName.c_str());

                if (Response.players[i].ActorLevel < 10)
                  draw->AddText(NULL, ((float)glHeight / 48.0f),
                                {(EnemyRootSc.x - (InfoWidth / 2.f)) - 4,
                                 EnemyRootSc.y + 20},
                                ImColor(0, 0, 0, 255), stdHeroLevel.c_str());
                if (Response.players[i].ActorLevel >= 10)
                  draw->AddText(NULL, ((float)glHeight / 48.0f),
                                {(EnemyRootSc.x - (InfoWidth / 2.f)) - 10,
                                 EnemyRootSc.y + 20},
                                ImColor(0, 0, 0, 255), stdHeroLevel.c_str());
              }
            } // end canDrawOnScreen && !skipPosESP

            if (canDrawOnScreen && ESP.Cooldown) {
              int cid_esp = Response.players[i].ConfigID;
              float cdRadius = 22.f * depthScale;
              float cdSpacing = 50.f * depthScale;
              float cdOffsetX =
                  (boxWidth / 2 + 27.5f * depthScale) + 7.5f * depthScale;

              // Helper lambda-like: draw skill icon at position with CD
              // overlay Skill 1
              float s1X = EnemyRootSc.x + cdOffsetX;
              float s1Y = EnemyHeadSc.y + 0;
              TextureInfo s1Tex = GetSkillTexture(cid_esp, 1);
              if (s1Tex.textureId) {
                draw->AddImageRounded(
                    s1Tex.textureId, ImVec2(s1X - cdRadius, s1Y - cdRadius),
                    ImVec2(s1X + cdRadius, s1Y + cdRadius), ImVec2(0, 0),
                    ImVec2(1, 1),
                    (C1_CD == 0 && UnlockC1) ? IM_COL32(255, 255, 255, 255)
                                             : IM_COL32(100, 100, 100, 255),
                    cdRadius);
              } else {
                ImColor Skill1Color =
                    (C1_CD == 0 && UnlockC1)
                        ? ImColor(0, 255, 0, 255)
                        : (UnlockC1 ? ImColor(255, 255, 255, 255)
                                    : ImColor(0, 0, 0, 255));
                draw->AddCircleFilled(ImVec2(s1X, s1Y), cdRadius, Skill1Color);
              }
              if (C1_CD > 0) {
                draw->AddCircleFilled(ImVec2(s1X, s1Y), cdRadius,
                                      IM_COL32(0, 0, 0, 150));
                drawTextInt(ImVec2(s1X, s1Y), (C1_CD + 999) / 1000, draw);
              }

              // Skill 2
              float s2X = EnemyRootSc.x + cdOffsetX;
              float s2Y = EnemyHeadSc.y + cdSpacing;
              TextureInfo s2Tex = GetSkillTexture(cid_esp, 2);
              if (s2Tex.textureId) {
                draw->AddImageRounded(
                    s2Tex.textureId, ImVec2(s2X - cdRadius, s2Y - cdRadius),
                    ImVec2(s2X + cdRadius, s2Y + cdRadius), ImVec2(0, 0),
                    ImVec2(1, 1),
                    (C2_CD == 0 && UnlockC2) ? IM_COL32(255, 255, 255, 255)
                                             : IM_COL32(100, 100, 100, 255),
                    cdRadius);
              } else {
                ImColor Skill2Color =
                    (C2_CD == 0 && UnlockC2)
                        ? ImColor(0, 255, 0, 255)
                        : (UnlockC2 ? ImColor(255, 255, 255, 255)
                                    : ImColor(0, 0, 0, 255));
                draw->AddCircleFilled(ImVec2(s2X, s2Y), cdRadius, Skill2Color);
              }
              if (C2_CD > 0) {
                draw->AddCircleFilled(ImVec2(s2X, s2Y), cdRadius,
                                      IM_COL32(0, 0, 0, 150));
                drawTextInt(ImVec2(s2X, s2Y), (C2_CD + 999) / 1000, draw);
              }

              // Skill 3
              float s3X = EnemyRootSc.x + cdOffsetX;
              float s3Y = EnemyHeadSc.y + cdSpacing * 2;
              TextureInfo s3Tex = GetSkillTexture(cid_esp, 3);
              if (s3Tex.textureId) {
                draw->AddImageRounded(
                    s3Tex.textureId, ImVec2(s3X - cdRadius, s3Y - cdRadius),
                    ImVec2(s3X + cdRadius, s3Y + cdRadius), ImVec2(0, 0),
                    ImVec2(1, 1),
                    (C3_CD == 0 && UnlockC3) ? IM_COL32(255, 255, 255, 255)
                                             : IM_COL32(100, 100, 100, 255),
                    cdRadius);
              } else {
                ImColor Skill3Color =
                    (C3_CD == 0 && UnlockC3)
                        ? ImColor(0, 255, 0, 255)
                        : (UnlockC3 ? ImColor(255, 255, 255, 255)
                                    : ImColor(0, 0, 0, 255));
                draw->AddCircleFilled(ImVec2(s3X, s3Y), cdRadius, Skill3Color);
              }
              if (C3_CD > 0) {
                draw->AddCircleFilled(ImVec2(s3X, s3Y), cdRadius,
                                      IM_COL32(0, 0, 0, 150));
                drawTextInt(ImVec2(s3X, s3Y), (C3_CD + 999) / 1000, draw);
              }

              // Separator line
              draw->AddLine(ImVec2(EnemyRootSc.x + cdOffsetX - cdRadius,
                                   EnemyHeadSc.y + cdSpacing * 3),
                            ImVec2(EnemyRootSc.x + cdOffsetX + cdRadius,
                                   EnemyHeadSc.y + cdSpacing * 3),
                            IM_COL32(255, 255, 255, 255), 1.7f);

              // Talent (below separator)
              float tX = EnemyRootSc.x + cdOffsetX;
              float tY = EnemyHeadSc.y + cdSpacing * 4;
              unsigned int tSID = Response.players[i].TalentSkillId;
              TextureInfo tTex = GetTalentTexture(tSID);
              if (tTex.textureId) {
                draw->AddImageRounded(
                    tTex.textureId, ImVec2(tX - cdRadius, tY - cdRadius),
                    ImVec2(tX + cdRadius, tY + cdRadius), ImVec2(0, 0),
                    ImVec2(1, 1),
                    (Talent_CD == 0) ? IM_COL32(255, 255, 255, 255)
                                     : IM_COL32(100, 100, 100, 255),
                    cdRadius);
              } else {
                ImColor TalentColor = (Talent_CD == 0)
                                          ? ImColor(0, 255, 0, 255)
                                          : ImColor(255, 255, 255, 255);
                draw->AddCircleFilled(ImVec2(tX, tY), cdRadius, TalentColor);
              }
              if (Talent_CD > 0) {
                draw->AddCircleFilled(ImVec2(tX, tY), cdRadius,
                                      IM_COL32(0, 0, 0, 150));
                drawTextInt(ImVec2(tX, tY), (Talent_CD + 999) / 1000, draw);
              }
            } // end canDrawOnScreen && ESP.Cooldown

            if (canDrawOnScreen && !skipPosESP) {
              if (ESP.Alert && isOutsideScreen(ImVec2(EnemyRootSc.x,
                                                      glHeight - EnemyRootSc.y),
                                               ImVec2(glWidth, glHeight))) {
                ImVec2 hintDotRenderPos =
                    pushToScreenBorder(ImVec2(EnemyRootSc.x, EnemyRootSc.y),
                                       ImVec2(glWidth, glHeight), -50);
                ImVec2 hintTextRenderPos =
                    pushToScreenBorder(ImVec2(EnemyRootSc.x, EnemyRootSc.y),
                                       ImVec2(glWidth, glHeight), -50);

                if ((int)Response.players[i].Distance < 30) {
                  draw->AddCircleFilled(hintDotRenderPos, 45,
                                        IM_COL32(255, 0, 0, 110));
                } else if ((int)Response.players[i].Distance <= 50) {
                  draw->AddCircleFilled(hintDotRenderPos, 45,
                                        IM_COL32(255, 255, 0, 110));
                } else {
                  draw->AddCircleFilled(hintDotRenderPos, 45,
                                        IM_COL32(0, 255, 0, 110));
                }

                DrawCircleHealthAlert(
                    hintDotRenderPos, Response.players[i].ActorHP,
                    Response.players[i].ActorHPTotal, 45, draw);
                std::string strDistance =
                    to_string((int)Response.players[i].Distance) + "M";
                auto AlertSize1 = ImGui::CalcTextSize(
                    strDistance.c_str(), 0, ((float)glHeight / 45.0f));
                draw->AddText(NULL, ((float)glHeight / 45.0f),
                              {hintTextRenderPos.x - (AlertSize1.x / 2),
                               hintTextRenderPos.y + 14},
                              IM_COL32(255, 255, 255, 255),
                              strDistance.c_str());
                auto AlertSize2 = ImGui::CalcTextSize(
                    GetNameActors(Response.players[i].ConfigID), 0,
                    ((float)glHeight / 39.0f));
                draw->AddText(NULL, ((float)glHeight / 39.0f),
                              {hintTextRenderPos.x - (AlertSize2.x / 2),
                               hintTextRenderPos.y - 14},
                              IM_COL32(255, 255, 255, 255),
                              GetNameActors(Response.players[i].ConfigID));
              }

              if (ESP.HP) {
                float boxHeight = abs(EnemyHeadSc.y - EnemyRootSc.y);
                float boxWidth = boxHeight * 0.75f;

                int EnemyHp = Response.players[i].ActorHP;
                int EnemyHpTotal = Response.players[i].ActorHPTotal;
                float PercentHP =
                    ((float)EnemyHp * boxHeight) / ((float)EnemyHpTotal);

                ImU32 healthColor = IM_COL32(45, 180, 45, 255);
                if (EnemyHp <= (EnemyHpTotal * 0.6)) {
                  healthColor = IM_COL32(180, 180, 45, 255);
                }
                if (EnemyHp < (EnemyHpTotal * 0.3)) {
                  healthColor = IM_COL32(180, 45, 45, 255);
                }

                draw->AddRectFilled(
                    ImVec2(EnemyRootSc.x - (boxWidth / 2) - 15, EnemyRootSc.y),
                    ImVec2(EnemyRootSc.x - (boxWidth / 2) - 5,
                           EnemyRootSc.y - PercentHP),
                    healthColor);
                draw->AddRect(
                    ImVec2(EnemyRootSc.x - (boxWidth / 2) - 15, EnemyRootSc.y),
                    ImVec2(EnemyHeadSc.x - (boxWidth / 2) - 5, EnemyHeadSc.y),
                    IM_COL32(0, 0, 0, 255), 0, 240, 0.5);
              }
            } // end canDrawOnScreen && !skipPosESP (Alert + HP)

            if (ESP.Map && !skipPosESP) {
              ImVec2 rectPosition;
              float posX, posY, scaleX, scaleY;
              if (MinimapPosSc.y < 860) {
                if (myPlayerCamp == 2) {
                  scaleX = -minimapScale;
                  scaleY = -minimapScale;
                } else {
                  scaleX = minimapScale;
                  scaleY = minimapScale;
                }
                posX = minimapPosX;
                posY = minimapPosY;
                rectPosition =
                    ImVec2(MinimapScSize.x - MinimapPosSc.x + 80, 94);
              }
              if (MinimapPosSc.y > 860) {
                if (myPlayerCamp == 2) {
                  scaleX = -minimapScale;
                  scaleY = -minimapScale;
                } else {
                  scaleX = minimapScale;
                  scaleY = minimapScale;
                }
                posX = minimapPosX;
                posY = minimapPosY + 77.0f;
                rectPosition = ImVec2(MinimapScSize.x - MinimapPosSc.x + 80, 0);
              }

              // ImVec2 rectSize = ImVec2(MinimapScale.x * MinimapScSize.x,
              // MinimapScale.y * MinimapScSize.y); ImVec2 rectMax =
              // ImVec2(rectPosition.x + rectSize.x, rectPosition.y +
              // rectSize.y); ImVec2 DrawMap = ImVec2((rectSize.x +
              // EnemyPosition.x * scaleX) - posX, (rectSize.y - EnemyPosition.z
              // * scaleY) - posY);

              rectSize = ImVec2(MinimapScale.x * MinimapScSize.x,
                                MinimapScale.y * MinimapScSize.y);
              rectMax = ImVec2(rectPosition.x + rectSize.x,
                               rectPosition.y + rectSize.y);
              DrawMap = ImVec2((rectSize.x + EnemyPosition.x * scaleX) - posX,
                               (rectSize.y - EnemyPosition.z * scaleY) - posY);

              draw->AddRect(rectPosition,
                            ImVec2(rectPosition.x + rectSize.x,
                                   rectPosition.y + rectSize.y),
                            IM_COL32(235, 222, 206, 255));

              draw->AddCircle(DrawMap, 20.0f, IM_COL32(100, 0, 0, 255), 100,
                              4.0f);
              // draw->AddCircleFilled(DrawMap, 16.0f, IM_COL32(255, 255, 255,
              // 255));
              TextureInfo textureInfo{};
              int cid = Response.players[i].ConfigID;
              if (cid > 0 && cid < 700)
                textureInfo = HeroImage[cid];

              if (textureInfo.textureId)
                draw->AddImageRounded(textureInfo.textureId,
                                      ImVec2(DrawMap.x - 20, DrawMap.y - 20),
                                      ImVec2(DrawMap.x + 20, DrawMap.y + 20),
                                      ImVec2(0, 0), ImVec2(1, 1),
                                      ImColor(255, 255, 255, 255), 100);

              DrawCircleHealth(Response.players[i].ActorHP,
                               Response.players[i].ActorHPTotal, 20.0f, DrawMap,
                               draw);
            }

            if (ESP.HeroImage && !skipPosESP) {
              float squareSize =
                  std::min(vBoxEnd.x - vBoxStart.x, vBoxEnd.y - vBoxStart.y);

              float scale = 0.8f;
              ImVec2 imageSize = ImVec2(squareSize * scale, squareSize * scale);

              vBoxStart =
                  ImVec2((vBoxStart.x + vBoxEnd.x - imageSize.x) * 0.5f,
                         (vBoxStart.y + vBoxEnd.y - imageSize.y) * 0.5f);
              vBoxEnd =
                  ImVec2(vBoxStart.x + imageSize.x, vBoxStart.y + imageSize.y);

              TextureInfo textureInfo{};
              int cid2 = Response.players[i].ConfigID;
              if (cid2 > 0 && cid2 < 700)
                textureInfo = HeroImage[cid2];

              if (textureInfo.textureId)
                draw->AddImageRounded(
                    textureInfo.textureId, ImVec2(vBoxStart.x, vBoxStart.y),
                    ImVec2(vBoxEnd.x, vBoxEnd.y), ImVec2(0, 0), ImVec2(1, 1),
                    IM_COL32(255, 255, 255, 255), 100);
            }
          }
        }
      }
    } catch (...) {
    }
  }
}
// Restore Response macro to global version
#undef Response
#define Response ResponseBuf[g_responseFront]
/*
class Camera {
public:
    static Camera* get_main() {
        Camera* (*get_main_) () = (Camera* (*)())
(GetMethodOffset(oxorany("UnityEngine.dll"), oxorany("UnityEngine"),
oxorany("Camera"), oxorany("get_main"), 0)); return get_main_();
    }

    float get_fieldOfView() {
        float (*get_fieldOfView_)(Camera* camera) = (float (*)(Camera*))
(GetMethodOffset(oxorany("UnityEngine.dll"), oxorany("UnityEngine"),
oxorany("Camera"), oxorany("get_fieldOfView"), 0)); return
get_fieldOfView_(this);
    }

    void set_fieldOfView(float value) {
        void (*set_fieldOfView_)(Camera* camera, float value) = (void
(*)(Camera*, float)) (GetMethodOffset(oxorany("UnityEngine.dll"),
oxorany("UnityEngine"), oxorany("Camera"), oxorany("set_fieldOfView"), 1));
        set_fieldOfView_(this, value);
    }
};

float getFieldOfView = 0;
float setFieldOfView = 1.0f + (Camera.V2.Value - 1) * 0.1f;*/

bool (*_ShowHeroInfo)(void *instance);
bool ShowHeroInfo(void *instance) {
  if (instance != NULL && MemoryHack.Unti) {
    return true;
  }
  return _ShowHeroInfo(instance);
}

void (*_SetVisible)(...);
void SetVisible(void *instance, int camp, bool bVisible,
                const bool forceSync = false) {
  // Cache visibility state for our camp (used by ESP minion/monster filter)
  if (instance != NULL && campDetected && LGameActorMgr &&
      camp == myPlayerCamp) {
    int slot = -1;
    for (int i = 0; i < g_visibleCacheCount; i++) {
      if (g_visibleCache[i].linkerPtr == instance) {
        slot = i;
        break;
      }
    }
    if (slot < 0 && g_visibleCacheCount < 500)
      slot = g_visibleCacheCount++;
    if (slot >= 0) {
      g_visibleCache[slot].linkerPtr = instance;
      g_visibleCache[slot].visible = bVisible;
    }
  }
  if (instance != NULL && MemoryHack.Map && LGameActorMgr) {
    if (camp == 1 || camp == 2) {
      bVisible = true;
    }
  }
  return _SetVisible(instance, camp, bVisible, forceSync);
}

// Hien Unti
void (*_ShowSkillStateInfo)(void *instance, bool bShow);
void ShowSkillStateInfo(void *instance, bool bShow) {
  if (instance != NULL && MemoryHack.Unti) {
    bShow = true;
  }
  _ShowSkillStateInfo(instance, bShow);
}

void (*_ShowHeroHpInfo)(void *instance, bool bShow);
void ShowHeroHpInfo(void *instance, bool bShow) {
  if (instance != NULL && MemoryHack.Unti) {
    bShow = true;
  }
  _ShowHeroHpInfo(instance, bShow);
}

bool (*_get_IsHostProfile)(void *instance);
bool get_IsHostProfile(void *instance) {
  if (instance != NULL && MemoryHack.History) {
    return true;
  }
  return _get_IsHostProfile(instance);
}

bool (*_IsSkillDirControlRotate)(void *instance, int skillSlotType);
bool IsSkillDirControlRotate(void *instance, int skillSlotType) {
  if (instance != NULL) {
    if (MemoryHack.HideLineElsu)
      return false;
  }
  return _IsSkillDirControlRotate(instance, skillSlotType);
}

int SkillDown, skillSlot;

bool onHold() {
  if (SkillDown == 1 && skillSlot == 2)
    return true;
  else
    return false;
}

bool (*_IsUseSkillJoystick)(void *a1, int a2);
bool IsUseSkillJoystick(void *a1, int a2) {
  skillSlot = a2;
  if (a1 != NULL && MemoryHack.AutoTrungElsu && onHold() && skillSlot == 2)
    return true;
  return _IsUseSkillJoystick(a1, a2);
}

void (*_onSkillButtonDown)(...);
void onSkillButtonDown(void *a1, int a2) {
  if (a1 != NULL && skillSlot == 2)
    SkillDown = 1;
  _onSkillButtonDown(a1, a2);
}

void (*_OnSkillButtonUp)(...);
void OnSkillButtonUp(void *a1, int a2) {
  if (a1 != NULL && skillSlot == 2)
    SkillDown = 0;
  _OnSkillButtonUp(a1, a2);
}

bool (*_IsSmartUse)(...);
bool IsSmartUse(void *a1) {
  if (a1 != NULL && MemoryHack.AutoTrungElsu && skillSlot == 2)
    return true;
  return _IsSmartUse(a1);
}

bool (*_get_IsUseCameraMoveWithIndicator)(...);
bool get_IsUseCameraMoveWithIndicator(void *a1) {
  if (a1 != NULL && MemoryHack.AutoTrungElsu && onHold() && skillSlot == 2)
    return false;
  return _get_IsUseCameraMoveWithIndicator(a1);
}

void (*_IsDistanceLowerEqualAsAttacker)(...);
void IsDistanceLowerEqualAsAttacker(void *a1, int a2, int a3) {
  if (a1 != NULL && MemoryHack.AutoTrungElsu && skillSlot == 2)
    a3 = 10000 * 6000;
  _IsDistanceLowerEqualAsAttacker(a1, a2, a3);
}

void (*_RET)(void *instance);
void RET(void *instance) {
  if (instance != NULL)
    return;
}
