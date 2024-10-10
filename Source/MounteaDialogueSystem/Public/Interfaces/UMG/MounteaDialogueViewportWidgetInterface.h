// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Widgets/Layout/Anchors.h"
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

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UI|Viewport")
	void AddChildWidget(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(0.f,0.f,1.f,1.f), const FMargin& WidgetMargin = FMargin());
	virtual void AddChildWidget_Implementation(UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(0.f,0.f,1.f,1.f), const FMargin& WidgetMargin = FMargin()) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UI|Viewport")
	void RemoveChildWidget(UUserWidget* ChildWidget);
	virtual void RemoveChildWidget_Implementation(UUserWidget* ChildWidget) = 0;
};
