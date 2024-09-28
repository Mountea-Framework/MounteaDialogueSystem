// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueUIBFL.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;

struct FDialogueRowData;
struct FDialogueOptionData;
struct FDialogueRow;
struct FWidgetDialogueRow;

/**
 * 
 */
UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueUIBFL : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta = (BlueprintThreadSafe), meta=(CustomTag="MounteaK2Setter"))
	static FDialogueOptionData NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow);

	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta = (BlueprintThreadSafe), meta=(CustomTag="MounteaK2Setter"))
	static FWidgetDialogueRow NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FGuid GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Setter"))
	static FText ReplaceRegexInText(const FString& Regex, const FText& Replacement, const FText& SourceText);

};
