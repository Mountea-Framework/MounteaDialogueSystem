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

#include "CoreMinimal.h"
#include "Interfaces/HUD/MounteaDialogueHUDClassInterface.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Helpers/MounteaDialogueHUDStatics.h"
#include "MounteaDialogueViewportHUDSubsystem.generated.h"

class UUserWidget;

UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueViewportHUDSubsystem : public ULocalPlayerSubsystem, public IMounteaDialogueHUDClassInterface
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Removes the wrapper widget from the screen and forgets it, so the next InitializeViewportWidget builds a
	 * fresh one. This subsystem outlives levels, so a wrapper left over from a previous world would otherwise
	 * keep its stale child widgets.
	 */
	void ResetViewportWidget();

	// IMounteaDialogueHUDClassInterface
	virtual TSubclassOf<UUserWidget> GetViewportBaseClass_Implementation() const override;
	virtual void InitializeViewportWidget_Implementation() override;
	virtual UUserWidget* GetViewportWidget_Implementation() const override;
	virtual void AddChildWidgetToViewport_Implementation(UUserWidget* ChildWidget, const int32 ZOrder, const FAnchors WidgetAnchors, const FMargin& WidgetMargin) override;
	virtual void RemoveChildWidgetFromViewport_Implementation(UUserWidget* ChildWidget) override;
	// ~IMounteaDialogueHUDClassInterface

private:
#if WITH_DEV_AUTOMATION_TESTS
	friend struct FMounteaDialogueTestAccess;
#endif

	void HandleWorldBeginTearDown(UWorld* World);

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ViewportWidget = nullptr;

	// World the wrapper was created for. UUserWidget::GetWorld() can follow the player context into the
	// current world, so it cannot be trusted to detect a wrapper left over from a previous one.
	TWeakObjectPtr<UWorld> ViewportWidgetWorld;
};
