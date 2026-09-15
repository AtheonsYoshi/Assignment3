#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Haptics/HapticFeedbackEffect_Base.h"
#include "Kismet/GameplayStatics.h"
#include "Rhythm/RhythmTypes.h"

namespace RhythmHaptics
{
	inline void Play(const UObject* WorldContext, ERhythmHand Hand, UHapticFeedbackEffect_Base* Effect)
	{
		if (!Effect || Hand == ERhythmHand::Any)
		{
			return;
		}
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContext, 0))
		{
			PC->PlayHapticEffect(Effect, Hand == ERhythmHand::Left ? EControllerHand::Left : EControllerHand::Right);
		}
	}
}
