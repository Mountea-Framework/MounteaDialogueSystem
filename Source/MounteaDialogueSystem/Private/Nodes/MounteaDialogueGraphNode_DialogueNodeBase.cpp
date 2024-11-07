// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "TimerManager.h"
#include "Data/MounteaDialogueContext.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DialogueNodeBase"

UMounteaDialogueGraphNode_DialogueNodeBase::UMounteaDialogueGraphNode_DialogueNodeBase()
{
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseTitle", "Dialogue Node Base");
	NodeTypeName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseInternalTitle", "Dialogue Node Base");
#if WITH_EDITORONLY_DATA
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseContextMenu", "Dialogue Node");
	BackgroundColor = FLinearColor(FColor::Orange);

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
		if (UMounteaDialogueContext* Context = Manager->Execute_GetDialogueContext(Manager.GetObject()))
		{
			GetWorld()->GetTimerManager().ClearTimer(Manager->GetDialogueRowTimerHandle());

			const FDialogueRow DialogueRow = UMounteaDialogueSystemBFC::GetDialogueRow(Context->ActiveNode);
			if (UMounteaDialogueSystemBFC::IsDialogueRowValid(DialogueRow) && DialogueRow.DialogueRowData.Array().IsValidIndex(Context->GetActiveDialogueRowDataIndex()))
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
	if (bUseGameplayTags)
	{
		// Switch Participants based on Tags
		if (Manager.GetInterface())
		{
			if (const auto TempContext = Manager->Execute_GetDialogueContext(Manager.GetObject()))
			{
				const TScriptInterface<IMounteaDialogueParticipantInterface> BestMatchingParticipant = UMounteaDialogueSystemBFC::SwitchActiveParticipant(TempContext);
				UMounteaDialogueSystemBFC::SetActiveDialogueParticipant(TempContext, BestMatchingParticipant);
			}
		}
	}

	Super::PreProcessNode_Implementation(Manager);
}

UDataTable* UMounteaDialogueGraphNode_DialogueNodeBase::GetDataTable() const
{
	return DataTable;
}

bool UMounteaDialogueGraphNode_DialogueNodeBase::ValidateNodeRuntime_Implementation() const
{
	if (DataTable == nullptr)
	{
		return false;
	}

	if (RowName.IsNone())
	{
		return false;
	}

	if (MaxChildrenNodes > -1 && ChildrenNodes.Num() > MaxChildrenNodes)
	{
		return false;
	}

	const FString Context;
	const FDialogueRow* SelectedRow = DataTable->FindRow<FDialogueRow>(RowName, Context);

	if (SelectedRow == nullptr)
	{
		return false;
	}

	if (SelectedRow)
	{
		if (SelectedRow->DialogueRowData.Num() == 0)
		{
			return false;
		}
	}
	
	return true;
}

#if WITH_EDITOR

bool UMounteaDialogueGraphNode_DialogueNodeBase::ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat)
{
	bool bResult = Super::ValidateNode(ValidationsMessages, RichFormat);

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
	
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
	
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
	
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	const FString Context;
	const FDialogueRow* SelectedRow = DataTable!=nullptr ? DataTable->FindRow<FDialogueRow>(RowName, Context) : nullptr;

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
	
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (SelectedRow)
	{
		if (SelectedRow->DialogueRowData.Num() == 0)
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
	
			ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
	
	const auto Row = UMounteaDialogueSystemBFC::GetDialogueRow( this );
	if (UMounteaDialogueSystemBFC::IsDialogueRowValid(Row))
	{
		for (auto Itr : Row.DialogueRowData.Array())
		{
			ReturnValues.Add( Itr.RowText );
		}
	}
	else
	{
		ReturnValues.Empty();
	}

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

