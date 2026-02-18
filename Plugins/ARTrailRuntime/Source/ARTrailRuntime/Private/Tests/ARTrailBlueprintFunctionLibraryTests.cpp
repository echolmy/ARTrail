#if WITH_DEV_AUTOMATION_TESTS

#include "ARTrailBlueprintFunctionLibrary.h"
#include "ARTrailSubsystem.h"
#include "Misc/AutomationTest.h"

namespace ARTrailBlueprintFunctionLibraryTests
{
	// Shared fixtures used by multiple tests to keep expectations consistent.
	constexpr double FloatTolerance = 0.001;
	const FString ValidSingleTrailJson = TEXT("{\"1000000\":{\"position\":[1.5,2.0,-3.25],\"velocity\":4.5}}");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringEmptyInputTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.EmptyInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringEmptyInputTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr = TEXT("seed");
	// The parser logs an expected error on this branch; mark it to avoid noisy test output.
	AddExpectedErrorPlain(TEXT("JsonString is empty."));

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(TEXT(" \t\n"), Trails, OutErr);

	TestFalse(TEXT("Empty JSON string should fail to parse."), bParsed);
	TestEqual(TEXT("Error message should match empty-input branch."), OutErr, FString(TEXT("JsonString is empty.")));
	TestTrue(TEXT("Output trails should be cleared when parsing fails."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringInvalidJsonTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.InvalidJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringInvalidJsonTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr;
	// This test targets JSON deserialization failure before schema checks begin.
	AddExpectedErrorPlain(TEXT("Failed to deserialize JSON. Root object is invalid."));

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(TEXT("{\"broken_json\":"), Trails, OutErr);

	TestFalse(TEXT("Malformed JSON should fail to deserialize."), bParsed);
	TestEqual(
		TEXT("Error message should match deserialize-failure branch."),
		OutErr,
		FString(TEXT("Failed to deserialize JSON. Root object is invalid.")));
	TestTrue(TEXT("Output trails should stay empty for invalid JSON."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringInvalidTimestampTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.InvalidTimestamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringInvalidTimestampTest::RunTest(const FString& Parameters)
{
	// Timestamp keys are part of the schema and must parse as int64 microseconds.
	const FString JsonString = TEXT("{\"not_a_number\":{\"position\":[0,0,0],\"velocity\":1.0}}");
	TArray<FARTrail> Trails;
	FString OutErr;

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(JsonString, Trails, OutErr);

	TestFalse(TEXT("Non-integer timestamp key should fail."), bParsed);
	TestTrue(TEXT("Error should point to timestamp parsing failure."), OutErr.Contains(TEXT("Failed to parse trail timestamp")));
	TestTrue(TEXT("Output trails should be empty when timestamp is invalid."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringEmptyRootObjectTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.EmptyRootObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringEmptyRootObjectTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr;
	// Root object exists but contains no trail samples, which is treated as invalid input.
	AddExpectedErrorPlain(TEXT("JSON root object is empty."));

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(TEXT("{}"), Trails, OutErr);

	TestFalse(TEXT("Empty JSON object should fail because no trail values exist."), bParsed);
	TestEqual(TEXT("Error should match empty-root-object branch."), OutErr, FString(TEXT("JSON root object is empty.")));
	TestTrue(TEXT("Output trails should be empty when root object is empty."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringTrailValueNotObjectTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.TrailValueNotObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringTrailValueNotObjectTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr;

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(TEXT("{\"1000000\":42}"), Trails, OutErr);

	TestFalse(TEXT("Each timestamp entry should map to a JSON object."), bParsed);
	TestTrue(TEXT("Error should mention object requirement for each trail point."), OutErr.Contains(TEXT("should be a JSON object")));
	TestTrue(TEXT("Output trails should stay empty when trail value is not an object."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringInvalidPositionArrayTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.InvalidPositionArray",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringInvalidPositionArrayTest::RunTest(const FString& Parameters)
{
	// Position must be exactly [x, y, z]; wrong shape should fail fast.
	const FString JsonString = TEXT("{\"1000000\":{\"position\":[1,2],\"velocity\":1.0}}");
	TArray<FARTrail> Trails;
	FString OutErr;

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(JsonString, Trails, OutErr);

	TestFalse(TEXT("Position array with length != 3 should fail."), bParsed);
	TestTrue(TEXT("Error should mention the expected array of 3 numbers."), OutErr.Contains(TEXT("array of 3 numbers")));
	TestTrue(TEXT("Output trails should be empty on position-shape failure."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringNonNumericPositionTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.NonNumericPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringNonNumericPositionTest::RunTest(const FString& Parameters)
{
	// Position array entries are required to be numeric for FVector construction.
	const FString JsonString = TEXT("{\"1000000\":{\"position\":[1,\"bad\",3],\"velocity\":1.0}}");
	TArray<FARTrail> Trails;
	FString OutErr;

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(JsonString, Trails, OutErr);

	TestFalse(TEXT("Non-numeric position values should fail parsing."), bParsed);
	TestTrue(TEXT("Error should mention numeric position requirement."), OutErr.Contains(TEXT("numeric values for position")));
	TestTrue(TEXT("Output trails should be empty on non-numeric position failure."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringMissingVelocityTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.MissingVelocity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringMissingVelocityTest::RunTest(const FString& Parameters)
{
	// Velocity is mandatory in each sample object.
	const FString JsonString = TEXT("{\"1000000\":{\"position\":[1,2,3]}}");
	TArray<FARTrail> Trails;
	FString OutErr;

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(JsonString, Trails, OutErr);

	TestFalse(TEXT("Missing velocity field should fail."), bParsed);
	TestTrue(TEXT("Error should mention numeric velocity requirement."), OutErr.Contains(TEXT("numeric velocity")));
	TestTrue(TEXT("Output trails should be empty on velocity failure."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonStringSuccessTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonString.Success",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonStringSuccessTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr = TEXT("seed");

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonString(
		ARTrailBlueprintFunctionLibraryTests::ValidSingleTrailJson,
		Trails,
		OutErr);

	TestTrue(TEXT("Well-formed JSON should parse successfully."), bParsed);
	TestTrue(TEXT("Error output should be empty on success."), OutErr.IsEmpty());
	TestEqual(TEXT("Exactly one trail point should be parsed."), Trails.Num(), 1);

	if (Trails.Num() == 1)
	{
		// Happy-path contract: timestamp is preserved and position converts from meters to centimeters.
		const FARTrail& Trail = Trails[0];
		TestEqual(TEXT("Timestamp should match source JSON key."), Trail.Timestamp, int64(1000000));
		TestTrue(
			TEXT("Position.X should convert meters to centimeters."),
			FMath::IsNearlyEqual(Trail.Position.X, 150.0, ARTrailBlueprintFunctionLibraryTests::FloatTolerance));
		TestTrue(
			TEXT("Position.Y should convert meters to centimeters."),
			FMath::IsNearlyEqual(Trail.Position.Y, 200.0, ARTrailBlueprintFunctionLibraryTests::FloatTolerance));
		TestTrue(
			TEXT("Position.Z should convert meters to centimeters."),
			FMath::IsNearlyEqual(Trail.Position.Z, -325.0, ARTrailBlueprintFunctionLibraryTests::FloatTolerance));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonFileEmptyPathTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonFile.EmptyPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonFileEmptyPathTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr;
	// File API should short-circuit before any path normalization or file IO.
	AddExpectedErrorPlain(TEXT("FilePath is empty."));

	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonFile(TEXT(""), Trails, OutErr);

	TestFalse(TEXT("Empty file path should fail."), bParsed);
	TestEqual(TEXT("Error should match empty file path branch."), OutErr, FString(TEXT("FilePath is empty.")));
	TestTrue(TEXT("Output trails should stay empty for empty file path."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonFileNotFoundTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonFile.NotFound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonFileNotFoundTest::RunTest(const FString& Parameters)
{
	TArray<FARTrail> Trails;
	FString OutErr;
	// Relative path is resolved against Content and then validated for existence.
	AddExpectedErrorPlain(TEXT("JSON file not found."));

	const bool bParsed =
		UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonFile(TEXT("this/path/does/not/exist/trails.json"), Trails, OutErr);

	TestFalse(TEXT("Nonexistent file path should fail."), bParsed);
	TestTrue(TEXT("Error should mention file-not-found condition."), OutErr.Contains(TEXT("JSON file not found.")));
	TestTrue(TEXT("Output trails should stay empty when file is missing."), Trails.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FParseTrailFromJsonFileSuccessTest,
	"ARTrail.Runtime.BlueprintFunctionLibrary.ParseTrailFromJsonFile.Success",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FParseTrailFromJsonFileSuccessTest::RunTest(const FString& Parameters)
{
	// Integration-style check for the file wrapper over ParseTrailFromJsonString.
	TArray<FARTrail> Trails;
	FString OutErr;
	const bool bParsed = UARTrailBlueprintFunctionLibrary::ParseTrailFromJsonFile(TEXT("trail-points.json"), Trails, OutErr);

	TestTrue(TEXT("Existing JSON file in Content should parse successfully."), bParsed);
	TestTrue(TEXT("Error output should be empty for valid JSON file."), OutErr.IsEmpty());
	TestTrue(TEXT("Parsed trail list should not be empty."), Trails.Num() > 0);
	if (Trails.Num() > 0)
	{
		TestTrue(TEXT("Timestamps should be positive from source data."), Trails[0].Timestamp > 0);
		TestTrue(TEXT("Velocity should be non-negative for sample data."), Trails[0].Velocity >= 0.0f);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
