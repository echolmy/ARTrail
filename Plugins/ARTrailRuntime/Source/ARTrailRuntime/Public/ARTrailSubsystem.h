// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARTrailSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTrailsParsedFinished, bool, bSuccess,
											   FString, Message,
											   int32, PointsNum);

USTRUCT(BlueprintType)
struct FARTrail
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	int64 Timestamp = 0; // Timestamp in microseconds

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	FVector Position = FVector::ZeroVector; // Position in world space (cm)

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	float Velocity = 0.0f;
	// Velocity in cm/s, can be used for visual effects like changing trail width or color based on speed
};

/**
 *
 */
UCLASS()
class ARTRAILRUNTIME_API UARTrailSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void LoadTrailsFromJsonFileAsync(const FString &FilePath);

	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void GetAllTrails(TArray<FARTrail> &OutTrails) const;

	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void GetCurrentWindowTrails(TArray<FARTrail> &OutTrails) const;

	UFUNCTION(BlueprintCallable, Category = "ARTrail")
	void SetPlaying(bool bInAdvanced) { bAdvanced = bInAdvanced; }

	UFUNCTION(BlueprintCallable, Category = "ARTrail | Time")
	void SetCurrentTime(float InCurrentTimeSeconds);

	UFUNCTION(BlueprintCallable, Category = "ARTrail | Time")
	void SetTrailDurationSeconds(float InTrailDurationSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail | Time")
	int64 CurrentTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARTrail")
	float SpeedRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ARTrail")
	bool bAdvanced = false;

	UPROPERTY(BlueprintAssignable, Category = "ARTrail")
	FOnTrailsParsedFinished OnTrailsParsedFinished;

private:
	void ResetWindowState();
	void UpdateWindow();

	void SetTrailDurationUs(int64 InTrailDurationUs);

	// All trails loaded from the JSON file
	TArray<FARTrail> Trails;
	// Trails that fall within the current time window defined by CurrentTime and TrailDuration
	TArray<FARTrail> CurrentWindowTrails;

	int64 TrailDuration = 500000; // default 0.5s

	bool bIsLoading = false; // Flag to prevent multiple simultaneous loads

	// Two-pointer sliding window: [WindowStartIdx, WindowEndIdx)
	// Start index for the current time window, used to optimize which trails are currently active based on CurrentTime and TrailDuration.
	// This index will be updated as CurrentTime changes to ensure efficient access to the relevant trails without needing to iterate through the entire Trails array each time.
	int64 WindowStartIdx = 0;
	int64 WindowEndIdx = 0; // Exclusive end index for the current time window

	bool bWindowInitialized = false;

	// CurrentTime used in the previous window update.
	// If CurrentTime goes backward compared to this value, window must rebuild from scratch.
	int64 LastUpdateTime = -1;

	// TrailDuration used in the previous window update.
	// If duration changes, incremental two-pointer update is invalid and full rebuild is required.
	int64 LastUpdateDuration = -1;
};
