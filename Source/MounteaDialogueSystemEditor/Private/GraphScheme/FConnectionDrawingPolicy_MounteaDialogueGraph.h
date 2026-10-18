// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EngineVersionComparison.h"
#include "BlueprintConnectionDrawingPolicy.h"
#include "ConnectionDrawingPolicy.h"

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
	// FVector2f is the engine's standard from 5.7 on; the FVector2D virtuals only exist (deprecated) for older engines.
	virtual void DrawSplineWithArrow(const FVector2f& StartPoint, const FVector2f& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawConnection(int32 LayerId, const FVector2f& Start, const FVector2f& End, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2f& StartPoint, const FVector2f& EndPoint, UEdGraphPin* Pin) override;
	virtual FVector2f ComputeSplineTangent(const FVector2f& Start, const FVector2f& End) const override;
#if UE_VERSION_OLDER_THAN(5, 7, 0)
	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override;
#endif
	virtual bool IsConnectionCulled(const FArrangedWidget& StartLink, const FArrangedWidget& EndLink) const override;
	virtual void DetermineLinkGeometry(FArrangedChildren& ArrangedNodes, TSharedRef<SWidget>& OutputPinWidget, UEdGraphPin* OutputPin, UEdGraphPin* InputPin, FArrangedWidget*& StartWidgetGeometry, FArrangedWidget*& EndWidgetGeometry) override;
	// End of FConnectionDrawingPolicy interface
	
protected:
	/** Default wire between two points (no obstacle knowledge). */
	void DrawSubwayWireWithArrow(const FVector2f& StartPoint, const FVector2f& EndPoint, const FConnectionParams& Params);
	/** The single bounded renderer: validates, rounds, clips and draws a path plus arrow and animation. */
	void DrawWirePath(TArray<FVector2D>& Corners, const FConnectionParams& Params);
	/** Draws the route calculated by the layered layout when one is still valid. */
	bool TryDrawPreparedRoute(const FGeometry& StartGeom, const FConnectionParams& Params);
	/**
	 * Slate gives culled (off-screen) nodes a synthesized geometry: zero size, scale 1, correct top-left only.
	 * Rebuilds the real bottom/top centre anchors of a node-to-node link from the node sizes and the zoom factor.
	 * Returns false for links that are not a node-to-node link (the control -> node stub).
	 */
	bool ResolveCulledAnchors(const FConnectionParams& Params, FVector2f& OutStart, FVector2f& OutEnd) const;

	UEdGraph* GraphObj;
	TMap<UEdGraphNode*, int32> NodeWidgetMap;
	TSet<UEdGraphNode*> SelectedEdgeNodes;
	const FArrangedChildren* CachedArrangedNodes = nullptr;
	
};
