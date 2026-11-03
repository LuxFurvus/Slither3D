
#include "SlitherGameState.h"

#include "Net/UnrealNetwork.h"


void ASlitherGameState::MarkWorldReady()
{
    if (const bool HasServerAuthority
            = HasAuthority();
        !HasServerAuthority)
    {
        return;
    }

    bIsWorldReady = true;
}

void ASlitherGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ASlitherGameState, bIsWorldReady);
}

bool ASlitherGameState::IsWorldReady() const
{
    return bIsWorldReady;
}
