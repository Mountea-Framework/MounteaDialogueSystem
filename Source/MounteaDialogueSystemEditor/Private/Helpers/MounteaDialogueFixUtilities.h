// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

struct FNodeReplacementRule
{
    // Old node definition
	struct FOldNode
	{
		FString ClassName;
		FString Type;
		FString Parent;  // Can be empty for broken BP nodes
		FString Function;
		bool bIsInterfaceCall;
		bool bIsBlueprintFunction;
		TArray<FString> IgnorePins;
	} OldNode;

	// New node definition
	struct FNewNode
	{
		FString ClassName;
		FString Type;
		FString Parent;
		FString Function;
		bool bIsInterfaceCall;
		bool bIsBlueprintFunction;
		TMap<FString, FString> PinMapping;
	} NewNode;

    bool FromJson(const TSharedPtr<FJsonObject>& JsonObject);
};

class FMounteaDialogueFixUtilities
{

private:
	static TArray<FNodeReplacementRule> LoadReplacementRules();
	static void ProcessBlueprint(UBlueprint* Blueprint, const TArray<FNodeReplacementRule>& Rules);
	static bool ShouldReplaceNode(UK2Node* Node, const FNodeReplacementRule::FOldNode& OldNodeDef);
	static void ReplaceNode(UEdGraph* Graph, UK2Node* OldNode, const FNodeReplacementRule::FNewNode& NewNodeDef, const FNodeReplacementRule::FOldNode& OldNodeDef);
	static void ReconnectPins(UK2Node* OldNode, UK2Node* NewNode);
	static UFunction* FindFunction(const FString& ParentPath, const FString& FunctionName);

public:

	static void ReplaceNodesInSelectedBlueprints();
	static bool CanExecute();
};
