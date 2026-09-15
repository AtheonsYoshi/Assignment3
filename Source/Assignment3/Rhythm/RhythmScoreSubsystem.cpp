#include "Rhythm/RhythmScoreSubsystem.h"

void URhythmScoreSubsystem::ResetRun()
{
	Score = 0;
	Combo = 0;
	MaxCombo = 0;
	Health = 1.0f;
	Hits = 0;
	Misses = 0;
	PerfectHits = 0;
	DodgeFails = 0;
	PurifyCount = 0;
	DestroyCount = 0;
	bOverrun = false;

	OnScoreChanged.Broadcast(Score, Combo);
	OnHealthChanged.Broadcast(Health);
}

void URhythmScoreSubsystem::RegisterHit(ERhythmHitGrade Grade, ERhythmNoteType NoteType, ERhythmChoice Choice)
{
	if (Grade == ERhythmHitGrade::Miss)
	{
		RegisterMiss(NoteType);
		return;
	}

	int32 BasePoints = 0;
	switch (Grade)
	{
	case ERhythmHitGrade::Perfect: BasePoints = 115; ++PerfectHits; break;
	case ERhythmHitGrade::Great:   BasePoints = 100; break;
	case ERhythmHitGrade::Good:    BasePoints = 70;  break;
	default: break;
	}

	++Hits;
	Score += BasePoints * GetMultiplier();
	++Combo;
	MaxCombo = FMath::Max(MaxCombo, Combo);

	if (Choice == ERhythmChoice::Purify)
	{
		++PurifyCount;
		OnChoiceMade.Broadcast(Choice);
	}
	else if (Choice == ERhythmChoice::Destroy)
	{
		++DestroyCount;
		OnChoiceMade.Broadcast(Choice);
	}

	ApplyHealthDelta(HitHeal);
	OnScoreChanged.Broadcast(Score, Combo);
}

void URhythmScoreSubsystem::RegisterMiss(ERhythmNoteType NoteType)
{
	++Misses;
	BreakCombo();
	ApplyHealthDelta(-MissDamage);
}

void URhythmScoreSubsystem::RegisterDodge(bool bSuccess)
{
	if (bSuccess)
	{
		++Combo;
		MaxCombo = FMath::Max(MaxCombo, Combo);
		OnScoreChanged.Broadcast(Score, Combo);
		return;
	}

	++DodgeFails;
	BreakCombo();
	ApplyHealthDelta(-DodgeFailDamage);
}

ERhythmEnding URhythmScoreSubsystem::EvaluateEnding() const
{
	if (bOverrun)
	{
		return ERhythmEnding::Overrun;
	}

	const int32 TotalChoices = PurifyCount + DestroyCount;
	if (TotalChoices > 0 && static_cast<float>(PurifyCount) / TotalChoices >= HarmonyPurifyRatio)
	{
		return ERhythmEnding::Harmony;
	}
	return ERhythmEnding::Silence;
}

float URhythmScoreSubsystem::GetMusicIntensity() const
{
	return FMath::Clamp(static_cast<float>(Combo) / FMath::Max(1, ComboForFullIntensity), 0.0f, 1.0f);
}

float URhythmScoreSubsystem::GetAccuracy() const
{
	const int32 Total = Hits + Misses;
	return Total > 0 ? static_cast<float>(Hits) / Total : 1.0f;
}

void URhythmScoreSubsystem::ApplyHealthDelta(float Delta)
{
	if (bOverrun || (bNoFailMode && Delta < 0.0f))
	{
		return;
	}

	Health = FMath::Clamp(Health + Delta, 0.0f, 1.0f);
	OnHealthChanged.Broadcast(Health);

	if (Health <= 0.0f)
	{
		bOverrun = true;
		OnOverrun.Broadcast();
	}
}

void URhythmScoreSubsystem::BreakCombo()
{
	Combo = 0;
	OnScoreChanged.Broadcast(Score, Combo);
}

int32 URhythmScoreSubsystem::GetMultiplier() const
{
	// Beat Saber style: x2 after 2, x4 after 6, x8 after 14 consecutive hits.
	if (Combo >= 14) return 8;
	if (Combo >= 6) return 4;
	if (Combo >= 2) return 2;
	return 1;
}
