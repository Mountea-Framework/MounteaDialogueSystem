// All rights reserved Dominik Pavlicek 2023


#include "Decorators/MounteaDialogueDecorator_OverrideDialogue.h"

#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueDecorator_OverrideDialogue"

void UMounteaDialogueDecorator_OverrideDialogue::CleanupDecorator_Implementation()
{
	Super::CleanupDecorator_Implementation();

	Context = nullptr;
}

bool UMounteaDialogueDecorator_OverrideDialogue::ValidateDecorator_Implementation(TArray<FText>& ValidationMessages)
{
	bool bSatisfied = Super::ValidateDecorator_Implementation(ValidationMessages);
	const FText Name = GetDecoratorName();
	const UMounteaDialogueGraphNode* owningNode = GetOwningNode();
	
	if (DataTable == nullptr)
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_DT", "Decorator {0} has no Data Table!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	if (RowName.IsNone() || RowName.IsNone())
	{
		const FText TempText = FText::Format(LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_DT", "Decorator {0}: Invalid Row Name!"), Name);
		ValidationMessages.Add(TempText);
		
		bSatisfied = false;
	}
	
	if (owningNode && owningNode->IsA(UMounteaDialogueGraphNode_StartNode::StaticClass()))
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(
			LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_StartNode",
				"Decorator {0}: is not allowed for Start Nodes!\nAttach this decorator to subsequent nodes instead."),
				Name);
		ValidationMessages.Add(TempText);
	}

	if (owningNode && owningNode->IsA(UMounteaDialogueGraphNode_ReturnToNode::StaticClass()))
	{
		bSatisfied = false;
		
		const FText TempText = FText::Format(
			LOCTEXT("MounteaDialogueDecorator_OverrideDialogue_Validation_ReturnNode",
				"Decorator {0}: is not allowed for Start Nodes!\nAttach this decorator to different nodes instead."),
				Name);
		ValidationMessages.Add(TempText);
	}
	
	return bSatisfied;
}

void UMounteaDialogueDecorator_OverrideDialogue::ExecuteDecorator_Implementation()
{
	Super::ExecuteDecorator_Implementation();

	if (!OwningManager) return;

	if (const auto TempContext = GetContext())
	{
		// We assume Context and Manager are already valid, but safety is safety
		if (!UMounteaDialogueSystemBFC::IsContextValid(TempContext) ) return;

		const auto NewRow = UMounteaDialogueSystemBFC::FindDialogueRow(DataTable, RowName);

		FDataTableRowHandle newDialogueTableHandle = FDataTableRowHandle();
		newDialogueTableHandle.DataTable = DataTable;
		newDialogueTableHandle.RowName = RowName;
		
		TempContext->UpdateActiveDialogueTable(newDialogueTableHandle);
		TempContext->UpdateActiveDialogueRow( NewRow );
	}
}

#undef LOCTEXT_NAMESPACE