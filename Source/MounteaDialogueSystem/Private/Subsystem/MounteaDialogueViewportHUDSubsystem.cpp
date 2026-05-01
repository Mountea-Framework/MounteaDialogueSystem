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

#include "Subsystem/MounteaDialogueViewportHUDSubsystem.h"


#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Interfaces/UMG/MounteaDialogueViewportWidgetInterface.h"
#include "Settings/MounteaDialogueSystemSettings.h"

void UMounteaDialogueViewportHUDSubsystem::Deinitialize()
{
	ViewportWidget = nullptr;

	Super::Deinitialize();
}

TSubclassOf<UUserWidget> UMounteaDialogueViewportHUDSubsystem::GetViewportBaseClass_Implementation() const
{
	TSubclassOf<UUserWidget> viewportBaseClass = nullptr;
	const auto settings = GetDefault<UMounteaDialogueSystemSettings>();
	if (!IsValid(settings))
		return viewportBaseClass;
	const auto config =settings->GetDialogueConfiguration().LoadSynchronous();
	if (!IsValid(config))
		return viewportBaseClass;
	viewportBaseClass = config->DefaultDialogueWrapperWidgetClass.LoadSynchronous();	
	return viewportBaseClass;
}

void UMounteaDialogueViewportHUDSubsystem::InitializeViewportWidget_Implementation()
{
	if (IsValid(ViewportWidget))
		return;

	TSubclassOf<UUserWidget> viewportBaseClass = nullptr;
	const auto settings = GetDefault<UMounteaDialogueSystemSettings>();
	if (!IsValid(settings))
		return;
	const auto config =settings->GetDialogueConfiguration().LoadSynchronous();
	if (!IsValid(config))
		return;
	viewportBaseClass = config->DefaultDialogueWrapperWidgetClass.LoadSynchronous();	
	
	ULocalPlayer* localPlayer = GetLocalPlayer();
	if (!localPlayer || !localPlayer->GetPlayerController(GetWorld()))
		return;

	ViewportWidget = CreateWidget<UUserWidget>(localPlayer->GetPlayerController(GetWorld()), viewportBaseClass);
	if (!IsValid(ViewportWidget))
		return;

	ViewportWidget->AddToPlayerScreen();
}

UUserWidget* UMounteaDialogueViewportHUDSubsystem::GetViewportWidget_Implementation() const
{
	return ViewportWidget;
}

void UMounteaDialogueViewportHUDSubsystem::AddChildWidgetToViewport_Implementation(UUserWidget* ChildWidget, const int32 ZOrder, const FAnchors WidgetAnchors, const FMargin& WidgetMargin)
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

void UMounteaDialogueViewportHUDSubsystem::RemoveChildWidgetFromViewport_Implementation(UUserWidget* ChildWidget)
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
