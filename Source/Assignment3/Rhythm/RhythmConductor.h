#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rhythm/RhythmTypes.h"
#include "RhythmConductor.generated.h"

class ARhythmNote;
class UAudioComponent;
class UDataTable;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRhythmSongEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmBeat, int32, BeatIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRhythmNoteSpawned, ARhythmNote*, Note);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnRhythmNoteResolved, ARhythmNote*, Note, ERhythmHitGrade, Grade, ERhythmChoice, Choice);

USTRUCT()
struct FRhythmNotePool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<ARhythmNote>> Notes;
};

/**
 * Plays the song, keeps the song clock, and spawns pooled notes from a beatmap DataTable so
 * they arrive at the player exactly on their beat. Place one in the level.
 */
UCLASS()
class ASSIGNMENT3_API ARhythmConductor : public AActor
{
	GENERATED_BODY()

public:
	ARhythmConductor();

	virtual void Tick(float DeltaSeconds) override;

	/** Starts the song. StartAtTime lets you jump into the middle of a song while testing. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Conductor")
	void StartSong(float StartAtTime = 0.0f);

	/** Stops the music and clears all notes without firing OnSongFinished. */
	UFUNCTION(BlueprintCallable, Category = "Rhythm|Conductor")
	void StopSong();

	/** Seconds since the start of the song (negative during the lead-in). */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Conductor")
	double GetSongTime() const;

	/** Current position in beats — use the fractional part to pulse visuals on the beat. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Conductor")
	float GetSongBeat() const;

	UFUNCTION(BlueprintPure, Category = "Rhythm|Conductor")
	bool IsSongPlaying() const { return bPlaying; }

	/** Player head position this frame (used for dodge checks). */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Conductor")
	FVector GetHeadLocation() const { return HeadLocation; }

	/** Where the player was standing/facing when the song started. Notes are laid out relative to this. */
	UFUNCTION(BlueprintPure, Category = "Rhythm|Conductor")
	FTransform GetAnchorTransform() const { return Anchor; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Song")
	TObjectPtr<USoundBase> Song;

	/** DataTable with row type RhythmNoteRow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Song", meta = (RequiredAssetDataTags = "RowStructure=/Script/Assignment3.RhythmNoteRow"))
	TObjectPtr<UDataTable> Beatmap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Song", meta = (ClampMin = "1"))
	float BPM = 120.0f;

	/** Silence before the music starts so the first notes have time to fly in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Song", meta = (ClampMin = "0"))
	float LeadInTime = 3.0f;

	/** Positive values make notes arrive later. Tune on the headset until hits feel on the beat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Song")
	float AudioLatencyOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Song")
	bool bResetScoreOnStart = true;

	/** Blueprint note class to use for each note type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes")
	TMap<ERhythmNoteType, TSubclassOf<ARhythmNote>> NoteClasses;

	/** Seconds a note is visible before it reaches the player. Lower = harder. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes", meta = (ClampMin = "0.2"))
	float TravelTime = 2.0f;

	/** How far away (cm) notes appear. In MR this can be "through" your real walls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes")
	float SpawnDistance = 900.0f;

	/** Distance in front of the player (cm) where notes should be hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes")
	float HitDistance = 70.0f;

	/** Height of the hit point relative to the player's head (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes")
	float HitHeightOffset = -30.0f;

	/** Lay notes out around the player's head when the song starts. Turn off to use this actor's transform instead. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes")
	bool bAnchorToPlayerView = true;

	/** Notes of each class spawned up front so nothing spawns mid-song (avoids hitches on Quest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rhythm|Notes", meta = (ClampMin = "0"))
	int32 PrewarmPerClass = 8;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rhythm|Song")
	TObjectPtr<UAudioComponent> MusicComponent;

	/** Fires when the music actually starts (after the lead-in). */
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Conductor")
	FOnRhythmSongEvent OnSongStarted;

	/** Fires once the music has ended and every note is resolved — start your ending sequence here. */
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Conductor")
	FOnRhythmSongEvent OnSongFinished;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Conductor")
	FOnRhythmBeat OnBeat;

	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Conductor")
	FOnRhythmNoteSpawned OnNoteSpawned;

	/** Fires for every hit, miss and dodge — use the note's location for spatial consequences (cracks, light). */
	UPROPERTY(BlueprintAssignable, Category = "Rhythm|Conductor")
	FOnRhythmNoteResolved OnNoteResolved;

	// --- Called by ARhythmNote ---
	void ReleaseNote(ARhythmNote* Note);
	void NotifyNoteResolved(ARhythmNote* Note, ERhythmHitGrade Grade, ERhythmChoice Choice);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleMusicFinished();

	double GetRawSongTime() const;
	void UpdateHeadLocation();
	void CaptureAnchor();
	void PrewarmPools();
	void SpawnNote(const FRhythmNoteRow& Row);
	ARhythmNote* AcquireNote(TSubclassOf<ARhythmNote> NoteClass);
	ARhythmNote* CreatePooledNote(TSubclassOf<ARhythmNote> NoteClass);
	void ClearActiveNotes();

	UPROPERTY()
	TArray<TObjectPtr<ARhythmNote>> ActiveNotes;

	UPROPERTY()
	TMap<TSubclassOf<ARhythmNote>, FRhythmNotePool> Pools;

	TArray<FRhythmNoteRow> SortedNotes;
	FTransform Anchor;
	FVector HeadLocation = FVector::ZeroVector;
	double SongStartAudioTime = 0.0;
	double StoppedSongTime = 0.0;
	float StartAtTime = 0.0f;
	float LastNoteTime = 0.0f;
	int32 NextNoteIndex = 0;
	int32 LastBeatIndex = -1;
	bool bPlaying = false;
	bool bMusicStarted = false;
	bool bMusicFinished = false;
};
