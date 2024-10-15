// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Nodes/MounteaDialogueGraphNode_Delay.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DelayNode"

UMounteaDialogueGraphNode_Delay::UMounteaDialogueGraphNode_Delay()
	: DelayDuration(3.f)
{
#if WITH_EDITORONLY_DATA
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DelayNodeBaseTitle", "Delay Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_DelayNodeBaseInternalTitle", "Delay Node");
	
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DelayNodeContextMenuName", "Delay Node");
	
	BackgroundColor = FLinearColor(FColor::FromHex("007ddc"));
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_DelayTooltip", "* This Node does nothing but waits until its Timer expires\n* After expiration following node is selected automatically");
#endif
	
	bAutoStarts = true;
}

void UMounteaDialogueGraphNode_Delay::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Super::ProcessNode_Implementation(Manager);

	
}

FText UMounteaDialogueGraphNode_Delay::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DelayNodeDescription", "Waiting Node with no extra logic.\nUseful for cutscenes or transitions.");
}

FText UMounteaDialogueGraphNode_Delay::GetNodeCategory_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DelayNodeCategory", "Utilities");
}

bool UMounteaDialogueGraphNode_Delay::ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat)
{
	bool bSatisfied = Super::ValidateNode(ValidationsMessages, RichFormat);
	bSatisfied = bSatisfied && DelayDuration > 0.0f;

	// TODO: Implement return texts
	
	return bSatisfied;
}

#undef LOCTEXT_NAMESPACE