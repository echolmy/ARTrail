// Fill out your copyright notice in the Description page of Project Settings.

#include "ARTrailSubsystem.h"

#include "ARTrailBlueprintFunctionLibrary.h"
#include "Async/Async.h"

void UARTrailSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);
	// ResetWindowState();
}

void UARTrailSubsystem::Deinitialize()
{
	Trails.Reset();
	// CurrentWindowTrails.Reset();
	CurrentWindowPositions.Reset();
	CurrentWindowVelocities.Reset();
	ResetWindowState();

	Super::Deinitialize();
}

void UARTrailSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bAdvanced || Trails.IsEmpty())
	{
		return;
	}

	// Avoid negative or zero delta time which may cause unexpected behavior in window update
	const int64 DeltaUs = UARTrailBlueprintFunctionLibrary::SecondsToMicroseconds(DeltaTime * FMath::Max(0.0f, SpeedRate));

	// Move current time forward by delta, but clamp to the last trail's timestamp to avoid overshooting
	CurrentTime = FMath::Min(CurrentTime + DeltaUs, Trails.Last().Timestamp);
	UpdateWindow();

	if (CurrentTime >= Trails.Last().Timestamp)
	{
		bAdvanced = false;
	}
}

TStatId UARTrailSubsystem::GetStatId() const
{
	return GetStatID();
}

void UARTrailSubsystem::LoadTrailsFromJsonFileAsync(const FString &FilePath)
{
	// Prevent multiple simultaneous loads
	if (bIsLoading)
	{
		UE_LOG(LogTemp, Warning, TEXT("Already loading trails. Please wait until the current loading is finished."));
		return;
	}
	bIsLoading = true;

	// Use a weak pointer to avoid potential issues if the subsystem gets destroyed while the async task is running
	TWeakObjectPtr<UARTrailSubsystem> WeakSubsystemPtr(this);
	FString Path = FilePath;
	// Run the parsing in a background thread to avoid blocking the game thread
	Async(EAsyncExecution::ThreadPool, [WeakSubsystemPtr, Path]()
		  {
		TArray<FARTrail> Trails;
		FString ErrMsg;
		// Parse the trails from the JSON file
		const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonFile(Path, Trails, ErrMsg);

		// Once parsing is done, switch back to the game thread to update the subsystem's state and broadcast the result
		AsyncTask(ENamedThreads::GameThread,
		          [WeakSubsystemPtr, Trails = MoveTemp(Trails), ErrMsg = MoveTemp(ErrMsg), bParsed]() mutable
		          {
			          if (!WeakSubsystemPtr.IsValid())
			          {
				          return;
			          }

			          // Update the subsystem's state based on the parsing result
			          UARTrailSubsystem* Subsystem = WeakSubsystemPtr.Get();
			          Subsystem->bIsLoading = false;

			          if (!bParsed)
			          {
				          UE_LOG(LogTemp, Error, TEXT("Async trail parsed failed: %s."), *ErrMsg);
				          Subsystem->OnTrailsParsedFinished.Broadcast(false, ErrMsg, 0);
				          return;
			          }

			          if (Trails.IsEmpty())
			          {
				          const FString Err = TEXT("No trail points were parsed.");
				          UE_LOG(LogTemp, Error, TEXT("%s"), *Err);
				          Subsystem->OnTrailsParsedFinished.Broadcast(false, Err, 0);
				          return;
			          }

			          // Sort trails by timestamp to ensure correct order
			          Trails.Sort([](const FARTrail& A, const FARTrail& B)
			          {
				          return A.Timestamp < B.Timestamp;
			          });

			          // Update the subsystem and broadcast success
			          Subsystem->Trails = MoveTemp(Trails);
			          Subsystem->CurrentTime = Subsystem->Trails[0].Timestamp;
			          Subsystem->ResetWindowState();
			          Subsystem->UpdateWindow();

			          const FString Msg = FString::Printf(
				          TEXT("Async trail load success. Points Num =%d"), Subsystem->Trails.Num());
			          UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
			          Subsystem->OnTrailsParsedFinished.Broadcast(true, Msg, Subsystem->Trails.Num());
		          }); });
}

void UARTrailSubsystem::GetAllTrails(TArray<FARTrail> &OutTrails) const
{
	OutTrails = Trails;
}

// void UARTrailSubsystem::GetCurrentWindowTrails(TArray<FARTrail> &OutTrails) const
// {
// 	OutTrails = CurrentWindowTrails;
// }

void UARTrailSubsystem::GetCurrentWindowPositions(TArray<FVector> &OutPositions) const
{
	OutPositions = CurrentWindowPositions;
}

void UARTrailSubsystem::GetCurrentWindowVelocities(TArray<float> &OutVelocities) const
{
	OutVelocities = CurrentWindowVelocities;
}

void UARTrailSubsystem::GetCurrentWindowArrays(TArray<FVector> &OutPositions, TArray<float> &OutVelocities) const
{
	OutPositions = CurrentWindowPositions;
	OutVelocities = CurrentWindowVelocities;
}

float UARTrailSubsystem::GetMaxTrailVelocity() const
{
	if (Trails.IsEmpty())
	{
		return 0.0f;
	}

	float MaxVelocity = Trails[0].Velocity;
	for (int32 i = 1; i < Trails.Num(); ++i)
	{
		MaxVelocity = FMath::Max(MaxVelocity, Trails[i].Velocity);
	}
	return MaxVelocity;
}

float UARTrailSubsystem::GetMinTrailVelocity() const
{
	if (Trails.IsEmpty())
	{
		return 0.0f;
	}

	float MinVelocity = Trails[0].Velocity;
	for (int32 i = 1; i < Trails.Num(); ++i)
	{
		MinVelocity = FMath::Min(MinVelocity, Trails[i].Velocity);
	}
	return MinVelocity;
}

void UARTrailSubsystem::SetCurrentTime(float InCurrentTimeSeconds)
{
	if (Trails.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SetCurrentTime ignored: Trails is empty."));
		CurrentTime = 0;
		ResetWindowState();
		// CurrentWindowTrails.Reset();
		CurrentWindowPositions.Reset();
		CurrentWindowVelocities.Reset();
		return;
	}
	const int64 RelativeUs = UARTrailBlueprintFunctionLibrary::SecondsToMicroseconds(InCurrentTimeSeconds);
	const int64 BaseTimestampUs = Trails[0].Timestamp;
	const int64 TargetTimestampUs = BaseTimestampUs + RelativeUs;
	CurrentTime = FMath::Clamp(TargetTimestampUs, BaseTimestampUs, Trails.Last().Timestamp);
	UpdateWindow();
}

void UARTrailSubsystem::SetTrailDurationSeconds(float InTrailDurationSeconds)
{
	const int64 DurationUs = UARTrailBlueprintFunctionLibrary::SecondsToMicroseconds(InTrailDurationSeconds);
	SetTrailDurationUs(DurationUs);
}

void UARTrailSubsystem::ResetWindowState()
{
	WindowStartIdx = 0;
	WindowEndIdx = 0;
	LastUpdateTime = -1;
	LastUpdateDuration = -1;
	bWindowInitialized = false;
}

void UARTrailSubsystem::UpdateWindow()
{
	// CurrentWindowTrails.Reset();
	CurrentWindowPositions.Reset();
	CurrentWindowVelocities.Reset();
	if (Trails.IsEmpty() || TrailDuration <= 0)
	{
		return;
	}

	const int64 WindowStartTime = CurrentTime - TrailDuration;

	// Need full rebuild when:
	// 1) first update
	// 2) time moved backward (not monotonic forward)
	// 3) duration changed
	if (!bWindowInitialized || (CurrentTime < LastUpdateTime) || (TrailDuration != LastUpdateDuration))
	{
		WindowStartIdx = 0;
		WindowEndIdx = 0;

		// Move start pointer to first point >= window start time
		while (WindowStartIdx < Trails.Num() && Trails[WindowStartIdx].Timestamp < WindowStartTime)
		{
			++WindowStartIdx;
		}

		WindowEndIdx = WindowStartIdx;

		// Move end pointer to first point > current time (exclusive bound)
		while (WindowEndIdx < Trails.Num() && Trails[WindowEndIdx].Timestamp <= CurrentTime)
		{
			++WindowEndIdx;
		}

		bWindowInitialized = true;
	}
	else
	{
		// Incremental forward update: pointers only move forward
		while (WindowStartIdx < Trails.Num() && Trails[WindowStartIdx].Timestamp < WindowStartTime)
		{
			++WindowStartIdx;
		}

		while (WindowEndIdx < Trails.Num() && Trails[WindowEndIdx].Timestamp <= CurrentTime)
		{
			++WindowEndIdx;
		}
	}

	// if (WindowStartIdx > WindowEndIdx)
	// {
	// 	WindowStartIdx = WindowEndIdx;
	// }

	LastUpdateTime = CurrentTime;
	LastUpdateDuration = TrailDuration;
	const int32 Count = WindowEndIdx - WindowStartIdx;
	if (Count <= 0)
	{
		return;
	}
	// CurrentWindowTrails.Reserve(Count);
	CurrentWindowPositions.Reserve(Count);
	CurrentWindowVelocities.Reserve(Count);
	for (int32 i = WindowStartIdx; i < WindowEndIdx; ++i)
	{
		const FARTrail &Trail = Trails[i];
		// CurrentWindowTrails.Add(Trail);
		CurrentWindowPositions.Add(Trail.Position);
		CurrentWindowVelocities.Add(Trail.Velocity);
	}
}

void UARTrailSubsystem::SetTrailDurationUs(int64 InTrailDurationUs)
{
	TrailDuration = FMath::Max<int64>(1000, InTrailDurationUs); // avoid zero or negative duration (1000us = 1ms)
	UpdateWindow();
}
