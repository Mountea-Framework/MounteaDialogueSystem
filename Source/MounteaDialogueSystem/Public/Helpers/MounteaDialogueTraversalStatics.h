// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Interfaces/Core/MounteaDialogueConditionContextInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MounteaDialogueTraversalStatics.generated.h"

class IMounteaDialogueParticipantInterface;
class UMounteaDialogueContext;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class UDataTable;

UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueTraversalStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode*> GetAllowedChildNodesFiltered(
		const UMounteaDialogueGraphNode* ParentNode,
		const TScriptInterface<IMounteaDialogueConditionContextInterface>& ConditionContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static UMounteaDialogueGraphNode* GetFirstChildNode(const UMounteaDialogueGraphNode* ParentNode);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static void SortNodes(TArray<UMounteaDialogueGraphNode*>& SortedNodes);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetSpeechData(UMounteaDialogueGraphNode* Node);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetDialogueRow(const UDataTable* SourceTable, FName SourceName);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Validate"))
	static bool IsDialogueRowValid(const FDialogueRow& Row);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Validate"))
	static bool IsDialogueRowDataValid(const FDialogueRowData& Data);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static float GetRowDuration(const FDialogueRowData& Row);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static TArray<FMounteaDialogueDecorator> GetAllDialogueDecorators(const UMounteaDialogueGraph* FromGraph);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Validate"))
	static bool HasNodeBeenTraversed(
		const UMounteaDialogueGraphNode* Node,
		const TArray<FDialogueTraversePath>& TraversedPath);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers",
		meta=(CustomTag="MounteaK2Getter"))
	static TScriptInterface<IMounteaDialogueParticipantInterface> FindParticipantByTag(
		const UMounteaDialogueContext* DialogueContext,
		FGameplayTag SearchTag);

	static TScriptInterface<IMounteaDialogueParticipantInterface> ResolveActiveParticipant(const UMounteaDialogueContext* DialogueContext);
	static bool UpdateMatchingDialogueParticipant(
		UMounteaDialogueContext* Context,
		const TScriptInterface<IMounteaDialogueParticipantInterface>& NewActiveParticipant);
};
