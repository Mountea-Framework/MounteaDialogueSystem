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

	virtual void Deinitialize() override;
	
	// IMounteaDialogueHUDClassInterface
	virtual TSubclassOf<UUserWidget> GetViewportBaseClass_Implementation() const override;
	virtual void InitializeViewportWidget_Implementation() override;
	virtual UUserWidget* GetViewportWidget_Implementation() const override;
	virtual void AddChildWidgetToViewport_Implementation(UUserWidget* ChildWidget, const int32 ZOrder, const FAnchors WidgetAnchors, const FMargin& WidgetMargin) override;
	virtual void RemoveChildWidgetFromViewport_Implementation(UUserWidget* ChildWidget) override;
	// ~IMounteaDialogueHUDClassInterface

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ViewportWidget = nullptr;
};
