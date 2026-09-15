#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "RhythmTypes.generated.h"

/** What the player has to do with a note. */
UENUM(BlueprintType)
enum class ERhythmNoteType : uint8
{
	/** Cut with a blade (Beat Saber). */
	Slice,
	/** Shoot it (Pistol Whip). */
	Shoot,
	/** Energy wall — move your body so your head avoids it. */
	Dodge,
	/** Corrupted creature — slice/shoot it (Destroy) or hold a palm/shield to it (Purify). */
	Choice
};

UENUM(BlueprintType)
enum class ERhythmHand : uint8
{
	Any,
	Left,
	Right
};

UENUM(BlueprintType)
enum class ERhythmCutDirection : uint8
{
	Any,
	Up,
	Down,
	Left,
	Right
};

UENUM(BlueprintType)
enum class ERhythmHitGrade : uint8
{
	Perfect,
	Great,
	Good,
	Miss
};

UENUM(BlueprintType)
enum class ERhythmChoice : uint8
{
	None,
	Purify,
	Destroy
};

UENUM(BlueprintType)
enum class ERhythmEnding : uint8
{
	None,
	/** Mostly purified — rifts close, room fills with light, full music. */
	Harmony,
	/** Mostly destroyed — you survived, but the room is scarred and the music hollow. */
	Silence,
	/** Health reached zero — the rifts consume the room. */
	Overrun
};

/**
 * One row of a beatmap DataTable. Import from CSV with columns:
 * Name,Time,Type,Hand,Direction,LaneX,LaneY,ApproachYaw
 */
USTRUCT(BlueprintType)
struct FRhythmNoteRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Seconds from the start of the song when the note reaches the player. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	float Time = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	ERhythmNoteType Type = ERhythmNoteType::Slice;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	ERhythmHand Hand = ERhythmHand::Any;

	/** Required swing direction for Slice notes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	ERhythmCutDirection Direction = ERhythmCutDirection::Any;

	/** Horizontal offset at the hit point in cm (negative = player's left). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	float LaneX = 0.0f;

	/** Vertical offset at the hit point in cm, relative to the conductor's hit height. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	float LaneY = 0.0f;

	/** Direction the note comes from in degrees around the player (0 = in front, 90 = right, 180 = behind). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm")
	float ApproachYaw = 0.0f;
};
