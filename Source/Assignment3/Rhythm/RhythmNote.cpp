#include "Rhythm/RhythmNote.h"

#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Rhythm/RhythmConductor.h"
#include "Rhythm/RhythmScoreSubsystem.h"

ARhythmNote::ARhythmNote()
{
	// The conductor moves notes from its own tick, so notes don't need to tick.
	PrimaryActorTick.bCanEverTick = false;

	HitVolume = CreateDefaultSubobject<USphereComponent>(TEXT("HitVolume"));
	HitVolume->InitSphereRadius(18.0f);
	HitVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	HitVolume->SetGenerateOverlapEvents(true);
	RootComponent = HitVolume;
}

void ARhythmNote::Activate(ARhythmConductor* InConductor, const FRhythmNoteRow& InData, const FVector& InSpawnLocation,
	const FVector& InHitLocation, const FVector& InForward, const FVector& InRight, float InTravelTime)
{
	GetWorldTimerManager().ClearTimer(DespawnTimer);

	Conductor = InConductor;
	NoteData = InData;
	SpawnLocation = InSpawnLocation;
	HitLocation = InHitLocation;
	ApproachForward = InForward;
	NoteRight = InRight;
	TravelTime = FMath::Max(InTravelTime, KINDA_SMALL_NUMBER);
	bActive = true;
	bResolved = false;

	// Face the player.
	SetActorLocationAndRotation(SpawnLocation, (-ApproachForward).Rotation());
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	OnNoteActivated();
}

void ARhythmNote::UpdateNote(double SongTime)
{
	if (!bActive)
	{
		return;
	}
	LastSongTime = SongTime;

	// Position is derived from song time every frame, so notes can never drift out of sync with the music.
	const double Alpha = 1.0 - (NoteData.Time - SongTime) / TravelTime;
	SetActorLocation(SpawnLocation + (HitLocation - SpawnLocation) * Alpha);

	// Setting the location can trigger an overlap that resolves (and pools) this note.
	if (!bActive || bResolved)
	{
		return;
	}

	if (NoteData.Type == ERhythmNoteType::Dodge)
	{
		const ARhythmConductor* ConductorPtr = Conductor.Get();
		if (!ConductorPtr)
		{
			return;
		}

		const FVector ToNote = GetActorLocation() - ConductorPtr->GetHeadLocation();
		const float Along = FVector::DotProduct(ToNote, ApproachForward);
		if (FMath::Abs(Along) <= DodgeHalfExtents.X + HeadRadius)
		{
			const float Side = FMath::Abs(FVector::DotProduct(ToNote, NoteRight));
			const float Height = FMath::Abs(ToNote.Z);
			if (Side <= DodgeHalfExtents.Y + HeadRadius && Height <= DodgeHalfExtents.Z + HeadRadius)
			{
				ResolveDodge(false);
			}
		}
		else if (Along < 0.0f)
		{
			ResolveDodge(true);
		}
		return;
	}

	if (SongTime - NoteData.Time > LateWindow)
	{
		ResolveMiss(false);
	}
}

bool ARhythmNote::TrySlice(ERhythmHand Hand, FVector BladeVelocity, ERhythmHitGrade& OutGrade)
{
	OutGrade = ERhythmHitGrade::Miss;
	const bool bSliceable = NoteData.Type == ERhythmNoteType::Slice || NoteData.Type == ERhythmNoteType::Choice;
	if (!bSliceable || !CanBeHitNow() || BladeVelocity.Size() < MinSwingSpeed)
	{
		return false;
	}

	if (!MatchesHand(Hand) || !MatchesDirection(BladeVelocity))
	{
		ResolveMiss(true);
		return true;
	}

	OutGrade = GradeCurrentTiming();
	ResolveHit(OutGrade, NoteData.Type == ERhythmNoteType::Choice ? ERhythmChoice::Destroy : ERhythmChoice::None);
	return true;
}

bool ARhythmNote::TryShoot(ERhythmHand Hand, ERhythmHitGrade& OutGrade)
{
	OutGrade = ERhythmHitGrade::Miss;
	const bool bShootable = NoteData.Type == ERhythmNoteType::Shoot || NoteData.Type == ERhythmNoteType::Choice;
	if (!bShootable || !CanBeHitNow())
	{
		return false;
	}

	if (!MatchesHand(Hand))
	{
		ResolveMiss(true);
		return true;
	}

	OutGrade = GradeCurrentTiming();
	ResolveHit(OutGrade, NoteData.Type == ERhythmNoteType::Choice ? ERhythmChoice::Destroy : ERhythmChoice::None);
	return true;
}

bool ARhythmNote::TryPurify(ERhythmHand Hand, ERhythmHitGrade& OutGrade)
{
	OutGrade = ERhythmHitGrade::Miss;
	if (NoteData.Type != ERhythmNoteType::Choice || !CanBeHitNow())
	{
		return false;
	}

	OutGrade = GradeCurrentTiming();
	ResolveHit(OutGrade, ERhythmChoice::Purify);
	return true;
}

float ARhythmNote::GetTimeUntilHit() const
{
	return static_cast<float>(NoteData.Time - GetLiveSongTime());
}

bool ARhythmNote::CanBeHitNow() const
{
	if (!bActive || bResolved)
	{
		return false;
	}
	const double Error = GetLiveSongTime() - NoteData.Time;
	return Error >= -EarlyWindow && Error <= LateWindow;
}

bool ARhythmNote::MatchesHand(ERhythmHand Hand) const
{
	return NoteData.Hand == ERhythmHand::Any || Hand == ERhythmHand::Any || Hand == NoteData.Hand;
}

bool ARhythmNote::MatchesDirection(const FVector& Velocity) const
{
	FVector Required;
	switch (NoteData.Direction)
	{
	case ERhythmCutDirection::Up:    Required = FVector::UpVector; break;
	case ERhythmCutDirection::Down:  Required = -FVector::UpVector; break;
	case ERhythmCutDirection::Left:  Required = -NoteRight; break;
	case ERhythmCutDirection::Right: Required = NoteRight; break;
	default: return true;
	}

	// Ignore the part of the swing that goes towards/away from the note.
	const FVector Planar = Velocity - ApproachForward * FVector::DotProduct(Velocity, ApproachForward);
	return FVector::DotProduct(Planar.GetSafeNormal(), Required) >= FMath::Cos(FMath::DegreesToRadians(MaxSwingAngle));
}

ERhythmHitGrade ARhythmNote::GradeCurrentTiming() const
{
	const double AbsError = FMath::Abs(GetLiveSongTime() - NoteData.Time);
	if (AbsError <= PerfectWindow) return ERhythmHitGrade::Perfect;
	if (AbsError <= GreatWindow) return ERhythmHitGrade::Great;
	return ERhythmHitGrade::Good;
}

double ARhythmNote::GetLiveSongTime() const
{
	const ARhythmConductor* ConductorPtr = Conductor.Get();
	return ConductorPtr ? ConductorPtr->GetSongTime() : LastSongTime;
}

void ARhythmNote::ResolveHit(ERhythmHitGrade Grade, ERhythmChoice Choice)
{
	bResolved = true;
	SetActorEnableCollision(false);

	if (URhythmScoreSubsystem* Score = GetWorld()->GetSubsystem<URhythmScoreSubsystem>())
	{
		Score->RegisterHit(Grade, NoteData.Type, Choice);
	}
	OnNoteHit(Grade, Choice);
	if (ARhythmConductor* ConductorPtr = Conductor.Get())
	{
		ConductorPtr->NotifyNoteResolved(this, Grade, Choice);
	}
	FinishResolution();
}

void ARhythmNote::ResolveMiss(bool bBadCut)
{
	bResolved = true;
	SetActorEnableCollision(false);

	if (URhythmScoreSubsystem* Score = GetWorld()->GetSubsystem<URhythmScoreSubsystem>())
	{
		Score->RegisterMiss(NoteData.Type);
	}
	OnNoteMissed(bBadCut);
	if (ARhythmConductor* ConductorPtr = Conductor.Get())
	{
		ConductorPtr->NotifyNoteResolved(this, ERhythmHitGrade::Miss, ERhythmChoice::None);
	}
	FinishResolution();
}

void ARhythmNote::ResolveDodge(bool bSuccess)
{
	bResolved = true;

	if (URhythmScoreSubsystem* Score = GetWorld()->GetSubsystem<URhythmScoreSubsystem>())
	{
		Score->RegisterDodge(bSuccess);
	}
	OnDodgeResolved(bSuccess);
	if (ARhythmConductor* ConductorPtr = Conductor.Get())
	{
		ConductorPtr->NotifyNoteResolved(this, bSuccess ? ERhythmHitGrade::Perfect : ERhythmHitGrade::Miss, ERhythmChoice::None);
	}

	// Let a successful wall fly on past the player before it disappears.
	if (bSuccess && DespawnDelay <= 0.0f)
	{
		GetWorldTimerManager().SetTimer(DespawnTimer, this, &ARhythmNote::Deactivate, 0.5f, false);
		return;
	}
	FinishResolution();
}

void ARhythmNote::FinishResolution()
{
	if (DespawnDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(DespawnTimer, this, &ARhythmNote::Deactivate, DespawnDelay, false);
	}
	else
	{
		Deactivate();
	}
}

void ARhythmNote::Deactivate()
{
	if (!bActive)
	{
		return;
	}

	SetPooled();
	if (ARhythmConductor* ConductorPtr = Conductor.Get())
	{
		ConductorPtr->ReleaseNote(this);
	}
}

void ARhythmNote::SetPooled()
{
	GetWorldTimerManager().ClearTimer(DespawnTimer);
	bActive = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}
