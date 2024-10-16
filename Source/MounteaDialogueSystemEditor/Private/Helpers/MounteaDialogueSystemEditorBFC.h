// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "MounteaDialogueSystemEditorBFC.generated.h"

/**
 * Editor Only helper functions.
 */
UCLASS()
class UMounteaDialogueSystemEditorBFC : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

#if WITH_EDITOR
	
	static const UMounteaDialogueGraphEditorSettings* GetMounteaDialogueEditorSettings()
	{
		return GetDefault<UMounteaDialogueGraphEditorSettings>();
	}

	static FText GetNodeTitle(UMounteaDialogueGraphNode* Node)
	{
		if (!Node) 
		{
			return FText::FromString("Invalid Node");
		}

		const UMounteaDialogueGraphEditorSettings* const EditorSettings = GetMounteaDialogueEditorSettings();
		if (!EditorSettings || !EditorSettings->ShowAutomaticNames())
		{
			return Node->GetNodeTitle();
		}

		const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node);
		if (!DialogueNodeBase || !DialogueNodeBase->GetDataTable())
		{
			return Node->GetNodeTitle();
		}

		FString ReturnString;
		DialogueNodeBase->GetRowName().ToString(ReturnString);
		return FText::FromString(ReturnString);
	}

	static ENodeTheme GetNodeTheme()
	{
		if (GetMounteaDialogueEditorSettings() != nullptr)
		{
			return GetMounteaDialogueEditorSettings()->GetNodeTheme();
		}

		return ENodeTheme::ENT_DarkTheme;
	}

	static void TriggerPreviewRefresh(TArray<UObject*> NodeObjects)
	{
		for (auto Itr : NodeObjects)
		{
			UEdNode_MounteaDialogueGraphNode* SelectedNode = Cast<UEdNode_MounteaDialogueGraphNode>(Itr);
			if (!SelectedNode) continue;
			if (!SelectedNode->DialogueGraphNode) continue;

			UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(SelectedNode->DialogueGraphNode);
			if (!DialogueNodeBase) continue;

			DialogueNodeBase->UpdatePreviews();
		}
	}

#endif
	
};
