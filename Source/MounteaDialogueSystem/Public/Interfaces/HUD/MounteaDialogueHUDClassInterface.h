// // All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Widgets/Layout/Anchors.h"
#include "MounteaDialogueHUDClassInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueHUDClassInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueHUDClassInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	TSubclassOf<UUserWidget> GetViewportBaseClass() const;
	virtual TSubclassOf<UUserWidget> GetViewportBaseClass_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void InitializeViewportWidget();
	virtual void InitializeViewportWidget_Implementation() = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void AddChildWidgetToViewport(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(), const FMargin& WidgetMargin = FMargin());
	virtual void AddChildWidgetToViewport_Implementation(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(), const FMargin& WidgetMargin = FMargin()) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void RemoveChildWidgetFromViewport(UUserWidget* ChildWidget);
	virtual void RemoveChildWidgetFromViewport_Implementation(UUserWidget* ChildWidget) = 0;

};
