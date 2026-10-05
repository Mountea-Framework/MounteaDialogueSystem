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

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR // editor-only: needs an editor world, kept out of game builds

#include "Tests/MounteaDialogueTestFixtures.h"
#include "Components/SceneComponent.h"
#include "WBP/MounteaDialogueSkip.h"
#include "Components/MounteaDialogueParticipantUserInterfaceComponent.h"
#include "Helpers/MounteaDialogueHUDStatics.h"
#include "Interfaces/HUD/MounteaDialogueWBPInterface.h"
#include "Interfaces/Core/MounteaDialogueParticipantUIInterface.h"

// Tests in this file cover dialogue widget lifetime across level changes (Bugreport #2): the participant UI
// component must close its widget when it ends play, and the viewport HUD subsystem (which outlives levels
// as a ULocalPlayerSubsystem) must not keep reusing a wrapper widget left over from a dead world.
//
// World / LocalPlayer setup lives in MounteaDialogueTestFixtures.h. Widget creation / viewport attachment
// is deliberately NOT exercised here - that needs a real viewport - so these tests seed a plain concrete
// UObject (UUserWidget itself is abstract) and assert on the component/subsystem's own state.

namespace MounteaDialogueWidgetLifecycleTest
{
	// SetUserInterface accepts any UObject. A plain concrete object does NOT implement
	// IMounteaDialogueWBPInterface, which is exactly what makes it useful - it proves the component copes
	// with whatever object it was given (and, not being a UUserWidget, skips the viewport removal step).
	static UObject* CreatePlainUserInterface(UWorld* World)
	{
		return NewObject<USceneComponent>(World);
	}

	// A concrete (non-abstract) UUserWidget to stand in for the HUD wrapper: UUserWidget itself is abstract and
	// no Blueprint wrapper class is available to a unit test. UMounteaDialogueSkip is native and concrete.
	static UUserWidget* CreateWrapperStandIn(UWorld* World)
	{
		return NewObject<UMounteaDialogueSkip>(World);
	}

	// Attaches a participant UI component to the fixture's PlayerController (so the owner chain resolves to
	// a local PlayerController) and seeds it with a UserInterface.
	static UMounteaDialogueParticipantUserInterfaceComponent* AddComponentWithUI(MounteaDialogueTest::FPlayerControllerFixture& Fixture, UObject* UserInterface)
	{
		UMounteaDialogueParticipantUserInterfaceComponent* component = NewObject<UMounteaDialogueParticipantUserInterfaceComponent>(Fixture.PlayerController);
		Fixture.PlayerController->AddInstanceComponent(component);
		component->RegisterComponent();
		IMounteaDialogueParticipantUIInterface::Execute_SetUserInterface(component, UserInterface);
		return component;
	}
}

// Direct regression for Bugreport #2: EndPlay used to only unbind from the manager, leaving the dialogue
// widget parented to the (level-surviving) viewport wrapper. Ending play (as OpenLevel does, with
// LevelTransition) must close the dialogue UI and release the widget.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueComponentEndPlayClosesUITest,
	"Mountea.DialogueSystem.WidgetLifecycle.Component.EndPlayClosesUI",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueComponentEndPlayClosesUITest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWidgetLifecycleTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	UObject* userInterface = CreatePlainUserInterface(fixture.GetWorld());
	UMounteaDialogueParticipantUserInterfaceComponent* component = AddComponentWithUI(fixture, userInterface);

	TestNotNull(TEXT("Precondition: component should hold the seeded UserInterface"),
		IMounteaDialogueParticipantUIInterface::Execute_GetUserInterface(component));

	// EndPlay with the reason OpenLevel produces. Not routed through Destroy(): that marks the component
	// garbage, after which reading its state back is meaningless. EndPlay is public on UActorComponent but
	// protected on the derived component, hence the base-pointer call.
	static_cast<UActorComponent*>(component)->EndPlay(EEndPlayReason::LevelTransition);

	TestNull(TEXT("Ending play must close the dialogue UI and drop the component's reference to it"),
		IMounteaDialogueParticipantUIInterface::Execute_GetUserInterface(component));

	fixture.Teardown(*this);
	return true;
}

// CloseDialogueUI now runs from EndPlay with whatever object SetUserInterface was given. The Execute_ helpers
// check() that the target implements the interface, so a non-implementing widget used to be a hard assert.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueComponentCloseUIToleratesNonInterfaceUserInterfaceTest,
	"Mountea.DialogueSystem.WidgetLifecycle.Component.CloseUIToleratesNonInterfaceUserInterface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueComponentCloseUIToleratesNonInterfaceUserInterfaceTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWidgetLifecycleTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	UObject* userInterface = CreatePlainUserInterface(fixture.GetWorld());
	TestFalse(TEXT("Precondition: the plain object must not implement the WBP interface"), userInterface->Implements<UMounteaDialogueWBPInterface>());

	UMounteaDialogueParticipantUserInterfaceComponent* component = AddComponentWithUI(fixture, userInterface);

	const bool bClosed = IMounteaDialogueParticipantUIInterface::Execute_CloseDialogueUI(component);
	TestTrue(TEXT("CloseDialogueUI should report success for a valid UserInterface"), bClosed);
	TestNull(TEXT("CloseDialogueUI should clear the UserInterface"), IMounteaDialogueParticipantUIInterface::Execute_GetUserInterface(component));

	fixture.Teardown(*this);
	return true;
}

// The HUD subsystem outlives levels, so its wrapper (and the stale dialogue child inside it) must be
// droppable on demand: after ResetViewportWidget the next InitializeViewportWidget starts from scratch.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemResetViewportWidgetTest,
	"Mountea.DialogueSystem.WidgetLifecycle.Subsystem.ResetViewportWidgetClearsWrapper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemResetViewportWidgetTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWidgetLifecycleTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
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

	UUserWidget* wrapper = CreateWrapperStandIn(fixture.GetWorld());
	FMounteaDialogueTestAccess::SeedViewportWidget(fixture.Subsystem, wrapper, fixture.GetWorld());
	TestTrue(TEXT("Precondition: the seeded wrapper should be reported by the subsystem"), fixture.Subsystem->GetViewportWidget_Implementation() == wrapper);

	fixture.Subsystem->ResetViewportWidget();

	TestNull(TEXT("ResetViewportWidget must drop the wrapper"), fixture.Subsystem->GetViewportWidget_Implementation());

	fixture.Teardown(*this);
	return true;
}

// Level change: when the wrapper's world begins tearing down, the subsystem must let go of the wrapper so the
// next dialogue gets a fresh one instead of the old one with its stale children. Tearing down some OTHER
// world (preview / PIE / streaming helpers) must leave the wrapper alone.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemWorldTearDownTest,
	"Mountea.DialogueSystem.WidgetLifecycle.Subsystem.WorldTearDownDropsWrapper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemWorldTearDownTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWidgetLifecycleTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
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

	UUserWidget* wrapper = CreateWrapperStandIn(fixture.GetWorld());
	FMounteaDialogueTestAccess::SeedViewportWidget(fixture.Subsystem, wrapper, fixture.GetWorld());

	// A throwaway world that is not the wrapper's.
	UWorld* unrelatedWorld = UWorld::CreateWorld(EWorldType::None, false, TEXT("MounteaDialogueUnrelatedTestWorld"));
	if (!TestNotNull(TEXT("Precondition: an unrelated world should be creatable"), unrelatedWorld))
	{
		fixture.Teardown(*this);
		return false;
	}

	FWorldDelegates::OnWorldBeginTearDown.Broadcast(unrelatedWorld);
	TestTrue(TEXT("Tearing down an unrelated world must keep the wrapper"), fixture.Subsystem->GetViewportWidget_Implementation() == wrapper);

	unrelatedWorld->DestroyWorld(false);

	FWorldDelegates::OnWorldBeginTearDown.Broadcast(fixture.GetWorld());
	TestNull(TEXT("Tearing down the wrapper's world must drop the wrapper"), fixture.Subsystem->GetViewportWidget_Implementation());

	fixture.Teardown(*this);
	return true;
}

// Safety net behind the teardown delegate: if the wrapper belongs to a world other than the subsystem's
// current one, InitializeViewportWidget must not keep reusing it.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueSubsystemInitializeReplacesStaleWrapperTest,
	"Mountea.DialogueSystem.WidgetLifecycle.Subsystem.InitializeReplacesStaleWrapper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueSubsystemInitializeReplacesStaleWrapperTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWidgetLifecycleTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
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

	// The fixture's LocalPlayer has no viewport client, so the subsystem's GetWorld() is null. A wrapper
	// recorded against the (non-null) fixture world is therefore "from another world".
	TestNull(TEXT("Precondition: the subsystem has no current world in this fixture"), fixture.Subsystem->GetWorld());
	UUserWidget* staleWrapper = CreateWrapperStandIn(fixture.GetWorld());
	FMounteaDialogueTestAccess::SeedViewportWidget(fixture.Subsystem, staleWrapper, fixture.GetWorld());

	// Whether a fresh wrapper gets created depends on the project's configured wrapper class; either way the
	// stale instance must not survive.
	UMounteaDialogueHUDStatics::InitializeViewportWidget(fixture.Subsystem);

	TestTrue(TEXT("InitializeViewportWidget must not keep reusing a wrapper from another world"),
		fixture.Subsystem->GetViewportWidget_Implementation() != staleWrapper);

	fixture.Teardown(*this);
	return true;
}

// Same teardown behaviour on the sibling subsystem, which carries a copy of the wrapper logic.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMounteaDialogueLocalPlayerSubsystemWorldTearDownTest,
	"Mountea.DialogueSystem.WidgetLifecycle.LocalPlayerSubsystem.WorldTearDownDropsWrapper",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMounteaDialogueLocalPlayerSubsystemWorldTearDownTest::RunTest(const FString& Parameters)
{
	using namespace MounteaDialogueWidgetLifecycleTest;

	MounteaDialogueTest::FPlayerControllerFixture fixture;
	if (!fixture.Setup(*this))
	{
		fixture.Teardown(*this);
		return false;
	}

	UMounteaDialogueLocalPlayerSubsystem* subsystem = fixture.LocalPlayer->GetSubsystem<UMounteaDialogueLocalPlayerSubsystem>();
	if (!TestNotNull(TEXT("Precondition: the LocalPlayer must resolve a LocalPlayer HUD subsystem"), subsystem))
	{
		fixture.Teardown(*this);
		return false;
	}

	UUserWidget* wrapper = CreateWrapperStandIn(fixture.GetWorld());
	FMounteaDialogueTestAccess::SeedViewportWidget(subsystem, wrapper, fixture.GetWorld());

	FWorldDelegates::OnWorldBeginTearDown.Broadcast(fixture.GetWorld());
	TestNull(TEXT("Tearing down the wrapper's world must drop the wrapper"), subsystem->GetViewportWidget_Implementation());

	fixture.Teardown(*this);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
