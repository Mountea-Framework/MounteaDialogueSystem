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

#include "Subsystem/MounteaDialogueLocalPlayerSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Interfaces/UMG/MounteaDialogueViewportWidgetInterface.h"
#include "Settings/MounteaDialogueSystemSettings.h"

void UMounteaDialogueLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UMounteaDialogueLocalPlayerSubsystem::HandleWorldBeginTearDown);
}

void UMounteaDialogueLocalPlayerSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);

	ResetViewportWidget();
	ViewportBaseClass = nullptr;

	Super::Deinitialize();
}

void UMounteaDialogueLocalPlayerSubsystem::ResetViewportWidget()
{
	if (IsValid(ViewportWidget))
		ViewportWidget->RemoveFromParent();

	ViewportWidget = nullptr;
	ViewportWidgetWorld.Reset();
}

void UMounteaDialogueLocalPlayerSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
	if (!World || !IsValid(ViewportWidget))
		return;

	// Drop the wrapper only when its own world (or the subsystem's current one) is going away.
	// A wrapper whose world is already gone also counts as dead.
	const UWorld* wrapperWorld = ViewportWidgetWorld.Get();
	if (!wrapperWorld || wrapperWorld == World || GetWorld() == World)
		ResetViewportWidget();
}

void UMounteaDialogueLocalPlayerSubsystem::SetViewportBaseClass(TSubclassOf<UUserWidget> NewViewportBaseClass)
{
	ViewportBaseClass = NewViewportBaseClass;
}

TSubclassOf<UUserWidget> UMounteaDialogueLocalPlayerSubsystem::GetViewportBaseClassSafe() const
{
	return ViewportBaseClass;
}

void UMounteaDialogueLocalPlayerSubsystem::AttachWidget(UUserWidget* Widget, const FWidgetAdditionParams WidgetParams)
{
	AddChildWidgetToViewport_Implementation(Widget, WidgetParams.ZOrder, WidgetParams.Anchors, WidgetParams.Margin);
}

void UMounteaDialogueLocalPlayerSubsystem::RemoveWidget(UUserWidget* Widget)
{
	RemoveChildWidgetFromViewport_Implementation(Widget);
}

TSubclassOf<UUserWidget> UMounteaDialogueLocalPlayerSubsystem::GetViewportBaseClass_Implementation() const
{
	return ViewportBaseClass;
}

void UMounteaDialogueLocalPlayerSubsystem::InitializeViewportWidget_Implementation()
{
	// Reuse the wrapper only while it still belongs to the current world; this subsystem outlives levels,
	// so a wrapper from a previous world would otherwise bring its stale child widgets along.
	if (IsValid(ViewportWidget))
	{
		if (ViewportWidgetWorld.Get() == GetWorld())
			return;

		ResetViewportWidget();
	}

	if (!ViewportBaseClass)
	{
		const auto settings = GetDefault<UMounteaDialogueSystemSettings>();
		if (!IsValid(settings))
			return;
		const auto config =settings->GetDialogueConfiguration().LoadSynchronous();
		if (!IsValid(config))
			return;
		ViewportBaseClass = config->DefaultDialogueWrapperWidgetClass.LoadSynchronous();	
	}
	
	ULocalPlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer || !localPlayer->GetPlayerController(GetWorld()))
		return;

	ViewportWidget = CreateWidget<UUserWidget>(localPlayer->GetPlayerController(GetWorld()), ViewportBaseClass);
	if (!IsValid(ViewportWidget))
		return;

	ViewportWidgetWorld = GetWorld();
	ViewportWidget->AddToPlayerScreen();
}

UUserWidget* UMounteaDialogueLocalPlayerSubsystem::GetViewportWidget_Implementation() const
{
	return ViewportWidget;
}

void UMounteaDialogueLocalPlayerSubsystem::AddChildWidgetToViewport_Implementation(UUserWidget* ChildWidget, const int32 ZOrder, const FAnchors WidgetAnchors, const FMargin& WidgetMargin)
{
	if (!IsValid(ChildWidget))
		return;

	InitializeViewportWidget_Implementation();

	if (IsValid(ViewportWidget) && ViewportWidget->Implements<UMounteaDialogueViewportWidgetInterface>())
	{
		IMounteaDialogueViewportWidgetInterface::Execute_AddChildWidget(ViewportWidget, ChildWidget, ZOrder, WidgetAnchors, WidgetMargin);
		return;
	}

	ChildWidget->AddToPlayerScreen(ZOrder);
}

void UMounteaDialogueLocalPlayerSubsystem::RemoveChildWidgetFromViewport_Implementation(UUserWidget* ChildWidget)
{
	if (!IsValid(ChildWidget))
		return;

	if (IsValid(ViewportWidget) && ViewportWidget->Implements<UMounteaDialogueViewportWidgetInterface>())
	{
		IMounteaDialogueViewportWidgetInterface::Execute_RemoveChildWidget(ViewportWidget, ChildWidget);
		return;
	}

	ChildWidget->RemoveFromParent();
}
