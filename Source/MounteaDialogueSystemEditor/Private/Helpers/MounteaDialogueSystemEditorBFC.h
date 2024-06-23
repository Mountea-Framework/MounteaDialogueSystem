// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Nodes/MounteaDialogueGraphNode.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"
#include "MounteaDialogueSystemEditorBFC.generated.h"

class UEdNode_MounteaDialogueGraphNode;

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
		if (!Node) return FText::FromString("Invalid Node");
		
		if (GetMounteaDialogueEditorSettings())
		{
			if (GetMounteaDialogueEditorSettings()->ShowAutomaticNames())
			{
				if (const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
				{
					if (DialogueNodeBase->GetDataTable())
					{
						FString ReturnString;
						DialogueNodeBase->GetRowName().ToString(ReturnString);

						return FText::FromString(ReturnString);
					}
				}

				return Node->GetInternalName();
			}
		}
		
		return Node->GetNodeTitle();
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
