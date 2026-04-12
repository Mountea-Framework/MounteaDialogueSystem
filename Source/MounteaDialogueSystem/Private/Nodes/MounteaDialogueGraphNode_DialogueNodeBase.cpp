// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "TimerManager.h"
#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueTraversalStatics.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Misc/DataValidation.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DialogueNodeBase"

UMounteaDialogueGraphNode_DialogueNodeBase::UMounteaDialogueGraphNode_DialogueNodeBase()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseTitle", "Dialogue Node Base");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseInternalTitle", "Dialogue Node Base");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseContextMenu", "Dialogue Node");
	EditorNodeColour = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("3b82f6")));
	EditorHeaderForegroundColour = FLinearColor::White;

	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_BaseTooltip", "* Abstract class, should not appear in Graph Editor.\n* Enhances 'MounteaDialogueGraphNode' Base class with Dialogue data.\n* Provides DataTable and Row options that define the Dialogue data which will be displayed in UI.");
#endif
	
	bAutoStarts = false;
	bUseGameplayTags = true;

	AllowedInputClasses.Add(UMounteaDialogueGraphNode_Delay::StaticClass());
}

void UMounteaDialogueGraphNode_DialogueNodeBase::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (Manager)
	{
		if (UMounteaDialogueContext* Context = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager.GetObject()))
		{
			GetWorld()->GetTimerManager().ClearTimer(Manager->GetDialogueRowTimerHandle());

			const FDialogueRow DialogueRow = UMounteaDialogueTraversalStatics::GetSpeechData(Context->ActiveNode);
			if (UMounteaDialogueTraversalStatics::IsDialogueRowValid(DialogueRow) && DialogueRow.RowData.IsValidIndex(Context->GetActiveDialogueRowDataIndex()))
			{
				Context->UpdateActiveDialogueRow(DialogueRow);
				Context->UpdateActiveDialogueRowDataIndex(Context->ActiveDialogueRowDataIndex);
				Manager->GetDialogueContextUpdatedEventHande().Broadcast(Context);
			}
		}
	}
	
	Super::ProcessNode_Implementation(Manager);
}

void UMounteaDialogueGraphNode_DialogueNodeBase::PreProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (bUseGameplayTags && Manager.GetInterface())
	{
		if (UMounteaDialogueContext* tempContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager.GetObject()))
		{
			const TScriptInterface<IMounteaDialogueParticipantInterface> bestMatchingParticipant = UMounteaDialogueTraversalStatics::ResolveActiveParticipant(tempContext);
			UMounteaDialogueTraversalStatics::UpdateMatchingDialogueParticipant(tempContext, bestMatchingParticipant);
		}
	}

	Super::PreProcessNode_Implementation(Manager);
}

FDialogueRow UMounteaDialogueGraphNode_DialogueNodeBase::GetSpeechData_Implementation() const
{
	if (!IsValid(DataTable))
		return {};
	
	const FString searchContext;
	const auto returnValue = DataTable->FindRow<FDialogueRow>(RowName, searchContext);
	return returnValue != nullptr ? *returnValue : FDialogueRow::Invalid();
}

bool UMounteaDialogueGraphNode_DialogueNodeBase::SetSpeechData_Implementation(const FDialogueRow& NewSpeechData)
{
	LOG_ERROR(TEXT("[SetSpeechData] Dialogue Graph Node doesn't support direct Row override!"))
	return false;
}

UDataTable* UMounteaDialogueGraphNode_DialogueNodeBase::GetDataTable() const
{
	return DataTable;
}

bool UMounteaDialogueGraphNode_DialogueNodeBase::ValidateNodeRuntime_Implementation() const
{
	if (DataTable == nullptr)
		return false;

	if (RowName.IsNone())
		return false;

	if (MaxChildrenNodes > -1 && ChildrenNodes.Num() > MaxChildrenNodes)
		return false;

	const FString Context;
	const FDialogueRow* SelectedRow = DataTable->FindRow<FDialogueRow>(RowName, Context);

	if (SelectedRow == nullptr)
		return false;

	if (SelectedRow && SelectedRow->RowData.Num() == 0)
		return false;
	
	return true;
}

#if WITH_EDITOR

bool UMounteaDialogueGraphNode_DialogueNodeBase::ValidateNode(FDataValidationContext& Context, const bool RichFormat) const
{
	bool bResult = Super::ValidateNode(Context, RichFormat);

	if (DataTable == nullptr)
	{
		bResult = false;

		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Does not contain any Dialogue Data Table!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Does not contain any Dialogue Data Table!");
	
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (RowName.IsNone())
	{
		bResult = false;

		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Does not contain valid Dialogue Row!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Does not contain valid Dialogue Row!");
	
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (MaxChildrenNodes > -1 && ChildrenNodes.Num() > MaxChildrenNodes)
	{
		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Has more than ").
		Append("<RichTextBlock.Bold>").
		Append(FString::FromInt(MaxChildrenNodes)).
		Append("</>").
		Append(" Children Nodes!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Has more than ").Append(FString::FromInt(MaxChildrenNodes)).Append(" Children Nodes!");
	
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	const FString ContextString;
	const FDialogueRow* SelectedRow = DataTable!=nullptr ? DataTable->FindRow<FDialogueRow>(RowName, ContextString) : nullptr;

	if (SelectedRow == nullptr)
	{
		bResult = false;

		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": Invalid Selected Row!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": Invalid Selected Row!");
	
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (SelectedRow)
	{
	if (SelectedRow->RowData.Num() == 0)
		{
			bResult = false;

			const FString RichTextReturn =
			FString("* ").
			Append("<RichTextBlock.Bold>").
			Append(NodeTitle.ToString()).
			Append("</>").
			Append(": Invalid Selected Row! No Dialogue Data Rows inside!");

			const FString TextReturn =
			FString(NodeTitle.ToString()).
			Append(": Invalid Selected Row! No Dialogue Data Rows inside!");
	
			Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
		}
	}
	
	return bResult;
}

void UMounteaDialogueGraphNode_DialogueNodeBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_DialogueNodeBase, DataTable))
	{
		RowName = FName("");
		Preview.Empty();
		PreviewsUpdated.ExecuteIfBound();
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_DialogueNodeBase, RowName))
	{
		UpdatePreviews();
		PreviewsUpdated.ExecuteIfBound();
	}
}

FText UMounteaDialogueGraphNode_DialogueNodeBase::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseDescription", "Dialogue Base Node has no logic tied to itself.");
}

TArray<FText> UMounteaDialogueGraphNode_DialogueNodeBase::GetPreviews() const
{
	TArray<FText> ReturnValues;
	
	const FDialogueRow Row = GetSpeechData_Implementation();
	if (UMounteaDialogueTraversalStatics::IsDialogueRowValid(Row))
	{
		for (const auto& Itr : Row.RowData)
		{
			ReturnValues.Add( Itr.RowText );
		}
	}
	else
		ReturnValues.Empty();

	return ReturnValues;
}

void UMounteaDialogueGraphNode_DialogueNodeBase::UpdatePreviews()
{
	if (!DataTable) Preview.Empty();

	Preview.Empty();

	Preview = GetPreviews();
}

#endif	
#undef LOCTEXT_NAMESPACE

