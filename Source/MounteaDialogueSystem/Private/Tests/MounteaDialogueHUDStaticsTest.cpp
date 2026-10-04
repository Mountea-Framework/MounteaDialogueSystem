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

#include "Tests/AutomationCommon.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/SceneComponent.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Helpers/MounteaDialogueHUDStatics.h"
#include "Interfaces/HUD/MounteaDialogueHUDClassInterface.h"
#include "Subsystem/MounteaDialogueViewportHUDSubsystem.h"

// Tests in this file cover UMounteaDialogueHUDStatics' "viewport manager" helpers when handed a plain
// APlayerController (one that does NOT implement IMounteaDialogueHUDClassInterface). That path falls
// back to the UMounteaDialogueViewportHUDSubsystem on the controller's ULocalPlayer, and must reach the
// subsystem through IMounteaDialogueHUDClassInterface::Execute_*. Calling the interface event directly
// trips the engine's "Do not directly call Event functions in Interfaces" assertion (Bugreport #1).
//
// A real UWorld (FTestWorldWrapper) plus a ULocalPlayer registered via PlayerAdded() is the minimum
// needed for APlayerController::GetLocalPlayer()->GetSubsystem<>() to resolve to a live subsystem.
// Widget creation / viewport attachment is deliberately NOT exercised here - that needs a real viewport.

namespace MounteaDialogueHUDStaticsTest
{
	// Owns the test world, a PlayerController, and a registered ULocalPlayer for the duration of a test.
	struct FPlayerControllerFixture
	{
		FTestWorldWrapper WorldWrapper;
		APlayerController* PlayerController = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
		UMounteaDialogueViewportHUDSubsystem* Subsystem = nullptr;

		bool Setup(FAutomationTestBase& Test)
		{
			// Projects using CommonUI without a CommonGameViewportClient log this error on any world
			// bring-up. It is unrelated to what is under test, but the automation framework treats every
			// logged error as a failure. Occurrence count 0 = tolerated any number of times (including none).
			Test.AddExpectedError(TEXT("Using CommonUI without a CommonGameViewportClient"), EAutomationExpectedErrorFlags::Contains, 0);

			if (!WorldWrapper.CreateTestWorld(EWorldType::Game))
			{
				Test.AddError(TEXT("Failed to create test world"));
				return false;
			}

			UWorld* world = WorldWrapper.GetTestWorld();
			if (!world)
			{
				Test.AddError(TEXT("Test world is null"));
				return false;
			}
			WorldWrapper.BeginPlayInTestWorld();

			PlayerController = world->SpawnActor<APlayerController>();
			if (!PlayerController)
			{
				Test.AddError(TEXT("Failed to spawn PlayerController"));
				return false;
			}

			LocalPlayer = NewObject<ULocalPlayer>(GEngine);
			if (!LocalPlayer)
			{
				Test.AddError(TEXT("Failed to create LocalPlayer"));
				return false;
			}

			// Initializes the LocalPlayer's subsystem collection, which is what makes
			// GetSubsystem<UMounteaDialogueViewportHUDSubsystem>() non-null.
			LocalPlayer->PlayerAdded(nullptr, FPlatformUserId::CreateFromInternalId(0));
			PlayerController->Player = LocalPlayer;

			Subsystem = LocalPlayer->GetSubsystem<UMounteaDialogueViewportHUDSubsystem>();
			return true;
		}

		void Teardown(FAutomationTestBase& Test)
		{
			if (LocalPlayer)
			{
				LocalPlayer->PlayerRemoved();
				LocalPlayer = nullptr;
			}
			WorldWrapper.ForwardErrorMessages(&Test);
			WorldWrapper.DestroyTestWorld(true);
		}
	};
}

// Direct regression for Bugreport #1: GetViewportWidget with a PlayerController used to call the
// interface event stub on the subsystem and assert. It must return the subsystem's widget instead
// (null here, since nothing has initialised the viewport widget).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGetViewportWidgetPlayerControllerTest,
	"Mountea.DialogueSystem.HUDStatics.GetViewportWidget.PlayerControllerFallbackDoesNotAssert",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGetViewportWidgetPlayerControllerTest::RunTest(const FString& Parameters)
{
	MounteaDialogueHUDStaticsTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	TestFalse(TEXT("Precondition: a plain PlayerController must not implement the HUD class interface (else the fallback isn't exercised)"),
		fixture.PlayerController->Implements<UMounteaDialogueHUDClassInterface>());
	TestNotNull(TEXT("Precondition: the LocalPlayer must resolve a ViewportHUD subsystem"), fixture.Subsystem);

	const UUserWidget* widget = UMounteaDialogueHUDStatics::GetViewportWidget(fixture.PlayerController);
	TestNull(TEXT("GetViewportWidget via the PlayerController fallback should return the (uninitialised) subsystem widget without asserting"), widget);

	fixture.Teardown(*this);
	return true;
}

// Guards the already-working path: handing the subsystem itself in takes the Implements<> branch.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGetViewportWidgetSubsystemTest,
	"Mountea.DialogueSystem.HUDStatics.GetViewportWidget.SubsystemPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGetViewportWidgetSubsystemTest::RunTest(const FString& Parameters)
{
	MounteaDialogueHUDStaticsTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	if (!TestNotNull(TEXT("Precondition: the LocalPlayer must resolve a ViewportHUD subsystem"), fixture.Subsystem))
	{
		fixture.Teardown(*this);
		return false;
	}

	const UUserWidget* widget = UMounteaDialogueHUDStatics::GetViewportWidget(fixture.Subsystem);
	TestNull(TEXT("GetViewportWidget with the subsystem directly should return its (uninitialised) widget"), widget);

	fixture.Teardown(*this);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGetViewportWidgetInvalidInputTest,
	"Mountea.DialogueSystem.HUDStatics.GetViewportWidget.InvalidInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGetViewportWidgetInvalidInputTest::RunTest(const FString& Parameters)
{
	TestNull(TEXT("A null viewport manager should return null"), UMounteaDialogueHUDStatics::GetViewportWidget(nullptr));

	// UObject itself is abstract, so use a concrete class that is neither the interface nor a PlayerController.
	UObject* unrelated = NewObject<USceneComponent>(GetTransientPackage());
	TestNull(TEXT("An object that is neither the interface nor a PlayerController should return null"),
		UMounteaDialogueHUDStatics::GetViewportWidget(unrelated));
	return true;
}

// Second call site of the same bug: InitializeViewportWidget's PlayerController fallback called the
// interface event stub directly. Only "does not assert" is checked - whether a widget is created
// depends on project settings and needs a real viewport.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueInitializeViewportWidgetPlayerControllerTest,
	"Mountea.DialogueSystem.HUDStatics.InitializeViewportWidget.PlayerControllerFallbackDoesNotAssert",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueInitializeViewportWidgetPlayerControllerTest::RunTest(const FString& Parameters)
{
	MounteaDialogueHUDStaticsTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	TestNotNull(TEXT("Precondition: the LocalPlayer must resolve a ViewportHUD subsystem"), fixture.Subsystem);

	UMounteaDialogueHUDStatics::InitializeViewportWidget(fixture.PlayerController);
	TestTrue(TEXT("InitializeViewportWidget via the PlayerController fallback should complete without asserting"), true);

	fixture.Teardown(*this);
	return true;
}

// Sanity check for the sibling fallback that already used Execute_ - keeps it from regressing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueGetViewportBaseClassPlayerControllerTest,
	"Mountea.DialogueSystem.HUDStatics.GetViewportBaseClass.PlayerControllerFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueGetViewportBaseClassPlayerControllerTest::RunTest(const FString& Parameters)
{
	MounteaDialogueHUDStaticsTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	TestNotNull(TEXT("Precondition: the LocalPlayer must resolve a ViewportHUD subsystem"), fixture.Subsystem);

	// The value depends on project settings; this only proves the fallback path runs without asserting
	// and agrees with calling the subsystem through the interface.
	const TSubclassOf<UUserWidget> viaStatics = UMounteaDialogueHUDStatics::GetViewportBaseClass(fixture.PlayerController);
	const TSubclassOf<UUserWidget> viaSubsystem = fixture.Subsystem
		? IMounteaDialogueHUDClassInterface::Execute_GetViewportBaseClass(fixture.Subsystem)
		: nullptr;
	TestTrue(TEXT("GetViewportBaseClass via PlayerController should match the subsystem's own result"), viaStatics == viaSubsystem);

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
