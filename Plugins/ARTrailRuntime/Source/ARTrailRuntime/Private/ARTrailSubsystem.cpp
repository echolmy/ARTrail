// Fill out your copyright notice in the Description page of Project Settings.


#include "ARTrailSubsystem.h"

void UARTrailSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UARTrailSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UARTrailSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId UARTrailSubsystem::GetStatId() const
{
	return GetStatID();
}



