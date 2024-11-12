// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/Layout/Anchors.h"
#include "Layout/Margin.h"
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
	
	/**
	 * Retrieves the base widget class used for the viewport.
	 *
	 * @return                   The subclass of UUserWidget used as the base class for the viewport, or nullptr if an error occurs.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	TSubclassOf<UUserWidget> GetViewportBaseClass() const;
	virtual TSubclassOf<UUserWidget> GetViewportBaseClass_Implementation() const = 0;

	/**
	 * Creates the viewport widget.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void InitializeViewportWidget();
	virtual void InitializeViewportWidget_Implementation() = 0;

	/**
	 * Retrieves the viewport widget.
	 *
	 * @return                   The UUserWidget representing the viewport, or nullptr if the viewport manager does not implement the interface or an error occurs.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	class UUserWidget* GetViewportWidget() const;
	virtual UUserWidget* GetViewportWidget_Implementation() const = 0;

	/**
	 * Adds a child widget to the viewport.
	 *
	 * @param ChildWidget        The child widget to be added to the viewport.
	 * @param ZOrder The Z-order index of the child widget, determining its rendering order within the parent
	 * @param WidgetAnchors The anchors for the widget, determining how the widget is positioned relative to its parent
	 * @param WidgetMargin The margin for the widget, defining the padding or offset from the parent's bounds
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void AddChildWidgetToViewport(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(0.f,0.f,1.f,1.f), const FMargin& WidgetMargin = FMargin());
	virtual void AddChildWidgetToViewport_Implementation(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(0.f,0.f,1.f,1.f), const FMargin& WidgetMargin = FMargin()) = 0;

	/**
	 * Removes a child widget from the viewport.
	 *
	 * @param ChildWidget        The child widget to be removed from the viewport.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void RemoveChildWidgetFromViewport(UUserWidget* ChildWidget);
	virtual void RemoveChildWidgetFromViewport_Implementation(UUserWidget* ChildWidget) = 0;

};
