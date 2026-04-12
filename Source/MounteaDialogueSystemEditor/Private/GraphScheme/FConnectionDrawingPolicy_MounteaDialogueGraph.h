// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintConnectionDrawingPolicy.h"
#include "ConnectionDrawingPolicy.h"

class FMDSPathDrawer;
class FArrangedChildren;

class FConnectionDrawingPolicy_MounteaDialogueGraph : public FKismetConnectionDrawingPolicy
{

public:
	FConnectionDrawingPolicy_MounteaDialogueGraph(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);
	virtual ~FConnectionDrawingPolicy_MounteaDialogueGraph() {};
	
	// FConnectionDrawingPolicy interface 
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override;
	virtual void DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry) override;
	// End of FConnectionDrawingPolicy interface
	
protected:
	void DrawSubwayWireWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params);

	UEdGraph* GraphObj;
	TMap<UEdGraphNode*, int32> NodeWidgetMap;
	TSet<UEdGraphNode*> SelectedEdgeNodes;
	const FArrangedChildren* CachedArrangedNodes = nullptr;
	
};
