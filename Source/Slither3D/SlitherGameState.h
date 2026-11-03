
#pragma once

#include "GameFramework/GameState.h"

#include "SlitherGameState.generated.h"


UCLASS()
class ASlitherGameState : public AGameState
{
    GENERATED_BODY()

private:

    UPROPERTY(Replicated)
    bool bIsWorldReady = false;

private:

    void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:

    void MarkWorldReady();

    bool IsWorldReady() const;
};