// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Nodes/MounteaDialogueGraphNode_Delay.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"
#include "TimerManager.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DelayNode"

UMounteaDialogueGraphNode_Delay::UMounteaDialogueGraphNode_Delay()
	: DelayDuration(3.f)
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DelayNodeBaseTitle", "Delay Node");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_DelayNodeBaseInternalTitle", "Delay Node");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DelayNodeContextMenuName", "Delay Node");
	
	BackgroundColor = FLinearColor(FColor::FromHex("007ddc"));
	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_DelayTooltip", "* This Node does nothing but waits until its Timer expires\n* After expiration following node is selected automatically");
	bCanRenameNode = false;
#endif
	
	bAutoStarts = true;
	
	AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());

	MaxChildrenNodes = 1;
}

void UMounteaDialogueGraphNode_Delay::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	Super::ProcessNode_Implementation(Manager);

	if (GetWorld())
	{
		FTimerDelegate TimerDelegate_TypeWriterUpdateInterval;
		TimerDelegate_TypeWriterUpdateInterval.BindUFunction(this, "OnDelayDurationExpired", Manager);
		GetWorld()->GetTimerManager().SetTimer(TimerHandle_NodeDelay, TimerDelegate_TypeWriterUpdateInterval, DelayDuration, false);
	}
	else
	{
		TScriptInterface<IMounteaDialogueManagerInterface> MounteaDialogueManagerInterface = Manager;
		MounteaDialogueManagerInterface->GetDialogueNodeFinishedEventHandle().Broadcast(Manager->Execute_GetDialogueContext(Manager.GetObject()));
	}
}

FText UMounteaDialogueGraphNode_Delay::GetNodeTitle_Implementation() const
{
	return FText::Format(LOCTEXT("MounteaDialogueGraphNode_DelayNodeTitle", "Delay: {0}s"), FText::FromString(FString::FromInt(DelayDuration)));
}

void UMounteaDialogueGraphNode_Delay::OnDelayDurationExpired(const TScriptInterface<IMounteaDialogueManagerInterface>& MounteaDialogueManagerInterface)
{
	if (MounteaDialogueManagerInterface.GetInterface() == nullptr)
	{
		LOG_ERROR(TEXT("[Delay Node Expired] Invalid Dialogue Manager!"))
		return;
	}

	if (!ChildrenNodes.IsValidIndex(0))
	{
		LOG_ERROR(TEXT("[Delay Node Expired] Invalid Child node!"))
		return;
	}
	
	TimerHandle_NodeDelay.Invalidate();

	auto managerObject = MounteaDialogueManagerInterface.GetObject();
	if (const auto Context = MounteaDialogueManagerInterface->Execute_GetDialogueContext(managerObject))
	{
		/*
		auto dialogueNodeToStart = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(ChildrenNodes[0]);
			
		Context->SetDialogueContext(Context->DialogueParticipant, dialogueNodeToStart, UMounteaDialogueSystemBFC::GetAllowedChildNodes(dialogueNodeToStart));

		Context->ActiveDialogueRowDataIndex = 	UMounteaDialogueSystemBFC::GetDialogueRow(dialogueNodeToStart).DialogueRowData.Num() - 1; // Force-set the last row
		FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
		newDialogueTableHandle.DataTable = dialogueNodeToStart->GetDataTable();
		newDialogueTableHandle.RowName = dialogueNodeToStart->GetRowName();
		Context->UpdateActiveDialogueTable(dialogueNodeToStart ? newDialogueTableHandle : FDataTableRowHandle());
		*/
		
		MounteaDialogueManagerInterface->Execute_NodeProcessed(managerObject);
	}
}

#if WITH_EDITOR

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

#endif

#undef LOCTEXT_NAMESPACE