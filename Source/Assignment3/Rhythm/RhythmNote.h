#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rhythm/RhythmTypes.h"
#include "RhythmNote.generated.h"

class ARhythmConductor;
class USphereComponent;

/**
 * Base class for everything that flies at the player on the beat.
 * Make a Blueprint child per note type (orb, drone, wall, corrupted creature), add a mesh,
 * and implement the On... events for VFX/SFX. Movement, timing and scoring are handled here.
 * Notes are pooled by ARhythmConductor — don't spawn or destroy them yourself.
 */
UCLASS(Blueprintable)
class ASSIGNMENT3_API ARhythmNote : public AActor
{
	GENERATED_BODY()

public:
	ARhythmNote();

	/** Called by a blade/weapon. Returns true if the note was resolved (hit or bad cut). */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Note")
	bool TrySlice(ERhythmHand Hand, FVector BladeVelocity, ERhythmHitGrade& OutGrade);

	/** Call from your projectile/trace when it hits this note. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Note")
	bool TryShoot(ERhythmHand Hand, ERhythmHitGrade& OutGrade);

	/** Open palm / shield on a Choice note. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Note")
	bool TryPurify(ERhythmHand Hand, ERhythmHitGrade& OutGrade);

	/** Seconds until this note reaches the hit point (negative once it has passed). */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Note")
	float GetTimeUntilHit() const;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Note")
	bool IsResolved() const { return bResolved; }

	UFUNCTION(BlueprintPure, Category = "Rhythm|Note")
	bool IsNoteActive() const { return bActive; }

	/** Direction the note travels from (points from the player out towards the spawn point). */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Note")
	FVector GetApproachDirection() const { return ApproachForward; }

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Note")
	FRhythmNoteRow NoteData;

	/** Overlap volume used by blades, palms and projectiles. Resize it in your Blueprint. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rhythm|Note")
	TObjectPtr<USphereComponent> HitVolume;

	/** How early (seconds before the hit time) the note can be hit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Timing")
	float EarlyWindow = 0.35f;

	/** How late (seconds after the hit time) the note can still be hit before it counts as a miss. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Timing")
	float LateWindow = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Timing")
	float PerfectWindow = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Timing")
	float GreatWindow = 0.1f;

	/** Blade speed (cm/s) needed for a swing to count as a cut. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Slice")
	float MinSwingSpeed = 150.0f;

	/** How far (degrees) a swing may deviate from the required direction. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Slice")
	float MaxSwingAngle = 60.0f;

	/** Dodge walls: half size in cm (X = depth along travel, Y = width, Z = height). Match your wall mesh. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Dodge")
	FVector DodgeHalfExtents = FVector(10.0f, 40.0f, 40.0f);

	/** Treat the player's head as a sphere of this radius when checking dodges. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Dodge")
	float HeadRadius = 12.0f;

	/** Keep the note visible (and moving) this long after it resolves, e.g. for a dissolve. 0 = hide instantly. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Rhythm|Note")
	float DespawnDelay = 0.0f;

	// --- Called by ARhythmConductor ---
	void Activate(ARhythmConductor* InConductor, const FRhythmNoteRow& InData, const FVector& InSpawnLocation,
		const FVector& InHitLocation, const FVector& InForward, const FVector& InRight, float InTravelTime);
	void UpdateNote(double SongTime);
	void SetPooled();

protected:
	/** The note has just been pulled from the pool — set colour/arrow from NoteData here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rhythm|Note")
	void OnNoteActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Rhythm|Note")
	void OnNoteHit(ERhythmHitGrade Grade, ERhythmChoice Choice);

	/** bBadCut = touched with the wrong hand or wrong direction; false = it got past the player. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rhythm|Note")
	void OnNoteMissed(bool bBadCut);

	UFUNCTION(BlueprintImplementableEvent, Category = "Rhythm|Note")
	void OnDodgeResolved(bool bSuccess);

private:
	bool CanBeHitNow() const;
	bool MatchesHand(ERhythmHand Hand) const;
	bool MatchesDirection(const FVector& Velocity) const;
	ERhythmHitGrade GradeCurrentTiming() const;
	double GetLiveSongTime() const;

	void ResolveHit(ERhythmHitGrade Grade, ERhythmChoice Choice);
	void ResolveMiss(bool bBadCut);
	void ResolveDodge(bool bSuccess);
	void FinishResolution();
	void Deactivate();

	TWeakObjectPtr<ARhythmConductor> Conductor;
	FTimerHandle DespawnTimer;
	FVector SpawnLocation = FVector::ZeroVector;
	FVector HitLocation = FVector::ZeroVector;
	FVector ApproachForward = FVector::ForwardVector;
	FVector NoteRight = FVector::RightVector;
	float TravelTime = 2.0f;
	double LastSongTime = 0.0;
	bool bActive = false;
	bool bResolved = false;
};
