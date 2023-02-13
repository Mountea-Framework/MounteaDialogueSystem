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
		if (!Node) return FText::FromString("Invalid Node");
		
		if (GetMounteaDialogueEditorSettings())
		{
			if (GetMounteaDialogueEditorSettings()->ShowAutomaticNames())
			{
				if (const UMounteaDialogueGraphNode_DialogueNodeBase* DialogueNodeBase = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(Node))
				{
					if (DialogueNodeBase->GetDialogueGraphHandle().DataTable)
					{
						FString ReturnString;
						DialogueNodeBase->GetDialogueGraphHandle().RowName.ToString(ReturnString);

						return FText::FromString(ReturnString);
					}
				}

				return Node->GetInternalName();
			}
		}
		
		return Node->GetNodeTitle();
	}

#endif
	
};
