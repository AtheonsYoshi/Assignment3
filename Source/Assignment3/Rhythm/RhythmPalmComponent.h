#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Rhythm/RhythmTypes.h"
#include "RhythmPalmComponent.generated.h"

class ARhythmNote;
class UHapticFeedbackEffect_Base;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRhythmPurified, ARhythmNote*, Note, ERhythmHitGrade, Grade);

/**
 * The "purify" hand. Attach to a motion controller and call SetPalmOpen from your grip input.
 * While open, touching a Choice note purifies it instead of destroying it.
 */
UCLASS(ClassGroup = (Rhythm), meta = (BlueprintSpawnableComponent))
class ASSIGNMENT3_API URhythmPalmComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	URhythmPalmComponent();

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Palm")
	void SetPalmOpen(bool bOpen);

	UFUNCTION(BlueprintPure, Category = "Rhythm|Palm")
	bool IsPalmOpen() const { return bPalmOpen; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Palm")
	ERhythmHand Hand = ERhythmHand::Left;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Palm")
	TObjectPtr<UHapticFeedbackEffect_Base> PurifyHaptic;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Palm")
	FOnRhythmPurified OnPurified;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void TryPurify(AActor* Actor);

	bool bPalmOpen = false;
};
