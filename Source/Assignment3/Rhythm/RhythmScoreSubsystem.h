#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Rhythm/RhythmTypes.h"
#include "RhythmScoreSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRhythmScoreChanged, int32, Score, int32, Combo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmHealthChanged, float, Health);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmChoiceMade, ERhythmChoice, Choice);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRhythmOverrun);

/**
 * Tracks everything the player's performance and choices change: score, combo, health,
 * purify/destroy choices, and which ending they earned.
 * In Blueprint: "Get RhythmScoreSubsystem" from anywhere.
 */
UCLASS()
class ASSIGNMENT3_API URhythmScoreSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Score")
	void ResetRun();

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Score")
	void RegisterHit(ERhythmHitGrade Grade, ERhythmNoteType NoteType, ERhythmChoice Choice);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Score")
	void RegisterMiss(ERhythmNoteType NoteType);

	UFUNCTION(BlueprintCallable, Category = "Rhythm|Score")
	void RegisterDodge(bool bSuccess);

	/** Decides the ending from health and the purify/destroy balance. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Score")
	ERhythmEnding EvaluateEnding() const;

	/** 0..1, rises with combo and drops on misses. Drive music stem volumes / room glow with this. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Score")
	float GetMusicIntensity() const;

	/** Fraction of scorable notes hit (0..1). */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Score")
	float GetAccuracy() const;

	/** When true (e.g. during onboarding) health never drops and the player can't be overrun. */
	UPROPERTY(BlueprintReadWrite, Category = "Rhythm|Score")
	bool bNoFailMode = false;

	UPROPERTY(BlueprintReadWrite, Category = "Rhythm|Tuning")
	float MissDamage = 0.08f;

	UPROPERTY(BlueprintReadWrite, Category = "Rhythm|Tuning")
	float DodgeFailDamage = 0.15f;

	UPROPERTY(BlueprintReadWrite, Category = "Rhythm|Tuning")
	float HitHeal = 0.02f;

	/** Purify / (Purify + Destroy) at or above this earns the Harmony ending. */
	UPROPERTY(BlueprintReadWrite, Category = "Rhythm|Tuning")
	float HarmonyPurifyRatio = 0.5f;

	/** Combo needed for full music intensity. */
	UPROPERTY(BlueprintReadWrite, Category = "Rhythm|Tuning")
	int32 ComboForFullIntensity = 16;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 Combo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 MaxCombo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	float Health = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 Hits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 Misses = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 PerfectHits = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	int32 DodgeFails = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Choices")
	int32 PurifyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Choices")
	int32 DestroyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Rhythm|Score")
	bool bOverrun = false;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Score")
	FOnRhythmScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Score")
	FOnRhythmHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Choices")
	FOnRhythmChoiceMade OnChoiceMade;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Score")
	FOnRhythmOverrun OnOverrun;

private:
	void ApplyHealthDelta(float Delta);
	void BreakCombo();
	int32 GetMultiplier() const;
};
