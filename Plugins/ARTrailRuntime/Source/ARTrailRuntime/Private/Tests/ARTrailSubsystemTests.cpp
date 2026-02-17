#if WITH_DEV_AUTOMATION_TESTS

#include "ARTrailBlueprintFunctionLibrary.h"
#include "ARTrailSubsystem.h"
#include "Async/TaskGraphInterfaces.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"

namespace ARTrailSubsystemTests
{
	bool WaitUntil(const TFunctionRef<bool()>& Predicate, const double TimeoutSeconds)
	{
		const double StartTime = FPlatformTime::Seconds();
		while (!Predicate())
		{
			FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::GameThread);
			if (FPlatformTime::Seconds() - StartTime > TimeoutSeconds)
			{
				return false;
			}
			FPlatformProcess::Sleep(0.01f);
		}

		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FARTrailSubsystemSetCurrentTimeWithEmptyTrailsTest,
	"ARTrail.Runtime.Subsystem.SetCurrentTime.WithEmptyTrails",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FARTrailSubsystemSetCurrentTimeWithEmptyTrailsTest::RunTest(const FString& Parameters)
{
	UARTrailSubsystem* Subsystem = NewObject<UARTrailSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Subsystem instance should be created."), Subsystem))
	{
		return false;
	}

	Subsystem->AddToRoot();

	Subsystem->SetCurrentTime(123.0f);
	TestEqual(TEXT("CurrentTime should reset to 0 when no trails exist."), Subsystem->CurrentTime, int64(0));
	TestEqual(TEXT("Max trail velocity should be 0 when no trails exist."), Subsystem->GetMaxTrailVelocity(), 0.0f);
	TestEqual(TEXT("Min trail velocity should be 0 when no trails exist."), Subsystem->GetMinTrailVelocity(), 0.0f);

	TArray<FARTrail> AllTrails;
	Subsystem->GetAllTrails(AllTrails);
	TestTrue(TEXT("All trails should be empty by default."), AllTrails.IsEmpty());

	TArray<FVector> WindowPositions;
	Subsystem->GetCurrentWindowPositions(WindowPositions);
	TestTrue(TEXT("Window positions should be empty by default."), WindowPositions.IsEmpty());

	TArray<float> WindowVelocities;
	Subsystem->GetCurrentWindowVelocities(WindowVelocities);
	TestTrue(TEXT("Window velocities should be empty by default."), WindowVelocities.IsEmpty());

	Subsystem->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FARTrailSubsystemAsyncLoadAndWindowBehaviorTest,
	"ARTrail.Runtime.Subsystem.LoadTrailsFromJsonFileAsync.SuccessAndWindowBehavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FARTrailSubsystemAsyncLoadAndWindowBehaviorTest::RunTest(const FString& Parameters)
{
	UARTrailSubsystem* Subsystem = NewObject<UARTrailSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Subsystem instance should be created."), Subsystem))
	{
		return false;
	}

	Subsystem->AddToRoot();

	Subsystem->LoadTrailsFromJsonFileAsync(TEXT("trail-points.json"));
	const bool bCompleted = ARTrailSubsystemTests::WaitUntil(
		[Subsystem]()
		{
			TArray<FARTrail> PollTrails;
			Subsystem->GetAllTrails(PollTrails);
			return PollTrails.Num() > 0;
		},
		8.0);

	if (!TestTrue(TEXT("Async load callback should complete within timeout."), bCompleted))
	{
		Subsystem->RemoveFromRoot();
		return false;
	}

	TArray<FARTrail> AllTrails;
	Subsystem->GetAllTrails(AllTrails);
	TestTrue(TEXT("Loaded point count should be greater than zero."), AllTrails.Num() > 0);
	if (AllTrails.Num() > 1)
	{
		TestTrue(
			TEXT("Trails should be sorted by ascending timestamp after async load."),
			AllTrails[0].Timestamp <= AllTrails.Last().Timestamp);
	}

	if (!AllTrails.IsEmpty())
	{
		float ExpectedMaxVelocity = AllTrails[0].Velocity;
		float ExpectedMinVelocity = AllTrails[0].Velocity;
		for (int32 i = 1; i < AllTrails.Num(); ++i)
		{
			ExpectedMaxVelocity = FMath::Max(ExpectedMaxVelocity, AllTrails[i].Velocity);
			ExpectedMinVelocity = FMath::Min(ExpectedMinVelocity, AllTrails[i].Velocity);
		}

		TestEqual(
			TEXT("GetMaxTrailVelocity should match max value from loaded trails."),
			Subsystem->GetMaxTrailVelocity(),
			ExpectedMaxVelocity);
		TestEqual(
			TEXT("GetMinTrailVelocity should match min value from loaded trails."),
			Subsystem->GetMinTrailVelocity(),
			ExpectedMinVelocity);
	}

	if (!AllTrails.IsEmpty())
	{
		TestEqual(
			TEXT("CurrentTime should be initialized to first trail timestamp after successful load."),
			Subsystem->CurrentTime,
			AllTrails[0].Timestamp);
	}

	Subsystem->SetTrailDurationSeconds(0.2f);
	Subsystem->SetCurrentTime(0.0f);
	if (!AllTrails.IsEmpty())
	{
		TestEqual(TEXT("SetCurrentTime should clamp low bound to first timestamp."), Subsystem->CurrentTime, AllTrails[0].Timestamp);
	}

	TArray<FVector> StartWindowPositions;
	TArray<float> StartWindowVelocities;
	Subsystem->GetCurrentWindowArrays(StartWindowPositions, StartWindowVelocities);
	TestTrue(TEXT("Current window should include at least one point near start time."), StartWindowPositions.Num() > 0);
	TestEqual(
		TEXT("Positions and velocities should have the same count near start time."),
		StartWindowPositions.Num(),
		StartWindowVelocities.Num());

	Subsystem->SetCurrentTime(1000000.0f);
	if (!AllTrails.IsEmpty())
	{
		TestEqual(TEXT("SetCurrentTime should clamp high bound to last timestamp."), Subsystem->CurrentTime, AllTrails.Last().Timestamp);
	}

	TArray<FVector> EndWindowPositions;
	TArray<float> EndWindowVelocities;
	Subsystem->GetCurrentWindowArrays(EndWindowPositions, EndWindowVelocities);
	TestTrue(TEXT("Current window should include at least one point near end time."), EndWindowPositions.Num() > 0);
	TestEqual(
		TEXT("Positions and velocities should have the same count near end time."),
		EndWindowPositions.Num(),
		EndWindowVelocities.Num());

	Subsystem->SetPlaying(true);
	TestTrue(TEXT("SetPlaying(true) should enable advanced state."), Subsystem->bAdvanced);
	Subsystem->SetPlaying(false);
	TestFalse(TEXT("SetPlaying(false) should disable advanced state."), Subsystem->bAdvanced);

	Subsystem->RemoveFromRoot();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FARTrailSubsystemWindowGroundTruthTrailPointsFileTest,
	"ARTrail.Runtime.Subsystem.Window.GroundTruth.TrailPointsFile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FARTrailSubsystemWindowGroundTruthTrailPointsFileTest::RunTest(const FString& Parameters)
{
	UARTrailSubsystem* Subsystem = NewObject<UARTrailSubsystem>(GetTransientPackage());
	if (!TestNotNull(TEXT("Subsystem instance should be created."), Subsystem))
	{
		return false;
	}

	Subsystem->AddToRoot();

	const FString JsonFilePath = TEXT("trail-points.json");
	const float CurrentTimeSeconds = 2.0f;
	const float TrailDurationSeconds = 0.5f;

	Subsystem->LoadTrailsFromJsonFileAsync(JsonFilePath);
	const bool bCompleted = ARTrailSubsystemTests::WaitUntil(
		[Subsystem]()
		{
			TArray<FARTrail> PollTrails;
			Subsystem->GetAllTrails(PollTrails);
			return PollTrails.Num() > 0;
		},
		8.0);

	if (!TestTrue(TEXT("trail-points async load callback should complete within timeout."), bCompleted))
	{
		Subsystem->RemoveFromRoot();
		return false;
	}

	Subsystem->SetPlaying(false);
	Subsystem->SetTrailDurationSeconds(TrailDurationSeconds);
	Subsystem->SetCurrentTime(CurrentTimeSeconds);

	TArray<FVector> ActualPositions;
	Subsystem->GetCurrentWindowPositions(ActualPositions);

	TArray<FARTrail> ParsedTrails;
	FString ParseErr;
	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonFile(JsonFilePath, ParsedTrails, ParseErr);
	if (!TestTrue(TEXT("trail-points JSON should parse successfully for ground-truth comparison."), bParsed))
	{
		AddError(FString::Printf(TEXT("ParseTrailFromJsonFile failed: %s"), *ParseErr));
		Subsystem->RemoveFromRoot();
		return false;
	}

	ParsedTrails.Sort(
		[](const FARTrail& A, const FARTrail& B)
		{
			return A.Timestamp < B.Timestamp;
		});

	if (!TestTrue(TEXT("Parsed trail-points should not be empty."), ParsedTrails.Num() > 0))
	{
		Subsystem->RemoveFromRoot();
		return false;
	}

	const int64 BaseTimestampUs = ParsedTrails[0].Timestamp;
	const int64 LastTimestampUs = ParsedTrails.Last().Timestamp;
	const int64 DurationUs = UARTrailBlueprintFunctionLibrary::SecondsToMicroseconds(TrailDurationSeconds);
	const int64 TargetTimestampUs = BaseTimestampUs +
		UARTrailBlueprintFunctionLibrary::SecondsToMicroseconds(CurrentTimeSeconds);
	const int64 CurrentTimestampUs = FMath::Clamp(TargetTimestampUs, BaseTimestampUs, LastTimestampUs);
	const int64 WindowStartTimestampUs = CurrentTimestampUs - DurationUs;

	TArray<FVector> ExpectedPositions;
	ExpectedPositions.Reserve(ParsedTrails.Num());
	for (const FARTrail& Trail : ParsedTrails)
	{
		if (Trail.Timestamp >= WindowStartTimestampUs && Trail.Timestamp <= CurrentTimestampUs)
		{
			ExpectedPositions.Add(Trail.Position);
		}
	}

	TestTrue(TEXT("Ground-truth window should contain at least one point."), ExpectedPositions.Num() > 0);
	TestEqual(
		TEXT("Ground-truth point count from JSON window should equal subsystem current window count."),
		ActualPositions.Num(),
		ExpectedPositions.Num());

	if (!ActualPositions.IsEmpty() && ActualPositions.Num() == ExpectedPositions.Num())
	{
		const float ToleranceCm = 0.001f;
		const int32 LastIdx = ActualPositions.Num() - 1;
		const int32 MidIdx = LastIdx / 2;

		TestTrue(
			TEXT("First point should match ground truth."),
			ActualPositions[0].Equals(ExpectedPositions[0], ToleranceCm));
		TestTrue(
			TEXT("Middle point should match ground truth."),
			ActualPositions[MidIdx].Equals(ExpectedPositions[MidIdx], ToleranceCm));
		TestTrue(
			TEXT("Last point should match ground truth."),
			ActualPositions[LastIdx].Equals(ExpectedPositions[LastIdx], ToleranceCm));
	}

	Subsystem->RemoveFromRoot();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
