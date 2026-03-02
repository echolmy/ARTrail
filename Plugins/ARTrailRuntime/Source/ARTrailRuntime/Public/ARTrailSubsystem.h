// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ARTrailUdpReceiver.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARTrailSubsystem.generated.h"

/**
 * Broadcast after an async trail-file parse attempt finishes.
 *
 * @param bSuccess True when parsing succeeded and at least one point was loaded.
 * @param Message Human-readable success or error message.
 * @param PointsNum Number of parsed points on success; 0 on failure.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTrailsParsedFinished, bool, bSuccess,
                                               FString, Message,
                                               int32, PointsNum);

/**
 * Single trail sample parsed from external JSON.
 *
 * Units:
 * - Timestamp: microseconds
 * - Position: centimeters (converted from meters during parsing)
 * - Velocity: meters per second (same as JSON)
 */
USTRUCT(BlueprintType)
struct FARTrail
{
	GENERATED_BODY()

	/** Absolute timestamp in microseconds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	int64 Timestamp = 0;

	/** World-space position in centimeters. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	FVector Position = FVector::ZeroVector;

	/** Per-point velocity value from source data. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	float Velocity = 0.0f;
};

/**
 * World subsystem that manages AR trail data loading, playback time, and sliding-window queries.
 *
 * Responsibilities:
 * - Load and parse trail JSON asynchronously.
 * - Store the full sorted trail sequence.
 * - Maintain cached arrays for the active time window for Blueprint/Niagara consumption.
 * - Advance playback in Tick when bAdvanced is enabled.
 *
 * Active window definition:
 * - WindowStartTime = CurrentTime - TrailDuration
 * - Include points where Timestamp is in [WindowStartTime, CurrentTime]
 */
UCLASS()
class ARTRAILRUNTIME_API UARTrailSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Called automatically by Unreal when this subsystem is created. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	/** Called automatically by Unreal when this subsystem is shutting down. */
	virtual void Deinitialize() override;
	/** Advances CurrentTime and updates window cache when playback is active. */
	virtual void Tick(float DeltaTime) override;
	/** Tick stat id required by UTickableWorldSubsystem. */
	virtual TStatId GetStatId() const override;

	/**
	 * Parse a trail JSON file on a worker thread and apply results on the game thread.
	 *
	 * Behavior:
	 * - Returns immediately.
	 * - Ignores the call when another async load is already in progress.
	 * - On completion, broadcasts OnTrailsParsedFinished.
	 *
	 * @param FilePath Absolute path or path relative to ProjectContentDir().
	 */
	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void LoadTrailsFromJsonFileAsync(const FString& FilePath);

	/** Get all loaded trail samples. */
	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void GetAllTrails(TArray<FARTrail>& OutTrails) const;

	// UFUNCTION(BlueprintCallable, Category = "ARTrail")
	// void GetCurrentWindowTrails(TArray<FARTrail> &OutTrails) const;

	/** Get cached positions for the current active window. */
	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void GetCurrentWindowPositions(TArray<FVector>& OutPositions) const;

	/** Get cached velocities for the current active window. */
	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void GetCurrentWindowVelocities(TArray<float>& OutVelocities) const;

	/** Get both cached position and velocity arrays for the current active window. */
	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void GetCurrentWindowArrays(TArray<FVector>& OutPositions, TArray<float>& OutVelocities) const;

	/**
	 * Get max velocity among all loaded points.
	 *
	 * @return 0.0 when no trails are loaded.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ARTrail")
	float GetMaxTrailVelocity() const;

	/**
	 * Get min velocity among all loaded points.
	 *
	 * @return 0.0 when no trails are loaded.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ARTrail")
	float GetMinTrailVelocity() const;

	/** Enable or disable automatic time advancement during Tick. */
	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void SetPlaying(bool bInAdvanced) { bAdvanced = bInAdvanced; }

	/**
	 * Set playback time using seconds relative to the first trail timestamp.
	 *
	 * Input is converted to microseconds and clamped to loaded data range.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARTrail | Time")
	void SetCurrentTime(float InCurrentTimeSeconds);

	/**
	 * Set active window duration in seconds.
	 *
	 * Internally converted to microseconds and clamped to at least 1 ms.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARTrail | Time")
	void SetTrailDurationSeconds(float InTrailDurationSeconds);

	/** Current absolute playback timestamp in microseconds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail | Time")
	int64 CurrentTime;

	/** Playback speed multiplier used by Tick when bAdvanced is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARTrail", meta=(ClampMin="0.1"))
	float SpeedRate = 1.0f;

	/** Whether Tick should advance CurrentTime automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARTrail")
	bool bAdvanced = false;

	/** Async load completion event for Blueprint listeners. */
	UPROPERTY(BlueprintAssignable, Category = "ARTrail")
	FOnTrailsParsedFinished OnTrailsParsedFinished;

private:
	/** Reset sliding-window indices and incremental-update history. */
	void ResetWindowState();
	
	/** Rebuild or incrementally update cached arrays for the active window. */
	void UpdateWindow();

	/** Internal duration setter in microseconds. Enforces minimum duration. */
	void SetTrailDurationUs(int64 InTrailDurationUs);

	/** All trail samples loaded from the most recent successful parse. */
	TArray<FARTrail> Trails;

	/** Cached positions for the current sliding window. */
	TArray<FVector> CurrentWindowPositions;
	
	/** Cached velocities for the current sliding window. */
	TArray<float> CurrentWindowVelocities;

	/** Active window duration in microseconds. */
	int64 TrailDuration = 500000; // default 0.5s

	/** Flag to prevent multiple simultaneous async loads. */
	bool bIsLoading = false;

	/**
	 * Sliding-window bounds over Trails using half-open indexing: [WindowStartIdx, WindowEndIdx).
	 *
	 * Mapping to the time window [WindowStartTime, CurrentTime]:
	 * - WindowStartIdx points to the first trail with Timestamp >= WindowStartTime.
	 * - WindowEndIdx points one past the last trail with Timestamp <= CurrentTime.
	 */
	int64 WindowStartIdx = 0;
	/** Exclusive end index of the current sliding window. */
	int64 WindowEndIdx = 0;

	/** Whether the sliding-window pointers have been initialized at least once. */
	bool bWindowInitialized = false;

	/** Last CurrentTime used by UpdateWindow; used to detect backward time jumps. */
	int64 LastUpdateTime = -1;

	/** Last TrailDuration used by UpdateWindow; duration changes force a full window rebuild. */
	int64 LastUpdateDuration = -1;
	
	TUniquePtr<FARTrailUdpReceiver> UdpReceiver;
};
