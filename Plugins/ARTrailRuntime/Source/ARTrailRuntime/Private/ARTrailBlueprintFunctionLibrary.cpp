// Fill out your copyright notice in the Description page of Project Settings.

#include "ARTrailBlueprintFunctionLibrary.h"
#include "ARTrailSubsystem.h"
#include "Dom/JsonObject.h"
#include "Misc/DefaultValueHelper.h"
#include "Serialization/JsonSerializer.h"

constexpr double METER_TO_CM = 100.0;

bool UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonFile(const FString& FilePath, TArray<FARTrail>& Trails,
                                                              FString& OutErr)
{
	Trails.Reset();
	OutErr.Reset();

	if (FilePath.IsEmpty())
	{
		OutErr = TEXT("FilePath is empty.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutErr);
		return false;
	}

	FString Path = FilePath;
	if (FPaths::IsRelative(Path))
	{
		Path = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), FilePath));
	}

	FPaths::NormalizeFilename(Path);
	FPaths::CollapseRelativeDirectories(Path);

	if (!FPaths::FileExists(Path))
	{
		OutErr = FString::Printf(TEXT("JSON file not found. Input path ='%s'"), *Path);
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutErr);
		return false;
	}

	FString JsonStr;
	if (!FFileHelper::LoadFileToString(JsonStr, *Path))
	{
		OutErr = FString::Printf(TEXT("Failed to read JSON file: '%s'"), *Path);
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutErr);
		return false;
	}

	return ParseTrailFromJsonString(JsonStr, Trails, OutErr);
}

bool UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(const FString& JsonString, TArray<FARTrail>& Trails,
                                                                FString& OutErr)
{
	Trails.Reset();
	OutErr.Reset();

	// 1. Remove whitespace
	if (JsonString.TrimStartAndEnd().IsEmpty())
	{
		OutErr = TEXT("JsonString is empty.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutErr);
		return false;
	}

	// 2. Deserialize json string
	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		OutErr = TEXT("Failed to deserialize JSON. Root object is invalid.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutErr);
		return false;
	}

	if (RootObject->Values.IsEmpty())
	{
		OutErr = TEXT("JSON root object is empty.");
		UE_LOG(LogTemp, Error, TEXT("%s"), *OutErr);
		return false;
	}

	TArray<FARTrail> InTrails;
	InTrails.Reserve(RootObject->Values.Num());

	// 3. Parse JSON
	for (const auto& Pair : RootObject->Values)
	{
		// 3.1 Parse timestamp and convert it to int64
		int64 Timestamp = 0;
		if (!FDefaultValueHelper::ParseInt64(Pair.Key, Timestamp))
		{
			OutErr = FString::Printf(TEXT("Failed to parse trail timestamp: '%s'."), *Pair.Key);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutErr);
			return false;
		}

		// 3.2 Value (key:timestamp) should be a JSON object
		if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Object)
		{
			OutErr = FString::Printf(TEXT("Trail point at timestamp '%s' should be a JSON object."), *Pair.Key);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutErr);
			return false;
		}

		const auto& TrailObj = Pair.Value->AsObject();
		if (!TrailObj.IsValid())
		{
			OutErr = FString::Printf(TEXT("Trail point at timestamp '%s' should be a JSON object."), *Pair.Key);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutErr);
			return false;
		}

		// 3.3 Extract position
		const TArray<TSharedPtr<FJsonValue>>* TrailPosArr = nullptr;
		if (!TrailObj->TryGetArrayField(TEXT("position"), TrailPosArr) || !TrailPosArr || TrailPosArr->Num() != 3)
		{
			OutErr = FString::Printf(
				TEXT("Trail point at timestamp '%s' should have an array of 3 numbers."), *Pair.Key);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutErr);
			return false;
		}

		double x = 0.0;
		double y = 0.0;
		double z = 0.0;

		if (!(*TrailPosArr)[0].IsValid() || !(*TrailPosArr)[1].IsValid() || !(*TrailPosArr)[2].IsValid() ||
			!(*TrailPosArr)[0]->TryGetNumber(x) || !(*TrailPosArr)[1]->TryGetNumber(y) || !(*TrailPosArr)[2]->
			TryGetNumber(z))
		{
			OutErr = FString::Printf(
				TEXT("Trail point at timestamp '%s' should have numeric values for position."), *Pair.Key);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutErr);
			return false;
		}

		// 3.4 Extract velocity
		double Velocity = 0.0;
		if (!TrailObj->TryGetNumberField(TEXT("velocity"), Velocity))
		{
			OutErr = FString::Printf(TEXT("Trail point at timestamp '%s' should have numeric velocity."), *Pair.Key);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *OutErr);
			return false;
		}

		// 3.5 Write into trails array
		FARTrail TrailPoint;
		TrailPoint.Timestamp = Timestamp;
		TrailPoint.Position = FVector(x * METER_TO_CM, y * METER_TO_CM, z * METER_TO_CM);
		TrailPoint.Velocity = Velocity;

		InTrails.Add(MoveTemp(TrailPoint));
	}

	Trails = MoveTemp(InTrails);
	return true;
}
