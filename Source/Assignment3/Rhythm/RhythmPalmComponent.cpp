#include "Rhythm/RhythmPalmComponent.h"

#include "Rhythm/RhythmHaptics.h"
#include "Rhythm/RhythmNote.h"

URhythmPalmComponent::URhythmPalmComponent()
{
	InitSphereRadius(12.0f);
	SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	SetGenerateOverlapEvents(true);
}

void URhythmPalmComponent::BeginPlay()
{
	Super::BeginPlay();
	OnComponentBeginOverlap.AddDynamic(this, &URhythmPalmComponent::HandleBeginOverlap);
}

void URhythmPalmComponent::SetPalmOpen(bool bOpen)
{
	if (bPalmOpen == bOpen)
	{
		return;
	}
	bPalmOpen = bOpen;

	// Opening the palm while already touching a creature should still count.
	if (bPalmOpen)
	{
		TArray<AActor*> Overlapping;
		GetOverlappingActors(Overlapping, ARhythmNote::StaticClass());
		for (AActor* Actor : Overlapping)
		{
			TryPurify(Actor);
		}
	}
}

void URhythmPalmComponent::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bPalmOpen)
	{
		TryPurify(OtherActor);
	}
}

void URhythmPalmComponent::TryPurify(AActor* Actor)
{
	ARhythmNote* Note = Cast<ARhythmNote>(Actor);
	if (!Note)
	{
		return;
	}

	ERhythmHitGrade Grade;
	if (Note->TryPurify(Hand, Grade))
	{
		RhythmHaptics::Play(this, Hand, PurifyHaptic);
		OnPurified.Broadcast(Note, Grade);
	}
}
