#include "Rhythm/RhythmBladeComponent.h"

#include "Rhythm/RhythmHaptics.h"
#include "Rhythm/RhythmNote.h"

URhythmBladeComponent::URhythmBladeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	InitCapsuleSize(3.0f, 50.0f);
	SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetGenerateOverlapEvents(true);
}

void URhythmBladeComponent::BeginPlay()
{
	Super::BeginPlay();
	OnComponentBeginOverlap.AddDynamic(this, &URhythmBladeComponent::HandleBeginOverlap);
}

void URhythmBladeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FVector Tip = GetTipLocation();
	if (bHasLastTip && DeltaTime > KINDA_SMALL_NUMBER)
	{
		const FVector Instant = (Tip - LastTipLocation) / DeltaTime;
		TipVelocity = FMath::Lerp(TipVelocity, Instant, 0.6f);
	}
	LastTipLocation = Tip;
	bHasLastTip = true;
}

FVector URhythmBladeComponent::GetTipLocation() const
{
	return GetComponentLocation() + GetUpVector() * GetScaledCapsuleHalfHeight();
}

void URhythmBladeComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ARhythmNote* Note = Cast<ARhythmNote>(OtherActor);
	if (!Note)
	{
		return;
	}

	ERhythmHitGrade Grade;
	if (!Note->TrySlice(Hand, TipVelocity, Grade))
	{
		return;
	}

	RhythmHaptics::Play(this, Hand, Grade == ERhythmHitGrade::Miss ? BadCutHaptic : HitHaptic);
	OnBladeHit.Broadcast(Note, Grade);
}
