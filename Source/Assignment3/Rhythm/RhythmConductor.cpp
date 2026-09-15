#include "Rhythm/RhythmConductor.h"

#include "Assignment3.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Rhythm/RhythmNote.h"
#include "Rhythm/RhythmScoreSubsystem.h"
#include "Sound/SoundBase.h"

ARhythmConductor::ARhythmConductor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	MusicComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("Music"));
	MusicComponent->SetupAttachment(RootComponent);
	MusicComponent->bAutoActivate = false;
	MusicComponent->bAllowSpatialization = false;
}

void ARhythmConductor::BeginPlay()
{
	Super::BeginPlay();

	MusicComponent->OnAudioFinished.AddDynamic(this, &ARhythmConductor::HandleMusicFinished);
	PrewarmPools();
}

void ARhythmConductor::StartSong(float InStartAtTime)
{
	StopSong();

	SortedNotes.Reset();
	if (Beatmap)
	{
		if (Beatmap->GetRowStruct() != FRhythmNoteRow::StaticStruct())
		{
			UE_LOG(LogRhythm, Error, TEXT("%s: Beatmap '%s' must use row type RhythmNoteRow."), *GetName(), *Beatmap->GetName());
			return;
		}
		Beatmap->ForeachRow<FRhythmNoteRow>(TEXT("ARhythmConductor::StartSong"), [this, InStartAtTime](const FName&, const FRhythmNoteRow& Row)
		{
			if (Row.Time >= InStartAtTime)
			{
				SortedNotes.Add(Row);
			}
		});
		SortedNotes.Sort([](const FRhythmNoteRow& A, const FRhythmNoteRow& B) { return A.Time < B.Time; });
	}
	else
	{
		UE_LOG(LogRhythm, Warning, TEXT("%s: No beatmap assigned — only music will play."), *GetName());
	}

	if (bResetScoreOnStart)
	{
		if (URhythmScoreSubsystem* Score = GetWorld()->GetSubsystem<URhythmScoreSubsystem>())
		{
			Score->ResetRun();
		}
	}

	StartAtTime = InStartAtTime;
	LastNoteTime = SortedNotes.Num() > 0 ? SortedNotes.Last().Time : InStartAtTime;
	NextNoteIndex = 0;
	LastBeatIndex = FMath::FloorToInt32(InStartAtTime * BPM / 60.0f) - 1;
	bMusicStarted = false;
	bMusicFinished = false;

	// Song time starts at (StartAtTime - LeadInTime) and counts up to StartAtTime, where the music begins.
	SongStartAudioTime = GetWorld()->GetAudioTimeSeconds() + LeadInTime - InStartAtTime;
	UpdateHeadLocation();
	CaptureAnchor();
	bPlaying = true;
}

void ARhythmConductor::StopSong()
{
	if (!bPlaying)
	{
		return;
	}

	StoppedSongTime = GetSongTime();
	bPlaying = false;
	MusicComponent->Stop();
	ClearActiveNotes();
}

double ARhythmConductor::GetRawSongTime() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetAudioTimeSeconds() - SongStartAudioTime : 0.0;
}

double ARhythmConductor::GetSongTime() const
{
	return bPlaying ? GetRawSongTime() - AudioLatencyOffset : StoppedSongTime;
}

float ARhythmConductor::GetSongBeat() const
{
	return static_cast<float>(GetSongTime() * BPM / 60.0);
}

void ARhythmConductor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPlaying)
	{
		return;
	}

	UpdateHeadLocation();

	if (!bMusicStarted && GetRawSongTime() >= StartAtTime)
	{
		bMusicStarted = true;
		if (Song)
		{
			MusicComponent->SetSound(Song);
			MusicComponent->Play(StartAtTime);
		}
		else
		{
			bMusicFinished = true;
		}
		// Re-base on the frame the music actually started so the clock matches the audio.
		SongStartAudioTime = GetWorld()->GetAudioTimeSeconds() - StartAtTime;
		OnSongStarted.Broadcast();
	}

	const double SongTime = GetSongTime();

	while (NextNoteIndex < SortedNotes.Num() && SortedNotes[NextNoteIndex].Time - TravelTime <= SongTime)
	{
		SpawnNote(SortedNotes[NextNoteIndex]);
		++NextNoteIndex;
	}

	// Copy first: updating a note can resolve it and remove it from ActiveNotes.
	TArray<ARhythmNote*, TInlineAllocator<32>> NotesToUpdate(ActiveNotes);
	for (ARhythmNote* Note : NotesToUpdate)
	{
		if (IsValid(Note))
		{
			Note->UpdateNote(SongTime);
		}
	}

	if (SongTime >= 0.0)
	{
		const int32 BeatIndex = FMath::FloorToInt32(SongTime * BPM / 60.0);
		if (BeatIndex > LastBeatIndex)
		{
			LastBeatIndex = BeatIndex;
			OnBeat.Broadcast(BeatIndex);
		}
	}

	const bool bMusicDone = bMusicFinished && bMusicStarted;
	const bool bNotesDone = NextNoteIndex >= SortedNotes.Num() && ActiveNotes.Num() == 0;
	const bool bSilentSongDone = !Song && SongTime > LastNoteTime + 1.0;
	if (bNotesDone && bMusicDone && (Song || bSilentSongDone))
	{
		StoppedSongTime = SongTime;
		bPlaying = false;
		UE_LOG(LogRhythm, Log, TEXT("%s: Song finished."), *GetName());
		OnSongFinished.Broadcast();
	}
}

void ARhythmConductor::HandleMusicFinished()
{
	if (bPlaying && bMusicStarted)
	{
		bMusicFinished = true;
	}
}

void ARhythmConductor::UpdateHeadLocation()
{
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		HeadLocation = Camera->GetCameraLocation();
	}
	else
	{
		HeadLocation = GetActorLocation();
	}
}

void ARhythmConductor::CaptureAnchor()
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (bAnchorToPlayerView && Camera)
	{
		// Yaw only, so looking up/down at the start doesn't tilt the whole song.
		Anchor = FTransform(FRotator(0.0f, Camera->GetCameraRotation().Yaw, 0.0f), Camera->GetCameraLocation());
	}
	else
	{
		Anchor = FTransform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), GetActorLocation());
	}
}

void ARhythmConductor::PrewarmPools()
{
	for (const TPair<ERhythmNoteType, TSubclassOf<ARhythmNote>>& Pair : NoteClasses)
	{
		if (!Pair.Value)
		{
			continue;
		}
		FRhythmNotePool& Pool = Pools.FindOrAdd(Pair.Value);
		while (Pool.Notes.Num() < PrewarmPerClass)
		{
			if (ARhythmNote* Note = CreatePooledNote(Pair.Value))
			{
				Pool.Notes.Add(Note);
			}
			else
			{
				break;
			}
		}
	}
}

void ARhythmConductor::SpawnNote(const FRhythmNoteRow& Row)
{
	const TSubclassOf<ARhythmNote>* NoteClass = NoteClasses.Find(Row.Type);
	if (!NoteClass || !*NoteClass)
	{
		UE_LOG(LogRhythm, Warning, TEXT("%s: No note class set for type %s."), *GetName(), *UEnum::GetValueAsString(Row.Type));
		return;
	}

	ARhythmNote* Note = AcquireNote(*NoteClass);
	if (!Note)
	{
		return;
	}

	const FRotator ApproachRotation(0.0f, Anchor.Rotator().Yaw + Row.ApproachYaw, 0.0f);
	const FVector Forward = ApproachRotation.Vector();
	const FVector Right = FRotationMatrix(ApproachRotation).GetScaledAxis(EAxis::Y);

	const FVector HitLocation = Anchor.GetLocation()
		+ Forward * HitDistance
		+ Right * Row.LaneX
		+ FVector::UpVector * (HitHeightOffset + Row.LaneY);
	const FVector SpawnLocation = HitLocation + Forward * SpawnDistance;

	ActiveNotes.Add(Note);
	Note->Activate(this, Row, SpawnLocation, HitLocation, Forward, Right, TravelTime);
	OnNoteSpawned.Broadcast(Note);
}

ARhythmNote* ARhythmConductor::AcquireNote(TSubclassOf<ARhythmNote> NoteClass)
{
	FRhythmNotePool& Pool = Pools.FindOrAdd(NoteClass);
	while (Pool.Notes.Num() > 0)
	{
		ARhythmNote* Note = Pool.Notes.Pop(EAllowShrinking::No);
		if (IsValid(Note))
		{
			return Note;
		}
	}

	UE_LOG(LogRhythm, Verbose, TEXT("%s: Pool for %s ran dry, spawning another note. Consider raising PrewarmPerClass."), *GetName(), *NoteClass->GetName());
	return CreatePooledNote(NoteClass);
}

ARhythmNote* ARhythmConductor::CreatePooledNote(TSubclassOf<ARhythmNote> NoteClass)
{
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARhythmNote* Note = GetWorld()->SpawnActor<ARhythmNote>(NoteClass, GetActorTransform(), Params);
	if (Note)
	{
		Note->SetPooled();
	}
	return Note;
}

void ARhythmConductor::ReleaseNote(ARhythmNote* Note)
{
	if (!Note)
	{
		return;
	}

	ActiveNotes.RemoveSingleSwap(Note, EAllowShrinking::No);
	Pools.FindOrAdd(Note->GetClass()).Notes.AddUnique(Note);
}

void ARhythmConductor::NotifyNoteResolved(ARhythmNote* Note, ERhythmHitGrade Grade, ERhythmChoice Choice)
{
	OnNoteResolved.Broadcast(Note, Grade, Choice);
}

void ARhythmConductor::ClearActiveNotes()
{
	TArray<ARhythmNote*, TInlineAllocator<32>> NotesToClear(ActiveNotes);
	for (ARhythmNote* Note : NotesToClear)
	{
		if (IsValid(Note))
		{
			Note->SetPooled();
			ReleaseNote(Note);
		}
	}
	ActiveNotes.Reset();
}
