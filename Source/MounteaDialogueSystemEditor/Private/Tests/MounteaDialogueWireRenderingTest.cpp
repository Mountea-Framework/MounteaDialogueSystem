// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GraphScheme/MDSBoundedPath.h"
#include "Rendering/DrawElements.h"

#include <limits>

namespace
{
	int32 LineCount(const FSlateWindowElementList& Elements) { return Elements.GetUncachedDrawElements().Get<(uint8)EElementType::ET_Line>().Num(); }
	int32 SplineCount(const FSlateWindowElementList& Elements) { return Elements.GetUncachedDrawElements().Get<(uint8)EElementType::ET_Spline>().Num(); }
}

// The bounded wire renderer: no Slate spline is ever created, so large finite coordinates cannot make Slate's
// unbounded spline subdivision run away. Everything here is pure geometry plus draw-element submission.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaWireGeometryTest,
	"Mountea.DialogueSystemEditor.Layout.WireGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaWireGeometryTest::RunTest(const FString& Parameters)
{
	const double nan = std::numeric_limits<double>::quiet_NaN();
	const double inf = std::numeric_limits<double>::infinity();

	{
		TArray<FVector2D> path = { {0, 0}, {nan, 5}, {10, 10} };
		TestFalse(TEXT("NaN geometry is rejected"), MDSBoundedPath::Sanitize(path));
	}
	{
		TArray<FVector2D> path = { {0, 0}, {inf, 5} };
		TestFalse(TEXT("Infinite geometry is rejected"), MDSBoundedPath::Sanitize(path));
	}
	{
		TArray<FVector2D> path = { {0, 0}, {1.0e12, 5} };
		TestFalse(TEXT("Absurd (finite) geometry is rejected"), MDSBoundedPath::Sanitize(path));
	}
	{
		TArray<FVector2D> path = { {0, 0}, {0, 0}, {0, 0} };
		TestFalse(TEXT("A path that collapses to one point is unusable"), MDSBoundedPath::Sanitize(path));
	}
	{
		TArray<FVector2D> path = { {0, 0}, {0, 0}, {10, 0}, {10, 0}, {10, 10} };
		TestTrue(TEXT("Duplicate points are dropped"), MDSBoundedPath::Sanitize(path) && path.Num() == 3);
	}

	{
		const TArray<FVector2D> corners = { {0, 0}, {0, 100}, {100, 100} };
		TArray<FVector2D> rounded;
		MDSBoundedPath::RoundCorners(corners, 30.0, rounded);
		TestTrue(TEXT("A rounded corner uses at most the permitted number of segments"), rounded.Num() <= 2 + MDSBoundedPath::MaxCornerSegments + 1);
		TestEqual(TEXT("Rounding keeps the first point"), rounded[0], corners[0]);
		TestEqual(TEXT("Rounding keeps the last point"), rounded.Last(), corners.Last());
		TestTrue(TEXT("Rounding actually cuts the corner"), !rounded.Contains(corners[1]));
	}
	{
		// Clearance 6 on one side: the radius must shrink to fit instead of overshooting the neighbouring segment.
		const TArray<FVector2D> corners = { {0, 0}, {0, 100}, {6, 100}, {6, 200} };
		TArray<FVector2D> rounded;
		MDSBoundedPath::RoundCorners(corners, 40.0, rounded);
		for (const FVector2D& point : rounded)
			TestTrue(TEXT("Arcs stay inside the available clearance"), point.X >= -0.001 && point.X <= 6.001 && point.Y >= -0.001 && point.Y <= 200.001);
	}
	{
		const TArray<FVector2D> straight = { {0, 0}, {0, 100}, {0, 200} };
		TArray<FVector2D> rounded;
		MDSBoundedPath::RoundCorners(straight, 30.0, rounded);
		TestEqual(TEXT("Collinear points are not turned into arcs"), rounded.Num(), 3);
		TestEqual(TEXT("Path length is preserved"), MDSBoundedPath::Length(rounded), 200.0);
	}
	{
		const TArray<FVector2D> path = { {0, 0}, {0, 10}, {10, 10} };
		FVector2D direction;
		const FVector2D mid = MDSBoundedPath::SampleAt(path, 15.0, direction);
		TestEqual(TEXT("Sampling follows the displayed path"), mid, FVector2D(5, 10));
		TestEqual(TEXT("Sampling reports the segment direction"), direction, FVector2D(1, 0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaWireDrawTest,
	"Mountea.DialogueSystemEditor.Layout.WireDrawing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaWireDrawTest::RunTest(const FString& Parameters)
{
	const FBox2D screen(FVector2D(0, 0), FVector2D(1920, 1080));

	// Extreme but finite coordinates (the kind that made Slate's spline subdivision run away): must return promptly
	// and only emit lines for the part that is actually visible.
	{
		FSlateWindowElementList elements(nullptr);
		const TArray<FVector2D> path = { {-4.0e6, -4.0e6}, {-4.0e6, 5.0e5}, {960, 5.0e5}, {960, 540}, {4.0e6, 540} };
		const double start = FPlatformTime::Seconds();
		MDSBoundedPath::Draw(elements, 1, path, screen, 2.0f, FLinearColor::White);
		TestTrue(TEXT("Extreme coordinates draw instantly"), FPlatformTime::Seconds() - start < 0.5);
		TestTrue(TEXT("Only the visible part is submitted"), LineCount(elements) >= 1 && LineCount(elements) <= 2);
		TestEqual(TEXT("No Slate spline is ever created"), SplineCount(elements), 0);
	}
	{
		FSlateWindowElementList elements(nullptr);
		const TArray<FVector2D> path = { {5000, 5000}, {5000, 9000}, {9000, 9000} };
		MDSBoundedPath::Draw(elements, 1, path, screen, 2.0f, FLinearColor::White);
		TestEqual(TEXT("A wire entirely off screen submits nothing"), LineCount(elements), 0);
	}
	{
		FSlateWindowElementList elements(nullptr);
		TArray<FVector2D> corners = { {100, 100}, {100, 300}, {600, 300}, {600, 500} };
		TArray<FVector2D> rounded;
		MDSBoundedPath::RoundCorners(corners, 12.0, rounded);
		MDSBoundedPath::Draw(elements, 1, rounded, screen, 2.0f, FLinearColor::White);
		TestEqual(TEXT("A visible wire is a single continuous line element"), LineCount(elements), 1);
		TestEqual(TEXT("Rounded wires use lines, not splines"), SplineCount(elements), 0);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
