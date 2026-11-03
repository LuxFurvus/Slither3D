
#pragma once

#include "CoreMinimal.h"
#include "Settings/SlitherGameSettingsTypes.h"

#include "SlitherPlayerLifeTypes.generated.h"


UENUM()
enum class ESlitherLifeState : uint8
{
    WaitingForSpawn,
    ReadyToStart,
    Alive,
    Dead
};


USTRUCT()
struct FSlitherPlayerLifeData
{
    GENERATED_BODY()

public:

    UPROPERTY()
    uint32 LifeId = 0;

    UPROPERTY()
    int32 Mass = FSlitherSettingsGrowth::InitialMass;

    UPROPERTY()
    int32 Kills = 0;

    UPROPERTY()
    ESlitherLifeState State = ESlitherLifeState::ReadyToStart;

    UPROPERTY()
    double SpawnServerTime = -1.0;

    UPROPERTY()
    double DeathServerTime = -1.0;

    UPROPERTY()
    double RespawnAvailableServerTime = -1.0;
};
