// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARTrailSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FARTrail
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	int64 Timestamp = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	FVector Position = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ARTrail")
	float Velocity = 0.0f;
	
};
/**
 * 
 */
UCLASS()
class ARTRAILRUNTIME_API UARTrailSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	TArray<FARTrail> Trails;
};
