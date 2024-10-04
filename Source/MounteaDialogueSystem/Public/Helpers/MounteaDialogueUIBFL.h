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

	/**
	 * Creates a new dialogue option data structure.
	 * 
	 * @param Node The GUID of the dialogue node for which the option is created.
	 * @param DialogueRow The row data associated with the dialogue node.
	 * @return A new `FDialogueOptionData` structure containing the node and row data.
	 */
	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta = (BlueprintThreadSafe), meta=(CustomTag="MounteaK2Setter"))
	static FDialogueOptionData NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow);

	/**
	 * Creates a new dialogue widget row data structure.
	 * 
	 * @param SourceRow The source row from which to create the widget data.
	 * @param SourceRowData Additional data associated with the source row.
	 * @return A new `FWidgetDialogueRow` structure containing the source row and its data.
	 */
	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta = (BlueprintThreadSafe), meta=(CustomTag="MounteaK2Setter"))
	static FWidgetDialogueRow NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData);

	/**
	 * Retrieves the GUID of a dialogue node.
	 * 
	 * @param FromNode The dialogue node from which to retrieve the GUID.
	 * @return The GUID associated with the specified node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FGuid GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	/**
	 * Retrieves the dialogue row associated with a specific node.
	 * 
	 * @param FromNode The dialogue node from which to retrieve the row.
	 * @return The dialogue row associated with the specified node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	/**
	 * Filters an array of dialogue nodes to only include dialogue-friendly nodes.
	 * 
	 * @param RawNodes The array of raw dialogue nodes to be filtered.
	 * @return An array of nodes that are classified as dialogue-friendly.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes);

	/**
	 * Replaces text in a source string using a regular expression.
	 * 
	 * @param Regex The regular expression pattern to search for in the source text.
	 * @param Replacement The text to replace the matched pattern.
	 * @param SourceText The original text where the replacement will occur.
	 * @return The modified text with the replacements applied.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Setter"))
	static FText ReplaceRegexInText(const FString& Regex, const FText& Replacement, const FText& SourceText);


};
