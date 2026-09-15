#pragma once

#include "CoreMinimal.h"
#include "Components/CapsuleComponent.h"
#include "Rhythm/RhythmTypes.h"
#include "RhythmBladeComponent.generated.h"

class ARhythmNote;
class UHapticFeedbackEffect_Base;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRhythmBladeHit, ARhythmNote*, Note, ERhythmHitGrade, Grade);

/**
 * A saber. Attach to a motion controller and rotate it so the capsule points along the controller
 * (relative pitch -90). Tracks tip velocity and slices any ARhythmNote it touches.
 */
UCLASS(ClassGroup = (Rhythm), meta = (BlueprintSpawnableComponent))
class ASSIGNMENT3_API URhythmBladeComponent : public UCapsuleComponent
{
	GENERATED_BODY()

public:
	URhythmBladeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Smoothed velocity of the blade tip in cm/s. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Blade")
	FVector GetTipVelocity() const { return TipVelocity; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Blade")
	ERhythmHand Hand = ERhythmHand::Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Blade")
	TObjectPtr<UHapticFeedbackEffect_Base> HitHaptic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Blade")
	TObjectPtr<UHapticFeedbackEffect_Base> BadCutHaptic;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Blade")
	FOnRhythmBladeHit OnBladeHit;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	FVector GetTipLocation() const;

	FVector LastTipLocation = FVector::ZeroVector;
	FVector TipVelocity = FVector::ZeroVector;
	bool bHasLastTip = false;
};
