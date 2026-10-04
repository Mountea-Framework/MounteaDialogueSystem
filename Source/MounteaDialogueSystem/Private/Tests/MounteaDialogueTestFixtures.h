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

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GenericPlatform/GenericPlatformInputDeviceMapper.h"
#include "Subsystem/MounteaDialogueViewportHUDSubsystem.h"
#include "Subsystem/MounteaDialogueLocalPlayerSubsystem.h"

// Shared across the Tests/ folder. A real UWorld (FTestWorldWrapper) plus a ULocalPlayer registered via
// PlayerAdded() is the minimum needed for APlayerController::GetLocalPlayer()->GetSubsystem<>() to
// resolve to a live subsystem. Widget creation / viewport attachment needing a real viewport is
// deliberately not exercised by tests built on this fixture.
namespace MounteaDialogueTest
{
	// Owns the test world, a PlayerController, and a registered ULocalPlayer for the duration of a test.
	struct FPlayerControllerFixture
	{
		FTestWorldWrapper WorldWrapper;
		APlayerController* PlayerController = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
		UMounteaDialogueViewportHUDSubsystem* Subsystem = nullptr;

		UWorld* GetWorld() const { return WorldWrapper.GetTestWorld(); }

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

// Grants the Tests/ folder access to the HUD subsystems' private wrapper state (see the friend declarations),
// so a test can seed "a wrapper left over from a previous world" without driving real widget creation.
// Same pattern as FMounteaInteractionTestAccess in MounteaInteractionSystem.
struct FMounteaDialogueTestAccess
{
	static void SeedViewportWidget(UMounteaDialogueViewportHUDSubsystem* Subsystem, UUserWidget* Widget, UWorld* WrapperWorld)
	{
		Subsystem->ViewportWidget = Widget;
		Subsystem->ViewportWidgetWorld = WrapperWorld;
	}

	static void SeedViewportWidget(UMounteaDialogueLocalPlayerSubsystem* Subsystem, UUserWidget* Widget, UWorld* WrapperWorld)
	{
		Subsystem->ViewportWidget = Widget;
		Subsystem->ViewportWidgetWorld = WrapperWorld;
	}
};

#endif // WITH_DEV_AUTOMATION_TESTS
