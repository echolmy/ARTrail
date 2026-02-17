// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARTrailBlueprintFunctionLibrary.generated.h"

struct FARTrail;

/**
 * Blueprint-callable utility functions for AR trail data ingestion.
 *
 * This library provides:
 * 1) JSON parsing helpers that convert external trail payloads into FARTrail arrays.
 * 2) Time conversion helpers between seconds and microseconds.
 *
 * Parsing contract:
 * - Root JSON must be an object where each key is a timestamp in microseconds.
 * - Each value must be an object containing:
 *   - "position": [x, y, z] (meters in source JSON; converted to centimeters in output)
 *   - "velocity": number
 *
 * Notes:
 * - Parsing functions clear both Trails and OutErr at the start of every call.
 * - On failure, they return false OutErr.
 * - Parsed output order follows JSON object iteration order and is not guaranteed to be sorted.
 *   If chronological order is required, sort by FARTrail::Timestamp after parsing.
 */
UCLASS()
class ARTRAILRUNTIME_API UARTrailBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Parse trail points from a JSON file.
	 *
	 * Path rules:
	 * - Absolute paths are used directly.
	 * - Relative paths are resolved against ProjectContentDir().
	 *
	 * @param FilePath Input JSON path (absolute or relative to Content).
	 * @param Trails Parsed trail points (cleared before parsing).
	 * @param OutErr Error message when parsing fails; empty on success.
	 * @return true when the file is read and parsed successfully, otherwise false.
	 */
	UFUNCTION(BlueprintCallable, Category="ARTrail | JSON")
	static bool ParseTrailFromJsonFile(const FString& FilePath, TArray<FARTrail>& Trails, FString& OutErr);

	/**
	 * Parse trail points from a JSON string.
	 *
	 * Expected schema:
	 * {
	 *   "<timestamp_us>": { "position": [x, y, z], "velocity": number },
	 *   ...
	 * }
	 *
	 * @param JsonString Source JSON string.
	 * @param Trails Parsed trail points (cleared before parsing). Position is converted m -> cm.
	 * @param OutErr Error message when parsing fails; empty on success.
	 * @return true when the string matches the expected schema and is parsed successfully.
	 */
	UFUNCTION(BlueprintCallable, Category="ARTrail | JSON")
	static bool ParseTrailFromJsonString(const FString& JsonString, TArray<FARTrail>& Trails, FString& OutErr);

	/**
	 * Convert seconds to microseconds.
	 *
	 * The result is truncated when converting from float to int64.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARTrail | Time")
	static FORCEINLINE int64 SecondsToMicroseconds(float Seconds) { return Seconds * 1e6; };

	/**
	 * Convert microseconds to seconds.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARTrail | Time")
	static FORCEINLINE float MicrosecondsToSeconds(int64 Microseconds) { return Microseconds / 1e6; };
};
