// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "BlueprintConnectionDrawingPolicy.h"
#include "ConnectionDrawingPolicy.h"

class FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph : public FKismetConnectionDrawingPolicy
{
public:
	FConnectionDrawingPolicy_AdvancedMounteaDialogueGraph(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	virtual void DrawSplineWithArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params) override;
	virtual void DrawSplineWithArrow(const FGeometry& StartGeom, const FGeometry& EndGeom, const FConnectionParams& Params) override;
	virtual FVector2D ComputeSplineTangent(const FVector2D& Start, const FVector2D& End) const override;

protected:
	void DrawManhattanConnection(const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params);
	void DrawSimpleRadius(const FVector2D& Start, const FVector2D& StartDirection, const int32& AngleDeg, FVector2D& out_End, FVector2D& out_EndDirection);
	void DrawRadius(const FVector2D& Start, const FVector2D& StartDirection, const FVector2D& End, const FVector2D& EndDirection, const int32& AngleDeg);
	void DrawArrow(const FVector2D& StartPoint, const FVector2D& EndPoint, const FConnectionParams& Params);

	float GetRadiusOffset(const int32& AngleDeg, bool Perpendicular = false) const;
	float GetRadiusTangent(const int32& AngleDeg) const;

private:
	int32 StoredBackLayerID;
	int32 StoredFrontLayerID;
	float ZoomFactor;
	float RoundRadius;
	float WireThickness;
};

