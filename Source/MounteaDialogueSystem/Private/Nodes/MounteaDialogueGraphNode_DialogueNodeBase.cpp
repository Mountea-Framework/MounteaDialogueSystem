// All rights reserved Dominik Pavlicek 2023


#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"

#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueGraphNode_DialogueNodeBase"

UMounteaDialogueGraphNode_DialogueNodeBase::UMounteaDialogueGraphNode_DialogueNodeBase()
{
#if WITH_EDITORONLY_DATA
	NodeTitle = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseTitle", "Dialogue Node Base");
	InternalName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseInternalTitle", "Dialogue Node Base");
	ContextMenuName = LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseContextMenu", "Dialogue Node");
	BackgroundColor = FLinearColor(FColor::Orange);

	NodeTooltipText = LOCTEXT("MounteaDialogueGraphNode_BaseTooltip", "* Abstract class, should not appear in Graph Editor.\n* Enhances 'MounteaDialogueGraphNode' Base class with Dialogue data.\n* Provides DataTable and Row options that define the Dialogue data which will be displayed in UI.");
#endif
	
	bAutoStarts = false;
}

FText UMounteaDialogueGraphNode_DialogueNodeBase::GetDescription_Implementation() const
{
	return LOCTEXT("MounteaDialogueGraphNode_DialogueNodeBaseDescription", "Dialogue Base Node has no logic tied to itself.");
}

UDataTable* UMounteaDialogueGraphNode_DialogueNodeBase::GetDataTable() const
{
	return DataTable;
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

	return bResult;
}

bool UMounteaDialogueGraphNode_DialogueNodeBase::CanCreateConnection(UMounteaDialogueGraphNode* Other, EEdGraphPinDirection Direction, FText& ErrorMessage)
{
	if (Other == nullptr)
	{
		ErrorMessage = FText::FromString("Invalid Other Node!");
	}

	if (Other->GetMaxChildNodes() > -1 && Other->ChildrenNodes.Num() >= Other->GetMaxChildNodes())
	{
		const FString TextReturn =
		FString(Other->GetNodeTitle().ToString()).
		Append(": Cannot have more than ").Append(FString::FromInt(Other->GetMaxChildNodes())).Append(" Children Nodes!");

		ErrorMessage = FText::FromString(TextReturn);
		return false;
	}

	if (Direction == EGPD_Output)
	{
		
		// Fast checking for native classes
		if ( AllowedInputClasses.Contains(Other->GetClass()) )
		{
			return true;
		}

		// Slower iterative checking for child classes
		for (auto Itr : AllowedInputClasses)
		{
			if (Other->GetClass()->IsChildOf(Itr))
			{
				return true;
			}
		}
		
		ErrorMessage = FText::FromString("Invalid Node Connection!");
		return false;
	}

	return true;
}

void UMounteaDialogueGraphNode_DialogueNodeBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_DialogueNodeBase, DataTable))
	{
		RowName = FName("");
		Preview.Empty();
	}

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueGraphNode_DialogueNodeBase, RowName))
	{
		UpdatePreviews();
	}
}

void UMounteaDialogueGraphNode_DialogueNodeBase::UpdatePreviews()
{
	//TODO: use custom window in custom Tab rather than this crap
	if (!DataTable) Preview.Empty();

	Preview.Empty();
	const auto Row = UMounteaDialogueSystemBFC::GetDialogueRow(this);
	if (UMounteaDialogueSystemBFC::IsDialogueRowValid(Row))
	{
		for (auto Itr : Row.DialogueRowData.Array())
		{
			Preview.Add( Itr.RowText.ToString() );
		}
	}
	else
	{
		Preview.Empty();
	}
}

#endif	
#undef LOCTEXT_NAMESPACE

