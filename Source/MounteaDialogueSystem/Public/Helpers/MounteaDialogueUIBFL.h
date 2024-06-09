// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueUIBFL.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;
struct FDialogueOptionData;
struct FDialogueRow;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueUIBFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category = "Mountea|Dialogue", meta = (BlueprintThreadSafe))
	static FDialogueOptionData NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	static FGuid GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
	static FDialogueRow GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	static TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes);

	UFUNCTION(BlueprintCallable, Category = "Mountea|Dialogue")
	static FText ReplaceRegexInText(const FString& Regex, const FText& Replacement, const FText& SourceText);

};
