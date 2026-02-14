// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARTrailBlueprintFunctionLibrary.generated.h"

struct FARTrail;

/**
 * 
 */
UCLASS()
class ARTRAILRUNTIME_API UARTrailBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="ARTrail | JSON")
	static bool ParseTrailFromJsonFile(const FString& FilePath, TArray<FARTrail>& Trails, FString& OutErr);
	
	UFUNCTION(BlueprintCallable, Category="ARTrail | JSON")
	static bool ParseTrailFromJsonString(const FString& JsonString, TArray<FARTrail>& Trails, FString& OutErr);
	
};
