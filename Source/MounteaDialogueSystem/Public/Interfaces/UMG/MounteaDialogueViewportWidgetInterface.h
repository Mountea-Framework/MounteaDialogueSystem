// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Widgets/Layout/Anchors.h"
#include "Layout/Margin.h"
#include "MounteaDialogueViewportWidgetInterface.generated.h"

class UUserWidget;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueViewportWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueViewportWidgetInterface
{
	GENERATED_BODY()

	
public:

	/**
	 * Adds a child widget to the parent widget.
	 *
	 * @param ChildWidget     The child widget to be added.
	 * @param ZOrder The Z-order index of the child widget, determining its rendering order within the parent
	 * @param WidgetAnchors The anchors for the widget, determining how the widget is positioned relative to its parent
	 * @param WidgetMargin The margin for the widget, defining the padding or offset from the parent's bounds
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void AddChildWidget(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(0.f,0.f,1.f,1.f), const FMargin& WidgetMargin = FMargin());
	virtual void AddChildWidget_Implementation(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(0.f,0.f,1.f,1.f), const FMargin& WidgetMargin = FMargin()) = 0;

	/**
	 * Removes a child widget from the parent widget.
	 *
	 * @param ChildWidget     The child widget to be removed.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|HUD|Viewport")
	void RemoveChildWidget(UUserWidget* ChildWidget);
	virtual void RemoveChildWidget_Implementation(UUserWidget* ChildWidget) = 0;
};
