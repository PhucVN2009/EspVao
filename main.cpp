#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_android.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#include "KittyMemory/MemoryPatch.h"
#include "Iconcpp.h"
#include "ImguiPP.h"
#include "Font.h"
#include "Icon.h"
#include "Helper/fake_dlfcn.h"
#include "Helper/Includes.h"
#include "Helper/plthook.h"
#include "Helper/json.hpp"
#include "Helper/Items.h"
#include "StrEnc.h"
#include "Spoof.h"
#include "Tools.h"
#include "SDK.hpp"
#include "obfuscate.h"
#include "Dobby/dobby.h"

bool WriteAddr(void *addr, void *buffer, size_t length) {
    unsigned long page_size = sysconf(_SC_PAGESIZE);
    unsigned long size = page_size * sizeof(uintptr_t);
    return mprotect((void *) ((uintptr_t) addr - ((uintptr_t) addr % page_size) - page_size), (size_t) size, PROT_EXEC | PROT_READ | PROT_WRITE) == 0 && memcpy(addr, buffer, length) != 0;
}

template<typename T>
void Write(uintptr_t addr, T value) {
    WriteAddr((void *) addr, &value, sizeof(T));
}

using json = nlohmann::json;
using namespace SDK;
#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
// ======================================================================== //
bool initImGui = false;
int screenWidth = -1, glWidth, screenHeight = -1, glHeight;
float density = -1;
json items_data;
time_t rng = 0;
std::string g_Token, g_Auth;
bool bValid = false;
#define SLEEP_TIME 1000LL / 120LL
// ======================================================================== //

enum EAimTarget {
    Head = 0,
    Chest = 1
};

enum EAimTrigger {
    None = 0,
    Shooting = 1,
    Scoping = 2,
    Both = 3,
    Any = 4
};

struct sConfig {
	    
    struct sESPMenu {
        bool Line;
        bool Box;
        bool Skeleton;
        bool Health;
        bool Name;
        bool Distance;
        bool TeamID;
		bool Alert;
		bool Vehicle;
		bool LootBox;
        bool EnemyWeapon;
		bool Grenade;
    };
    sESPMenu ESPMenu;
    
    struct sAimMenu
	{
		bool Enable;
        float FOVSize;
		EAimTarget Target;
		EAimTrigger Trigger;
		bool IgnoreKnocked;
		bool IgnoreBot;
		bool VisCheck;
	};
	sAimMenu BulletTrack{0};
	
	struct sHighRisk {
        bool Shake;
        bool Recoil;
        bool Instant;
        bool HitEffect;
		bool WideView;
	    bool Flash;
		bool Crosshair;
		bool FSwitch;
    };
    sHighRisk HighRisk{0};

    struct sColorsESP {
        float *Line;
        float *Box;
        float *Name;
        float *Distance;
        float *Skeleton;
        float *Vehicle;
		float *Fov;
		float *WindowBG;
		float *FrameBG;
        float *FCircle;
		float *Color;
    };
    sColorsESP ColorsESP{0};
};
sConfig Config{0};

#define CREATE_COLOR(r, g, b, a) new float[4] {(float)r, (float)g, (float)b, (float)a};
// ======================================================================== //
uintptr_t g_UE4;
android_app *g_App = 0;
ASTExtraPlayerCharacter *g_LocalPlayer = 0;
ASTExtraPlayerController *g_LocalController = 0;

#define Actors_Offset 0x586C180
#define GNames_Offset 0x4041e90
#define GUObject_Offset 0x86e0650
#define GNativeApp_Offset 0x83620dc
//#define Actors_Offset 0x70

struct sRegion {
    uintptr_t start, end;
};

std::vector<sRegion> trapRegions;

bool isObjectInvalid(UObject *obj) {
    if (!Tools::IsPtrValid(obj)) {
        return true;
    }

    if (!Tools::IsPtrValid(obj->ClassPrivate)) {
        return true;
    }

    if (obj->InternalIndex <= 0) {
        return true;
    }

    if (obj->NamePrivate.ComparisonIndex <= 0) {
        return true;
    }

    if ((uintptr_t)(obj) % sizeof(uintptr_t) != 0x0 && (uintptr_t)(obj) % sizeof(uintptr_t) != 0x4) {
        return true;
    }

    if (std::any_of(trapRegions.begin(), trapRegions.end(), [obj](sRegion region) { return ((uintptr_t) obj) >= region.start && ((uintptr_t) obj) <= region.end; }) ||
        std::any_of(trapRegions.begin(), trapRegions.end(), [obj](sRegion region) { return ((uintptr_t) obj->ClassPrivate) >= region.start && ((uintptr_t) obj->ClassPrivate) <= region.end; })) {
        return true;
    }

    return false;
}

static UEngine *GEngine = 0;
UWorld *GetWorld() {
    while (!GEngine) {
        GEngine = UObject::FindObject<UEngine>("UAEGameEngine Transient.UAEGameEngine_1");
        sleep(1);
    }
    if (GEngine) {
        auto ViewPort = GEngine->GameViewport;
        if (ViewPort) {
            return ViewPort->World;
        }
    }
    return 0;
}

TNameEntryArray *GetGNames() {
    return ((TNameEntryArray *(*)()) (g_UE4 + GNames_Offset))();
}

TArray<AActor *> getActors() {
    auto World = GetWorld();
    if (World) {
        auto PersistentLevel = World->PersistentLevel;
        if (PersistentLevel) {
            return *(TArray<AActor *> *) ((uintptr_t) PersistentLevel + Actors_Offset);
        }
    }
    return TArray<AActor *>();
}
/*
std::vector<AActor *> getActors() {
    auto World = GetWorld();
    if (!World)
        return std::vector<AActor *>();
 
    auto PersistentLevel = World->PersistentLevel;
    if (!PersistentLevel)
        return std::vector<AActor *>();
 
    struct GovnoArray {
        uintptr_t base;
        int32_t count;
        int32_t max;
    };
    static thread_local GovnoArray Actors{};
 
    Actors = *(((GovnoArray*(*)(uintptr_t))(UE4 + GetActorArray))(reinterpret_cast<uintptr_t>(PersistentLevel)));
 
    if (Actors.count <= 0) {
        return {};
    }
 
    std::vector<AActor *> actors;
    for (int i = 0; i < Actors.count; i++) {
        auto Actor = *(uintptr_t *) (Actors.base + (i * sizeof(uintptr_t)));
        if (Actor) {
            actors.push_back(reinterpret_cast<AActor *const>(Actor));
        }
    }
    return actors;
}*/
// ======================================================================== //
std::string getObjectPath(UObject *Object) {
    std::string s;
    for (auto super = Object->ClassPrivate; super; super = (UClass *) super->SuperStruct) {
        if (!s.empty())
            s += ".";
        s += super->NamePrivate.GetName();
    }
    return s;
}

// ======================================================================== //
int32_t ToColor(float *col) {
    return ImGui::ColorConvertFloat4ToU32(*(ImVec4 *) (col));
}
//==================================================//
FRotator ToRotator(FVector local, FVector target) {
    FVector rotation = UKismetMathLibrary::Subtract_VectorVector(local, target);
    float hyp = sqrt(rotation.X * rotation.X + rotation.Y * rotation.Y);
    FRotator newViewAngle = {0};
    newViewAngle.Pitch = -atan(rotation.Z / hyp) * (180.f / (float) 3.14159265358979323846);
    newViewAngle.Yaw = atan(rotation.Y / rotation.X) * (180.f / (float) 3.14159265358979323846);
    newViewAngle.Roll = (float) 0.f;
    if (rotation.X >= 0.f)
        newViewAngle.Yaw += 180.0f;
    return newViewAngle;
}

#define W2S(w, s) UGameplayStatics::ProjectWorldToScreen(localController, w, true, s)
//=====BulletTrack360°======//
auto *GetTargetByDistance() {
    ASTExtraPlayerCharacter *result = 0;
    float max = std::numeric_limits<float>::infinity();
	
	auto GWorld = GetWorld();
    if (GWorld) {
        if (GWorld->PersistentLevel) {
            auto Actors = *(TArray<AActor *> *) ((uintptr_t) GWorld->PersistentLevel + Actors_Offset);
	
	auto localPlayer = g_LocalPlayer;
    auto localController = g_LocalController;

                if (localPlayer) {
                for (int i = 0; i < Actors.Num(); i++) {
                    auto Actor = Actors[i];
                    if (isObjectInvalid(Actor))
                        continue;

                    if (Actor->IsA(ASTExtraPlayerCharacter::StaticClass())) {
                        auto Player = (ASTExtraPlayerCharacter *) Actor;
                        
                        if (Player->PlayerKey == localPlayer->PlayerKey)
                            continue;

                        if (Player->TeamID == localPlayer->TeamID)
                            continue;

                        if (Player->bDead)
                            continue;
							
						if (Config.BulletTrack.IgnoreKnocked) {
                            if (Player->Health == 0.0f)
                                continue;
                        }

                        if (Config.BulletTrack.IgnoreBot) {
                            if (Player->bIsAI)
                                continue;
                        }

						float dist = localPlayer->GetDistanceTo(Player);
                        if (dist < max) {
                            max = dist;
                            result = Player;
                        }
                    }
                }
            }
		}
	}
    return result;
}
//==================================================================//
const char *GetVehicleName(ASTExtraVehicleBase *Vehicle) {
    switch (Vehicle->VehicleShapeType) {
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Motorbike:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Motorbike_SideCart:
            return "Motorbike";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Dacia:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_HeavyDacia:
            return "Dacia";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_MiniBus:
            return "Mini Bus";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_PickUp:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_PickUp01:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_HeavyPickup:
            return "Pick Up";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Buggy:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_HeavyBuggy:
            return "Buggy";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_UAZ:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_UAZ01:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_UAZ02:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_UAZ03:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_HeavyUAZ:
            return "UAZ";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_PG117:
            return "PG117";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Aquarail:
            return "Aquarail";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Mirado:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Mirado01:
            return "Mirado";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Rony:
            return "Rony";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Scooter:
            return "Scooter";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_SnowMobile:
            return "Snow Mobile";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_TukTukTuk:
            return "Tuk Tuk";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_SnowBike:
            return "Snow Bike";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Surfboard:
            return "Surf Board";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Snowboard:
            return "Snow Board";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Amphibious:
            return "Amphibious";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_LadaNiva:
            return "Lada Niva";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_UAV:
            return "UAV";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_MegaDrop:
            return "Mega Drop";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Lamborghini:
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_Lamborghini01:
            return "Lamborghini";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_GoldMirado:
            return "Gold Mirado";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_BigFoot:
            return "Big Foot";
            break;
        case ESTExtraVehicleShapeType::ESTExtraVehicleShapeType__VST_HeavyUH60:
            return "UH60";
            break;
        default:
            return "Vehicle";
            break;
    }
    return "Vehicle";
}

const char *GetPickUpName(APlayerTombBox *APickUpListWrapperActor)
{
	switch (APickUpListWrapperActor->BoxType)
	{
	case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_TombBox:
	    return "DeadBox";
		break;
	case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_AirDropBox:
		return "AirDrop";
		break;
    case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_TreasureBox:
        return "TreasureBox";
		break;
    case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_MonsterTombBox:
        return "MonsterDeadBox";
		break;
    case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_VehicleBox:
        return "VehicleBox";
		break;
	case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_DeadRemainBox:
        return "DeadRemainBox";
		break;
    case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_ResourceBox:
        return "ResourceBox";
		break;
	case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_LootBox:
        return "LootBox";
		break;
    case EPickUpBoxType::EPickUpBoxType__EPickUpBoxType_MAX:
        return "MAX";
		break;
	default:
		return "LootBox";
		break;
	}
	return "LootBox";
}

FVector2D pushToScreenBorder(FVector2D Pos, FVector2D screen, int borders, int offset) {
    int x = (int)Pos.X;
    int y = (int)Pos.Y;
    if ((borders & 1) == 1) {
        y = 0 - offset;
    }
    if ((borders & 2) == 2) {
        x = (int)screen.X + offset;
    }
    if ((borders & 4) == 4) {
        y = (int)screen.Y + offset;
    }
    if ((borders & 8) == 8) {
        x = 0 - offset;
    }
    return FVector2D(x, y);
}

int isOutsideSafezone(FVector2D pos, FVector2D screen) {
    FVector2D mSafezoneTopLeft(screen.X * 0.04f, screen.Y * 0.04f);
    FVector2D mSafezoneBottomRight(screen.X * 0.96f, screen.Y * 0.96f);
    int result = 0;
    if (pos.Y < mSafezoneTopLeft.Y) {
        result |= 1;
    }
    if (pos.X > mSafezoneBottomRight.X) {
        result |= 2;
    }
    if (pos.Y > mSafezoneBottomRight.Y) {
        result |= 4;
    }
    if (pos.X < mSafezoneTopLeft.X) {
        result |= 8;
    }
    return result;
}

//===================== ESP DRAW =====================//
void DrawESP(ImDrawList *draw) {
    auto GWorld = GetWorld();
    if (GWorld) {
        if (GWorld->PersistentLevel) {
            auto Actors = *(TArray<AActor *> *) ((uintptr_t) GWorld->PersistentLevel + Actors_Offset);

            int totalEnemies = 0, totalBots = 0;
            
            ASTExtraPlayerCharacter *localPlayer = 0;
            ASTExtraPlayerController *localController = 0;
                     
				for (int i = 0; i < Actors.Num(); i++) {
                auto Actor = Actors[i];
                if (isObjectInvalid(Actor))
                    continue;

                if (Actor->IsA(ASTExtraPlayerController::StaticClass())) {
                    localController = (ASTExtraPlayerController *) Actor;
                    break;
                }
            }
		
			
            if (localController) {
                for (int i = 0; i < Actors.Num(); i++) {
                    auto Actor = Actors[i];
                    if (isObjectInvalid(Actor))
                        continue;

                    if (Actor->IsA(ASTExtraPlayerCharacter::StaticClass())) {
                        if (((ASTExtraPlayerCharacter *) Actor)->PlayerKey == localController->PlayerKey) {
                            localPlayer = (ASTExtraPlayerCharacter *) Actor;
                            break;
                        }
                    }
                }


                if (localPlayer) {
                    if (localPlayer->PartHitComponent) {
                        auto ConfigCollisionDistSqAngles = localPlayer->PartHitComponent->ConfigCollisionDistSqAngles;
                        for (int j = 0; j < ConfigCollisionDistSqAngles.Num(); j++) {
                            ConfigCollisionDistSqAngles[j].Angle = 90.0f;
                        }
                        localPlayer->PartHitComponent->ConfigCollisionDistSqAngles = ConfigCollisionDistSqAngles;
                    }
	//===================BULLET-TRACK====================//
	if (Config.BulletTrack.Enable){
	draw->AddCircle(ImVec2(screenWidth / 3.00f, screenHeight / 3.00f), Config.BulletTrack.FOVSize*0.5f, IM_COL32(25, 255, 25, 255), 100, 1.0f);

		ASTExtraPlayerCharacter *Target = GetTargetByDistance();
				
			bool bReady = localPlayer->bIsWeaponFiring;
			if (bReady) {
			
			if (Target) {
			FVector targetAimPos = Target->GetBonePos("Head", {});
            if (Config.BulletTrack.Target == EAimTarget::Chest) {
				targetAimPos.Z -= 25.0f;
            }
			  
			auto WeaponManagerComponent = localPlayer->WeaponManagerComponent;
			if (WeaponManagerComponent) {
							
			auto CurrentWeaponReplicated = (ASTExtraShootWeapon *)WeaponManagerComponent->CurrentWeaponReplicated;
			if (CurrentWeaponReplicated) {
				
			auto ShootWeaponEntityComp = CurrentWeaponReplicated->ShootWeaponEntityComp;
            if (ShootWeaponEntityComp) {
               
                   ASTExtraVehicleBase *CurrentVehicle = Target->CurrentVehicle;
                   if (CurrentVehicle) {
                       FVector LinearVelocity = CurrentVehicle->ReplicatedMovement.LinearVelocity;

                       float dist = localPlayer->GetDistanceTo(Target);
                       auto timeToTravel = dist / ShootWeaponEntityComp->BulletRange;

                       targetAimPos = UKismetMathLibrary::Add_VectorVector(targetAimPos, UKismetMathLibrary::Multiply_VectorFloat(LinearVelocity, timeToTravel));
                       } else {
                       FVector Velocity = Target->GetVelocity();

                       float dist = localPlayer->GetDistanceTo(Target);
                       auto timeToTravel = dist / ShootWeaponEntityComp->BulletRange;

                       targetAimPos = UKismetMathLibrary::Add_VectorVector(targetAimPos, UKismetMathLibrary::Multiply_VectorFloat(Velocity, timeToTravel));
                          }
                       }
                         
						   FRotator aimRotation = ToRotator(localController->PlayerCameraManager->CameraCache.POV.Location,targetAimPos);
						   localController->PlayerCameraManager->CameraCache.POV.Rotation = aimRotation;
					
						}
					}
				}
			}
		}
				//===================ESP-VISUAL====================//
                for (int i = 0; i < Actors.Num(); i++) {
                     auto Actor = Actors[i];
                     if (isObjectInvalid(Actor))
                         continue;
						 
                            if (Actor->IsA(ASTExtraPlayerCharacter::StaticClass())) {
                            long PlayerBoxClrCf = IM_COL32(255, 000, 000, 255);
                            auto Player = (ASTExtraPlayerCharacter *) Actor;
							if (!localController->LineOfSightTo(Player, {0, 0, 0}, true)) {
                                PlayerBoxClrCf = IM_COL32(255, 255, 255, 255);
                            }
							
                            float Distance = localPlayer->GetDistanceTo(Player) / 100.0f;
							if (Distance > 500.0f)
								continue;
							
                            if (Player->PlayerKey == localPlayer->PlayerKey)
								continue;
							
							if (Player->TeamID == localPlayer->TeamID)
								continue;
							
							if (Player->bDead)
								continue;

                            if (Player->bIsAI)
								totalBots++;
								else totalEnemies++;
							
							auto HeadPos = Player->GetBonePos("Head", {});
							ImVec2 HeadPosSC;
							auto RootPos = Player->GetBonePos("Root", {});
							ImVec2 RootPosSC;
							
                            if (W2S(HeadPos, (FVector2D *) &HeadPosSC) && W2S(RootPos, (FVector2D *) &RootPosSC)) {
								
                                if (Config.ESPMenu.Line) {
                                    draw->AddLine({(float) glWidth / 2, 0}, HeadPosSC,
                                                  PlayerBoxClrCf, 0.6f);
                                }

                                if (Config.ESPMenu.Box) {
                                    float boxHeight = abs(HeadPosSC.y - RootPosSC.y);
                                    float boxWidth = boxHeight * 0.65f;
                                    ImVec2 vStart = {HeadPosSC.x - (boxWidth / 2), HeadPosSC.y};
                                    ImVec2 vEnd = {vStart.x + boxWidth, vStart.y + boxHeight};
                                    draw->AddRect(vStart, vEnd, PlayerBoxClrCf, 1.5f, 240, 1.7f);
                                }

                                if (Config.ESPMenu.Skeleton) {
                                    static std::vector<std::string> right_arm{"neck_01",
                                                                              "clavicle_r",
                                                                              "upperarm_r",
                                                                              "lowerarm_r",
                                                                              "hand_r", "item_r"};
                                    static std::vector<std::string> left_arm{"neck_01",
                                                                             "clavicle_l",
                                                                             "upperarm_l",
                                                                             "lowerarm_l",
                                                                             "hand_l", "item_l"};
                                    static std::vector<std::string> spine{"Head", "neck_01",
                                                                          "spine_03",
                                                                          "spine_02", "spine_01",
                                                                          "pelvis"};
                                    static std::vector<std::string> lower_right{"pelvis", "thigh_r",
                                                                                "calf_r", "foot_r"};
                                    static std::vector<std::string> lower_left{"pelvis", "thigh_l",
                                                                               "calf_l", "foot_l"};
                                    static std::vector<std::vector<std::string>> skeleton{right_arm,
                                                                                          left_arm,
                                                                                          spine,
                                                                                          lower_right,
                                                                                          lower_left};

                                    for (auto &boneStructure: skeleton) {
                                        std::string lastBone;
                                        for (std::string &currentBone: boneStructure) {
                                            if (!lastBone.empty()) {
                                                ImVec2 boneFrom, boneTo;
                                                if (W2S(Player->GetBonePos(lastBone.c_str(), {}),
                                                        (FVector2D *) &boneFrom) &&
                                                    W2S(Player->GetBonePos(currentBone.c_str(), {}),
                                                        (FVector2D *) &boneTo)) {
                                                    draw->AddLine(boneFrom, boneTo,
                                                                  PlayerBoxClrCf, 1.0f);
                                                }
                                            }
                                            lastBone = currentBone;
                                        }
                                    }
                                }

                            if (Config.ESPMenu.Health)
							{
								int CurHP = (int)std::max(0, std::min((int)Player->Health, (int)Player->HealthMax));
								int MaxHP = (int)Player->HealthMax;
								long HPColor = IM_COL32(std::min(((510 * (MaxHP - CurHP)) / MaxHP), 255), std::min((510 * CurHP) / MaxHP, 255), 0, 155);
								if (Player->Health == 0.0f && !Player->bDead)
								{
									HPColor = IM_COL32(255, 0, 0, 155);
									CurHP = Player->NearDeathBreath;
									if (Player->NearDeatchComponent)
									{
										MaxHP = Player->NearDeatchComponent->BreathMax;
									}
								}
								float boxWidth = density / 1.6f;
								boxWidth -= std::min(((boxWidth / 2) / 00.0f) * Distance, boxWidth / 2);
								float boxHeight = boxWidth * 0.07f;
								ImVec2 vStart = {HeadPosSC.x - (boxWidth / 2), HeadPosSC.y - (boxHeight * 2.1f)};
								ImVec2 vEndFilled = {vStart.x + (CurHP * boxWidth / MaxHP), vStart.y + boxHeight};
								ImVec2 vEndRect = {vStart.x + boxWidth, vStart.y + boxHeight};
								draw->AddRectFilled(vStart, vEndFilled, HPColor); //HP Color
								draw->AddRect(vStart, vEndRect, IM_COL32(0, 0, 0, 185)); //Border Color
							}
							
							
							if (Config.ESPMenu.TeamID || Config.ESPMenu.Name || Config.ESPMenu.Distance)
			     			{
								float boxWidth = density / 1.6f;
								boxWidth -= std::min(((boxWidth / 2) / 00.0f) * Distance, boxWidth / 2);
								float boxHeight = boxWidth * 0.07f;
                                float NameboxHeight = boxWidth * 0.20f;
                                ImVec2 vStart = {HeadPosSC.x - (boxWidth / 2), HeadPosSC.y - (NameboxHeight * 1.73f)};
                                ImVec2 vEndRect = {vStart.x + boxWidth, vStart.y + NameboxHeight};
								draw->AddRectFilled(vStart, vEndRect, IM_COL32(0, 0, 0, 75));//Background Color
								
								if (Config.ESPMenu.TeamID)
									{
										float boxWidth = density / 1.8f;
										boxWidth -= std::min(((boxWidth / 2) / 00.0f) * Distance, boxWidth / 2);
										float boxHeight = boxWidth * 0.19f;
										std::string s;
										s += std::to_string(Player->TeamID);
										draw->AddText(NULL, ((float)density / 30.0f), {HeadPosSC.x - (boxWidth / 2), HeadPosSC.y - (boxHeight * 1.83f)}, IM_COL32(255, 255, 255, 255), s.c_str());
									}
								if (Config.ESPMenu.Name)
									{
										float boxWidth = density / 1.8f;
										boxWidth -= std::min(((boxWidth / 2) / 00.0f) * Distance, boxWidth / 2);
										float boxHeight = boxWidth * 0.19f;
										std::string s;
										if (Player->bIsAI)
											{
												s += "  BOT";
											}
										else
											{
												s += Player->PlayerName.ToString();
											}
										draw->AddText(NULL, ((float)density / 30.0f), {HeadPosSC.x - (boxWidth / 3), HeadPosSC.y - (boxHeight * 1.83f)}, IM_COL32(255, 255, 255, 255), s.c_str());
									}
								if (Config.ESPMenu.Distance)
									{
										float boxWidth = density / 1.8f;
										boxWidth -= std::min(((boxWidth / 2) / 00.0f) * Distance, boxWidth / 2);
										float boxHeight = boxWidth * 0.19f;
										std::string s;
										s += std::to_string((int)Distance);
										s += "m";
										draw->AddText(NULL, ((float)density / 30.0f), {HeadPosSC.x + (boxWidth / 3), HeadPosSC.y - (boxHeight * 1.83f)}, IM_COL32(255, 255, 255, 255), s.c_str());
									}
							}
								
							FVector2D screen(glWidth, glHeight);
							FVector2D location(RootPosSC.x, HeadPosSC.y);
							int borders = isOutsideSafezone(location, screen);
							if (Config.ESPMenu.Alert && borders != 0) 
								{
									float Distance = localPlayer->GetDistanceTo(Player) / 100.0f;
									std::string s;
									s += std::to_string((int)Distance);
									s += "M";
									float mScale = glHeight / (float) 1080;
									auto hintDotRenderPos = pushToScreenBorder(location, screen, borders, (int)((mScale * 100) / 3));
									auto hintTextRenderPos = pushToScreenBorder(location, screen, borders, -(int)((mScale * 36)));
									draw->AddCircleFilled(ImVec2(hintDotRenderPos.X, hintDotRenderPos.Y), mScale * 100, IM_COL32(255, 0, 0, 128), 0);
									draw->AddText(NULL, ((float)density / 30.0f), ImVec2(hintTextRenderPos.X, hintTextRenderPos.Y), IM_COL32(255, 255, 255, 255), s.c_str());
								}
                            
                            if (Config.ESPMenu.EnemyWeapon) {
                                auto WeaponManagerComponent = Player->WeaponManagerComponent;
                                if (WeaponManagerComponent) {
                                        auto CurrentWeaponReplicated = (ASTExtraShootWeapon *)WeaponManagerComponent->CurrentWeaponReplicated;
                                        if (CurrentWeaponReplicated) {
                                            auto WeaponId = (int)CurrentWeaponReplicated->GetWeaponID();
                                            if (WeaponId) {
                                                std::string s;
                                                s += CurrentWeaponReplicated->GetWeaponName().ToString();
                                                auto textSize = ImGui::CalcTextSize2(s.c_str(), 0, ((float) density / 30.0f));
                                                draw->AddText(NULL, ((float) density / 30.0f), {RootPosSC.x - (textSize.x / 2), RootPosSC.y}, IM_COL32(255, 255, 255, 255), s.c_str());
                                            }
                                        }
                                    }
                                }
								
                            }
                        }
						
					if (Config.ESPMenu.Vehicle) {
						if (Actors[i]->IsA(ASTExtraVehicleBase::StaticClass())) {
							auto Vehicle = (ASTExtraVehicleBase *)Actors[i];
							if (!Vehicle->Mesh)
								continue;
							int CurHP = (int) std::max(0, std::min((int) Vehicle->VehicleCommon->HP, (int) Vehicle->VehicleCommon->HPMax));
                            int MaxHP = (int) Vehicle->VehicleCommon->HPMax;
                            long curHP_Color = IM_COL32(std::min(((510 * (MaxHP - CurHP)) / MaxHP), 255), std::min(((510 * CurHP) / MaxHP), 255), 0, 155);
							float Distance = Vehicle->GetDistanceTo(localPlayer) / 100.f;
							FVector2D vehiclePos;
							if (W2S(Vehicle->K2_GetActorLocation(), &vehiclePos))
							{
								auto mWidthScale = std::min(0.10f * Distance, 50.f);
								auto mWidth = 85.0f - mWidthScale;
								auto mHeight = mWidth * 0.07f;
									std::string s = GetVehicleName(Vehicle);
				     				s += " [";
						    		s += std::to_string((int)Distance);
					    			s += "m]";
				     	    		draw->AddText(NULL, ((float)density / 28.0f), {vehiclePos.X - (mWidth / 2), vehiclePos.Y}, IM_COL32(000, 255, 255, 255), s.c_str());
							}
						}
					}
					
					if (Config.ESPMenu.LootBox) {
                        if (Actors[i]->IsA(APlayerTombBox::StaticClass())) {
                            auto APickUpListWrapperActor = (APlayerTombBox *) Actors[i];
                            auto RootComponent = APickUpListWrapperActor->RootComponent;
                            if (!RootComponent)
                                continue;
                            float Distance = APickUpListWrapperActor->GetDistanceTo(localPlayer) / 100.0f;
                            FVector2D lootboxPos;
                            if (W2S(APickUpListWrapperActor->K2_GetActorLocation(), &lootboxPos)) {
                                std::string s = GetPickUpName(APickUpListWrapperActor);
                                s += " [";
                                s += std::to_string((int) Distance);
                                s += "m]";
                                draw->AddText(NULL, ((float) density / 24.0f),
                                              {lootboxPos.X, lootboxPos.Y},
                                              IM_COL32(000, 255, 000, 255), s.c_str());
                            }
                        }
                    }
					
					if (Config.ESPMenu.Grenade) {
                            if (Actors[i]->IsA(ASTExtraGrenadeBase::StaticClass())) {
                                auto Grenade = (ASTExtraGrenadeBase *) Actors[i];
                                auto RootComponent = Grenade->RootComponent;
                                if (!RootComponent)
                                    continue;
                                float Distance = Grenade->GetDistanceTo(localPlayer) / 100.0f;

                                FVector2D grenadePos;
                                if (W2S(Grenade->K2_GetActorLocation(), &grenadePos)) {
                                    std::string s = ICON_FA_BOMB;
                                    s += " [";
                                    s += std::to_string((int) Distance);
                                    s += "m]";
                                    draw->AddText(NULL, ((float) density / 15.0f), {((float)glWidth / 2) - (glWidth / 10), 100}, IM_COL32(255, 0, 0, 255), "!!!...THROWABLE WARNING...!!!");
                                    draw->AddText(NULL, ((float) density / 20.0f), {grenadePos.X, grenadePos.Y}, IM_COL32(255, 0, 0, 255), s.c_str());
                                }
                            }
                        }
						
                   }
               }
            }
      

    g_LocalController = localController;
    g_LocalPlayer = localPlayer;
    
     if (totalEnemies > 0 || totalBots > 0) {
                std::string s;
                if (totalEnemies > 0) {
                    s = "Enem";
                    if (totalEnemies > 1)
                        s += "ies";
                    else s += "y";
                    s += " Around: ";
                    s += std::to_string(totalEnemies);
                    if (totalBots > 0)
                        s += " | ";
                }
                if (totalBots) {
                    s += "Bot";
                    if (totalBots > 1)
                        s += "s";
                    s += " Around: ";
                    s += std::to_string(totalBots);
                }

        auto textSize = ImGui::CalcTextSize(s.c_str(), 0, s.size());
        draw->AddText(NULL, ((float) density / 12.0f), {((float) glWidth / 2) - (textSize.x / 2), 100}, IM_COL32(000, 255, 000, 255), s.c_str());
    }
                
        }
    }
}
// ======================================================================== //

std::string getClipboardText() {
    if (!g_App)
        return "";

    auto activity = g_App->activity;
    if (!activity)
        return "";

    auto vm = activity->vm;
    if (!vm)
        return "";

    auto object = activity->clazz;
    if (!object)
        return "";

    std::string result;

    JNIEnv *env;
    vm->AttachCurrentThread(&env, 0);
    {
        auto ContextClass = env->FindClass("android/content/Context");
        auto getSystemServiceMethod = env->GetMethodID(ContextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
        auto str = env->NewStringUTF("clipboard");
        auto clipboardManager = env->CallObjectMethod(object, getSystemServiceMethod, str);
        env->DeleteLocalRef(str);
        auto ClipboardManagerClass = env->FindClass("android/content/ClipboardManager");
        auto getText = env->GetMethodID(ClipboardManagerClass, "getText", "()Ljava/lang/CharSequence;");
        auto CharSequenceClass = env->FindClass("java/lang/CharSequence");
        auto toStringMethod = env->GetMethodID(CharSequenceClass, "toString", "()Ljava/lang/String;");
        auto text = env->CallObjectMethod(clipboardManager, getText);
        if (text) {
            str = (jstring) env->CallObjectMethod(text, toStringMethod);
            result = env->GetStringUTFChars(str, 0);
            env->DeleteLocalRef(str);
            env->DeleteLocalRef(text);
        }

        env->DeleteLocalRef(CharSequenceClass);
        env->DeleteLocalRef(ClipboardManagerClass);
        env->DeleteLocalRef(clipboardManager);
        env->DeleteLocalRef(ContextClass);
    }
    vm->DetachCurrentThread();

    return result;
}
// ======================================================================== //
const char *GetAndroidID(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass(/*android/content/Context*/ StrEnc("`L+&0^[S+-:J^$,r9q92(as", "\x01\x22\x4F\x54\x5F\x37\x3F\x7C\x48\x42\x54\x3E\x3B\x4A\x58\x5D\x7A\x1E\x57\x46\x4D\x19\x07", 23).c_str());
    jmethodID getContentResolverMethod = env->GetMethodID(contextClass, /*getContentResolver*/ StrEnc("E8X\\7r7ys_Q%JS+L+~", "\x22\x5D\x2C\x1F\x58\x1C\x43\x1C\x1D\x2B\x03\x40\x39\x3C\x47\x3A\x4E\x0C", 18).c_str(), /*()Landroid/content/ContentResolver;*/ StrEnc("8^QKmj< }5D:9q7f.BXkef]A*GYLNg}B!/L", "\x10\x77\x1D\x2A\x03\x0E\x4E\x4F\x14\x51\x6B\x59\x56\x1F\x43\x03\x40\x36\x77\x28\x0A\x08\x29\x24\x44\x33\x0B\x29\x3D\x08\x11\x34\x44\x5D\x77", 35).c_str());
    jclass settingSecureClass = env->FindClass(/*android/provider/Settings$Secure*/ StrEnc("T1yw^BCF^af&dB_@Raf}\\FS,zT~L(3Z\"", "\x35\x5F\x1D\x05\x31\x2B\x27\x69\x2E\x13\x09\x50\x0D\x26\x3A\x32\x7D\x32\x03\x09\x28\x2F\x3D\x4B\x09\x70\x2D\x29\x4B\x46\x28\x47", 32).c_str());
    jmethodID getStringMethod = env->GetStaticMethodID(settingSecureClass, /*getString*/ StrEnc("e<F*J5c0Y", "\x02\x59\x32\x79\x3E\x47\x0A\x5E\x3E", 9).c_str(), /*(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;*/ StrEnc("$6*%R*!XO\"m18o,0S!*`uI$IW)l_/_knSdlRiO1T`2sH|Ouy__^}%Y)JsQ:-\"(2_^-$i{?H", "\x0C\x7A\x4B\x4B\x36\x58\x4E\x31\x2B\x0D\x0E\x5E\x56\x1B\x49\x5E\x27\x0E\x69\x0F\x1B\x3D\x41\x27\x23\x7B\x09\x2C\x40\x33\x1D\x0B\x21\x5F\x20\x38\x08\x39\x50\x7B\x0C\x53\x1D\x2F\x53\x1C\x01\x0B\x36\x31\x39\x46\x0C\x15\x43\x2B\x05\x30\x15\x41\x43\x46\x55\x70\x0D\x59\x56\x00\x15\x58\x73", 71).c_str());

    auto obj = env->CallObjectMethod(context, getContentResolverMethod);
    auto str = (jstring) env->CallStaticObjectMethod(settingSecureClass, getStringMethod, obj, env->NewStringUTF(/*android_id*/ StrEnc("ujHO)8OfOE", "\x14\x04\x2C\x3D\x46\x51\x2B\x39\x26\x21", 10).c_str()));
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceModel(JNIEnv *env) {
    jclass buildClass = env->FindClass(/*android/os/Build*/ StrEnc("m5I{GKGWBP-VOxkA", "\x0C\x5B\x2D\x09\x28\x22\x23\x78\x2D\x23\x02\x14\x3A\x11\x07\x25", 16).c_str());
    jfieldID modelId = env->GetStaticFieldID(buildClass, /*MODEL*/ StrEnc("|}[q:", "\x31\x32\x1F\x34\x76", 5).c_str(), /*Ljava/lang/String;*/ StrEnc(".D:C:ETZ1O-Ib&^h.Y", "\x62\x2E\x5B\x35\x5B\x6A\x38\x3B\x5F\x28\x02\x1A\x16\x54\x37\x06\x49\x62", 18).c_str());

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceBrand(JNIEnv *env) {
    jclass buildClass = env->FindClass(/*android/os/Build*/ StrEnc("0iW=2^>0zTRB!B90", "\x51\x07\x33\x4F\x5D\x37\x5A\x1F\x15\x27\x7D\x00\x54\x2B\x55\x54", 16).c_str());
    jfieldID modelId = env->GetStaticFieldID(buildClass, /*BRAND*/ StrEnc("@{[FP", "\x02\x29\x1A\x08\x14", 5).c_str(), /*Ljava/lang/String;*/ StrEnc(".D:C:ETZ1O-Ib&^h.Y", "\x62\x2E\x5B\x35\x5B\x6A\x38\x3B\x5F\x28\x02\x1A\x16\x54\x37\x06\x49\x62", 18).c_str());

    auto str = (jstring) env->GetStaticObjectField(buildClass, modelId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetPackageName(JNIEnv *env, jobject context) {
    jclass contextClass = env->FindClass(/*android/content/Context*/ StrEnc("`L+&0^[S+-:J^$,r9q92(as", "\x01\x22\x4F\x54\x5F\x37\x3F\x7C\x48\x42\x54\x3E\x3B\x4A\x58\x5D\x7A\x1E\x57\x46\x4D\x19\x07", 23).c_str());
    jmethodID getPackageNameId = env->GetMethodID(contextClass, /*getPackageName*/ StrEnc("YN4DaP)!{wRGN}", "\x3E\x2B\x40\x14\x00\x33\x42\x40\x1C\x12\x1C\x26\x23\x18", 14).c_str(), /*()Ljava/lang/String;*/ StrEnc("VnpibEspM(b]<s#[9cQD", "\x7E\x47\x3C\x03\x03\x33\x12\x5F\x21\x49\x0C\x3A\x13\x20\x57\x29\x50\x0D\x36\x7F", 20).c_str());

    auto str = (jstring) env->CallObjectMethod(context, getPackageNameId);
    return env->GetStringUTFChars(str, 0);
}

const char *GetDeviceUniqueIdentifier(JNIEnv *env, const char *uuid) {
    jclass uuidClass = env->FindClass(/*java/util/UUID*/ StrEnc("B/TxJ=3BZ_]SFx", "\x28\x4E\x22\x19\x65\x48\x47\x2B\x36\x70\x08\x06\x0F\x3C", 14).c_str());

    auto len = strlen(uuid);

    jbyteArray myJByteArray = env->NewByteArray(len);
    env->SetByteArrayRegion(myJByteArray, 0, len, (jbyte *) uuid);

    jmethodID nameUUIDFromBytesMethod = env->GetStaticMethodID(uuidClass, /*nameUUIDFromBytes*/ StrEnc("P6LV|'0#A+zQmoat,", "\x3E\x57\x21\x33\x29\x72\x79\x67\x07\x59\x15\x3C\x2F\x16\x15\x11\x5F", 17).c_str(), /*([B)Ljava/util/UUID;*/ StrEnc("sW[\"Q[W3,7@H.vT0) xB", "\x5B\x0C\x19\x0B\x1D\x31\x36\x45\x4D\x18\x35\x3C\x47\x1A\x7B\x65\x7C\x69\x3C\x79", 20).c_str());
    jmethodID toStringMethod = env->GetMethodID(uuidClass, /*toString*/ StrEnc("2~5292eW", "\x46\x11\x66\x46\x4B\x5B\x0B\x30", 8).c_str(), /*()Ljava/lang/String;*/ StrEnc("P$BMc' #j?<:myTh_*h0", "\x78\x0D\x0E\x27\x02\x51\x41\x0C\x06\x5E\x52\x5D\x42\x2A\x20\x1A\x36\x44\x0F\x0B", 20).c_str());

    auto obj = env->CallStaticObjectMethod(uuidClass, nameUUIDFromBytesMethod, myJByteArray);
    auto str = (jstring) env->CallObjectMethod(obj, toStringMethod);
    return env->GetStringUTFChars(str, 0);
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *) userp;

    mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

//=========================MAIN LOGIN =================//
std::string Login(const char *user_key) {
    if (!g_App)
        return "Internal Error";

    auto activity = g_App->activity;
    if (!activity)
        return "Internal Error";

    auto vm = activity->vm;
    if (!vm)
        return "Internal Error";

    auto object = activity->clazz;
    if (!object)
        return "Internal Error";

    JNIEnv *env;
    vm->AttachCurrentThread(&env, 0);

    std::string hwid = user_key;
    hwid += GetAndroidID(env, object);
    hwid += GetDeviceModel(env);
    hwid += GetDeviceBrand(env);

    std::string UUID = GetDeviceUniqueIdentifier(env, hwid.c_str());

    vm->DetachCurrentThread();

    std::string errMsg;

    struct MemoryStruct chunk{};
    chunk.memory = (char *) malloc(1);
    chunk.size = 0;

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ᴍᴀᴅᴇʙʏɴᴏᴄᴀꜱʜ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
       if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://kuro.admin-key.xyz/connect") ;
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, /*https*/ StrEnc("!mLBO", "\x49\x19\x38\x32\x3C", 5).c_str());
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, /*Content-Type: application/x-www-form-urlencoded*/ StrEnc("@;Ls\\(KP4Qrop`b#d3094/r1cf<c<=H)AiiBG6i|Ta66s2[", "\x03\x54\x22\x07\x39\x46\x3F\x7D\x60\x28\x02\x0A\x4A\x40\x03\x53\x14\x5F\x59\x5A\x55\x5B\x1B\x5E\x0D\x49\x44\x4E\x4B\x4A\x3F\x04\x27\x06\x1B\x2F\x6A\x43\x1B\x10\x31\x0F\x55\x59\x17\x57\x3F", 47).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        char data[4096];
        sprintf(data, /*game=PUBG&user_key=%s&serial=%s*/ StrEnc("qu2yXK,YkJyGD@ut0.u~Nb'5(:.:chK", "\x16\x14\x5F\x1C\x65\x1B\x79\x1B\x2C\x6C\x0C\x34\x21\x32\x2A\x1F\x55\x57\x48\x5B\x3D\x44\x54\x50\x5A\x53\x4F\x56\x5E\x4D\x38", 31).c_str(), user_key, UUID.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            try {
                json result = json::parse(chunk.memory);
                if (result[/*status*/ StrEnc("(>_LBm", "\x5B\x4A\x3E\x38\x37\x1E", 6).c_str()] == true) {
                    std::string token = result[/*data*/ StrEnc("fAVA", "\x02\x20\x22\x20", 4).c_str()][/*token*/ StrEnc("{>3Lr", "\x0F\x51\x58\x29\x1C", 5).c_str()].get<std::string>();
                    time_t rng = result[/*data*/ StrEnc("fAVA", "\x02\x20\x22\x20", 4).c_str()][/*rng*/ StrEnc("+n,", "\x59\x00\x4B", 3).c_str()].get<time_t>();
                    if (rng + 30 > time(0)) {
                        std::string auth = /*PUBG*/ StrEnc("Q*) ", "\x01\x7F\x6B\x67", 4).c_str();;
                        auth += "-";
                        auth += user_key;
                        auth += "-";
                        auth += UUID;
                        auth += "-";
                        auth += /*Vm8Lk7Uj2JmsjCPVPVjrLa7zgfx3uz9E*/ StrEnc("-2:uwZdV^%]?{{wHs2V,+(^NJU;kC*_{", "\x7B\x5F\x02\x39\x1C\x6D\x31\x3C\x6C\x6F\x30\x4C\x11\x38\x27\x1E\x23\x64\x3C\x5E\x67\x49\x69\x34\x2D\x33\x43\x58\x36\x50\x66\x3E", 32).c_str();
                        std::string outputAuth = Tools::CalcMD5(auth);

                        g_Token = token;
                        g_Auth = outputAuth;

                        bValid = g_Token == g_Auth;
                    }
                } else {
                    errMsg = result[/*reason*/ StrEnc("LW(3(c", "\x3E\x32\x49\x40\x47\x0D", 6).c_str()].get<std::string>();
                }
            } catch (json::exception &e) {
                errMsg = "{";
                errMsg += e.what();
                errMsg += "}\n{";
                errMsg += chunk.memory;
                errMsg += "}";
            }
        } else {
            errMsg = curl_easy_strerror(res);
        }
    }
    curl_easy_cleanup(curl);

    return bValid ? "OK" : errMsg;
}

// ======================================================================== //
void DrawTextCentered(const char *text)
{
    ImGui::Separator();
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(text).x) / 2.f);
    ImGui::Text(text);
    ImGui::Separator();
}
// ======================================================================== //
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))
EGLBoolean (*orig_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface);
EGLBoolean _eglSwapBuffers(EGLDisplay dpy, EGLSurface surface) {
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);
    if (glWidth <= 0 || glHeight <= 0)
        return eglSwapBuffers(dpy, surface);

    if (!g_App)
        return eglSwapBuffers(dpy, surface);

    screenWidth = ANativeWindow_getWidth(g_App->window);
    screenHeight = ANativeWindow_getHeight(g_App->window);
    density = AConfiguration_getDensity(g_App->config);

    //====FLOAT=====//

        if (!initImGui) {
        ImGui::CreateContext();

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 5.5f;
        style.FramePadding = ImVec2(8, 4);
        style.FrameRounding = 5.0f;
        style.FrameBorderSize = 2.0f;
		style.WindowBorderSize = 2.0f;
        style.WindowTitleAlign = ImVec2(0.5, 0.5);
		style.ButtonTextAlign = ImVec2(0.5,0.5);
		
        style.ScaleAllSizes(std::max(1.0f, density / 180.0f));
        style.ScrollbarSize /= 1;

        ImGui_ImplAndroid_Init();
        ImGui_ImplOpenGL3_Init("#version 300 es");

        ImGuiIO &io = ImGui::GetIO();

        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.IniFilename = NULL;

        ImFontConfig cfg;
        cfg.SizePixels = ((float) density / 20.0f);
        io.Fonts->AddFontDefault(&cfg);

        memset(&Config, 0, sizeof(sConfig));
// ===============================ESPCOLOR ================================== //
        Config.ColorsESP.Line = CREATE_COLOR(255, 0, 0, 255);
        Config.ColorsESP.Box = CREATE_COLOR(255, 0, 255, 255);
        Config.ColorsESP.Name = CREATE_COLOR(255, 0, 0, 255);
        Config.ColorsESP.Distance = CREATE_COLOR(255, 0, 255, 255);
        Config.ColorsESP.Skeleton = CREATE_COLOR(255, 0, 0, 255);
        Config.ColorsESP.Vehicle = CREATE_COLOR(255, 0, 0, 255);
	    Config.ColorsESP.FCircle = CREATE_COLOR(0.00f, 1.0f, 0.0f, 1.00f);
        initImGui = true;
    }

    ImGuiIO &io = ImGui::GetIO();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
    ImGui::NewFrame();

    DrawESP(ImGui::GetBackgroundDrawList());
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
        ImGui::SetNextWindowSize(ImVec2((float) glWidth * 0.48f, (float) glHeight * 0.80f), ImGuiCond_Once);
	    if (ImGui::Begin (OBFUSCATE(" BETA V2.5.3 32BIT BY @IDRAGONCHEATS "), 0, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove)) {
        static bool isLogin = true;
        if (!isLogin) {
            ImGui::Text("Please Login!!");
            ImGui::PushItemWidth(-1);
            static char s[64];
            ImGui::InputText("##key", s, sizeof s);
            ImGui::PopItemWidth();

            if (ImGui::Button(" Click Me To Paste Key ", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))) {
                auto key = getClipboardText();
                strncpy(s, key.c_str(), sizeof s);
            }

            static std::string err;
            if (ImGui::Button(" Verify Key ", ImVec2(ImGui::GetContentRegionAvailWidth(), 0))) {
                err = Login(s);
                if (err == "OK") {
                    isLogin = bValid && g_Auth == g_Token;
                }
            }

            if (!err.empty() && err != "OK") {
                ImGui::Text("Error: %s", err.c_str());
            }

            } else{
					ImGui::Spacing();
                    if (ImGui::BeginTable("split", 3)) {
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Line", &Config.ESPMenu.Line);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Skeleton", &Config.ESPMenu.Skeleton);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Health", &Config.ESPMenu.Health);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Name", &Config.ESPMenu.Name);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Distance", &Config.ESPMenu.Distance);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Team ID", &Config.ESPMenu.TeamID);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("ESP Vehicle", &Config.ESPMenu.Vehicle);
                        ImGui::TableNextColumn();
						ImGui::Checkbox("ESP LootBox", &Config.ESPMenu.LootBox);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("360° Alert", &Config.ESPMenu.Alert);
                        ImGui::TableNextColumn();
                        ImGui::Checkbox("Enemy Weapon", &Config.ESPMenu.EnemyWeapon);
                        ImGui::TableNextColumn();
						ImGui::Checkbox("Grenade Warning", &Config.ESPMenu.Grenade);
                        ImGui::TableNextColumn();
                        ImGui::EndTable();
                    }
                                    
					DrawTextCentered("AIM MENU");
                    ImGui::Spacing();
					if (ImGui::BeginTable("split", 3))
                    {
				    ImGui::TableNextColumn();
					ImGui::Checkbox("BulletTrack360°", &Config.BulletTrack.Enable);
					ImGui::TableNextColumn();
					ImGui::Checkbox("Ignore Knocked", &Config.BulletTrack.IgnoreKnocked);
					ImGui::TableNextColumn();
					ImGui::Checkbox("Ignore Bot", &Config.BulletTrack.IgnoreBot);
					ImGui::TableNextColumn();
					ImGui::EndTable();
                    }
						
                    ImGui::Text("FOVSize:");
                    ImGui::SameLine();
                    ImGui::SliderFloat("##FOVSize", &Config.BulletTrack.FOVSize, 0.0f, 999.0f);
					
					ImGui::Text("TARGER: ");
                    ImGui::SameLine();
                    static const char *targets[] = {"Head", "Chest"};
                    ImGui::Combo("##Target", (int *) &Config.BulletTrack.Target, targets, 2, -1);
					
					ImGui::Text("TRIGGER: ");
                    ImGui::SameLine();
                    static const char *triggers[] = {"None", "Shooting", "Scoping", "Both (Shooting & Scoping)", "Any (Shooting / Scoping)"};
					ImGui::Combo("##Trigger", (int *)&Config.BulletTrack.Trigger, triggers, 5, -1);
						
    }
}

    ImGui::End();
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    return orig_eglSwapBuffers(dpy, surface);
}

int32_t (*orig_onInputEvent)(struct android_app *app, AInputEvent *inputEvent);
int32_t onInputEvent(struct android_app *app, AInputEvent *inputEvent) {
    if (initImGui) {
        ImGui_ImplAndroid_HandleInputEvent(inputEvent, {(float) screenWidth / (float) glWidth, (float) screenHeight / (float) glHeight});
    }
    return orig_onInputEvent(app, inputEvent);
}

#define SLEEP_TIME 1000LL / 60LL
[[noreturn]] void *maps_thread(void *) {
    while (true) {
        auto t1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        std::vector<sRegion> tmp;
        char line[512];
        FILE *f = fopen("/proc/self/maps", "r");
        if (f) {
            while (fgets(line, sizeof line, f)) {
                uintptr_t start, end;
                char tmpProt[16];
                if (sscanf(line, "%" PRIXPTR "-%" PRIXPTR " %16s %*s %*s %*s %*s", &start, &end, tmpProt) > 0) {
                    if (tmpProt[0] != 'r') {
                        tmp.push_back({start, end});
                    }
                }
            }
            fclose(f);
        }

        auto td = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - t1;
        std::this_thread::sleep_for(std::chrono::milliseconds(std::max(std::min(0LL, SLEEP_TIME - td), SLEEP_TIME)));
    }
}

void *main_thread(void *) {
    g_UE4 = Tools::GetBaseAddress("libUE4.so");
    while (!g_UE4) {
        g_UE4 = Tools::GetBaseAddress("libUE4.so");
        sleep(1);
    }
    while (!g_App) {
        g_App = *(android_app **) (g_UE4 + GNativeApp_Offset);
        sleep(1);
    }
    FName::GNames = GetGNames();
    while (!FName::GNames) {
        FName::GNames = GetGNames();
        sleep(1);
    }
    UObject::GUObjectArray = (FUObjectArray *) (g_UE4 + GUObject_Offset);

    orig_onInputEvent = decltype(orig_onInputEvent)(g_App->onInputEvent);
    g_App->onInputEvent = onInputEvent;

    plthook_t *plthook;
    if (plthook_open(&plthook, "libUE4.so") == 0) {
        plthook_replace(plthook, "eglSwapBuffers", (void *) _eglSwapBuffers, (void **) &orig_eglSwapBuffers);
        plthook_close(plthook);
    }
	Tools::Hook((void *) DobbySymbolResolver(OBFUSCATE("/system/lib/libEGL.so"), OBFUSCATE("eglSwapBuffers")), (void *) _eglSwapBuffers, (void **) &orig_eglSwapBuffers);

    pthread_t t;
    pthread_create(&t, 0, maps_thread, 0);
    items_data = json::parse(JSON_ITEMS);
     
    return 0;
}

__attribute__((constructor)) void _init() {
    pthread_t t;
    pthread_create(&t, 0, main_thread, 0);
}
