// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueFixUtilities.h"

#include "EditorUtilityLibrary.h"
#include "K2Node_CallFunction.h"
#include "Dom/JsonObject.h"
#include "K2Nodes/K2Node_MounteaDialogueCallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

///////////////////////////////////////////////////////////
// InternalBlueprintEditorLibrary
// Stolen from BlueprintEditorLibrary
namespace InternalBlueprintEditorLibrary
{
	/**
	* Replace the OldNode with the NewNode and reconnect it's pins. If the pins don't
	* exist on the NewNode, then orphan the connections.
	*
	* @param OldNode		The old node to replace
	* @param NewNode		The new node to put in the old node's place
	*/
	static bool ReplaceOldNodeWithNew(UEdGraphNode* OldNode, UEdGraphNode* NewNode)
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		
		bool bSuccess = false;

		if (Schema && OldNode && NewNode)
		{
			TMap<FName, FName> OldToNewPinMap;
			for (UEdGraphPin* Pin : OldNode->Pins)
			{
				if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
				{
					// there's no analogous pin, signal that we're expecting this
					OldToNewPinMap.Add(Pin->PinName, NAME_None);
				}
				else
				{
					// The input pins follow the same naming scheme
					OldToNewPinMap.Add(Pin->PinName, Pin->PinName);
				}
			}
			
			bSuccess = Schema->ReplaceOldNodeWithNew(OldNode, NewNode, OldToNewPinMap);
			// reconstructing the node will clean up any
			// incorrect default values that may have been copied over
			NewNode->ReconstructNode();
		}

		return bSuccess;
	}

	/**
	* Returns true if any of these nodes pins have any links. Does not check for 
	* a default value on pins
	*
	* @param Node		The node to check
	*
	* @return bool		True if the node has any links, false otherwise.
	*/
	static bool NodeHasAnyConnections(const UEdGraphNode* Node)
	{
		if (Node)
		{
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->LinkedTo.Num() > 0)
				{
					return true;
				}
			}
		}

		return false;
	}
	
	static FString GetPinDirectionStr(EEdGraphPinDirection Direction)
	{
		return Direction == EGPD_Input ? TEXT("_Input") : TEXT("_Output");
	}
};

bool FNodeReplacementRule::FromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	const TSharedPtr<FJsonObject>* OldNodeObj;
	if (JsonObject->TryGetObjectField(TEXT("old_node"), OldNodeObj))
	{
		(*OldNodeObj)->TryGetStringField(TEXT("class_name"), OldNode.ClassName);
		(*OldNodeObj)->TryGetStringField(TEXT("type"), OldNode.Type);
		(*OldNodeObj)->TryGetStringField(TEXT("parent"), OldNode.Parent);
		(*OldNodeObj)->TryGetStringField(TEXT("function"), OldNode.Function);
		(*OldNodeObj)->TryGetBoolField(TEXT("is_interface_call"), OldNode.bIsInterfaceCall);
	}

	// Parse new node definition
	const TSharedPtr<FJsonObject>* NewNodeObj;
	if (JsonObject->TryGetObjectField(TEXT("new_node"), NewNodeObj))
	{
		(*NewNodeObj)->TryGetStringField(TEXT("parent"), NewNode.Parent);
		(*NewNodeObj)->TryGetStringField(TEXT("function"), NewNode.Function);
		(*NewNodeObj)->TryGetBoolField(TEXT("is_interface_call"), NewNode.bIsInterfaceCall);
	}

	return !OldNode.ClassName.IsEmpty() && !NewNode.Parent.IsEmpty() && !NewNode.Function.IsEmpty();
}

TArray<FNodeReplacementRule> FMounteaDialogueFixUtilities::LoadReplacementRules()
{
	TArray<FNodeReplacementRule> Rules;
	
	FString PluginBaseDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("MounteaDialogueSystem"))
	);
	
	FString ConfigPath = FPaths::Combine(
		PluginBaseDir,
		TEXT("Source"),
		TEXT("MounteaDialogueSystemEditor"),
		TEXT("Config"),
		TEXT("node_replacements.json")
	);
	
	// Convert to full path and normalize
	ConfigPath = FPaths::ConvertRelativePathToFull(ConfigPath);
	FPaths::NormalizeFilename(ConfigPath);
	
	FString JsonString;
	
	if (!FPaths::FileExists(ConfigPath))
	{
		return Rules;
	}
	
	if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
	{
		return Rules;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
	
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return Rules;
	}

	const TArray<TSharedPtr<FJsonValue>>* ReplacementsArray;
	if (JsonObject->TryGetArrayField(TEXT("replacements"), ReplacementsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *ReplacementsArray)
		{
			if (const TSharedPtr<FJsonObject> ReplacementObj = Value->AsObject())
			{
				FNodeReplacementRule Rule;
				if (Rule.FromJson(ReplacementObj))
				{
					Rules.Add(Rule);
				}
			}
		}
	}

	return Rules;
}

void FMounteaDialogueFixUtilities::ReplaceNodesInSelectedBlueprints()
{
	TArray<FNodeReplacementRule> Rules = LoadReplacementRules();
	if (Rules.Num() == 0)
	{
		return;
	}

	TArray<FAssetData> SelectedAssets = UEditorUtilityLibrary::GetSelectedAssetData();
	for (const FAssetData& Asset : SelectedAssets)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
		{
			ProcessBlueprint(Blueprint, Rules);
		}
	}
}

bool FMounteaDialogueFixUtilities::CanExecute()
{
	return UEditorUtilityLibrary::GetSelectedAssetData().Num() > 0;
}

void FMounteaDialogueFixUtilities::ProcessBlueprint(UBlueprint* Blueprint, const TArray<FNodeReplacementRule>& Rules)
{
	bool bModified = false;

	// Process all graphs in the Blueprint
	TArray<UEdGraph*> AllGraphs;
	
	// Get UbergraphPages (EventGraph)
	AllGraphs.Append(Blueprint->UbergraphPages);
	
	// Get function graphs
	AllGraphs.Append(Blueprint->FunctionGraphs);
	
	// Get macro graphs
	AllGraphs.Append(Blueprint->MacroGraphs);
	
	// Get implemented interface graphs
	for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
	{
		AllGraphs.Append(Interface.Graphs);
	}

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		TArray<UK2Node*> AllNodes;
		Graph->GetNodesOfClass(AllNodes);

		for (UK2Node* Node : AllNodes)
		{
			for (const FNodeReplacementRule& Rule : Rules)
			{
				if (ShouldReplaceNode(Node, Rule.OldNode))
				{
					ReplaceNode(Graph, Node, Rule.NewNode);
					bModified = true;
					break;
				}
			}
		}
	}

	if (bModified)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
}

bool FMounteaDialogueFixUtilities::ShouldReplaceNode(UK2Node* Node, const FNodeReplacementRule::FOldNode& OldNodeDef)
{
	if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
	{
		const FMemberReference& FuncRef = FuncNode->FunctionReference;
		return FuncRef.GetMemberName() == *OldNodeDef.Function;
	}
	return false;
}

void FMounteaDialogueFixUtilities::ReplaceNode(UEdGraph* Graph, UK2Node* OldNode, const FNodeReplacementRule::FNewNode& NewNodeDef)
{
	if (!Graph || !OldNode)
	{
		 return;
	}

	if (UClass* Class = LoadObject<UClass>(nullptr, *NewNodeDef.Parent))
	{
		 if (UFunction* NewFunction = Class->FindFunctionByName(*NewNodeDef.Function))
		 {
			  Graph->Modify();
		 	
			  UK2Node_MounteaDialogueCallFunction* NewNode = NewObject<UK2Node_MounteaDialogueCallFunction>(Graph);
			  NewNode->SetFromFunction(NewFunction);
			  NewNode->bIsInterfaceCall = NewNodeDef.bIsInterfaceCall;
			  
			  if (UK2Node_CallFunction* OldFuncNode = Cast<UK2Node_CallFunction>(OldNode))
			  {
					NewNode->bIsPureFunc = OldFuncNode->bIsPureFunc;
			  }

			  NewNode->NodePosX = OldNode->NodePosX;
			  NewNode->NodePosY = OldNode->NodePosY;
		 	
			  Graph->AddNode(NewNode, false);
			  NewNode->AllocateDefaultPins();
		 	
			  TMap<FName, FName> OldToNewPinMap;
		 	
			  TMap<FString, TArray<UEdGraphPin*>> NewPinsByCategory;
			  for (UEdGraphPin* NewPin : NewNode->Pins)
			  {
					if (!NewPin->ParentPin) // Skip split pins
					{
						 const FString CategoryKey = NewPin->PinType.PinCategory.ToString() + 
							  InternalBlueprintEditorLibrary::GetPinDirectionStr(NewPin->Direction);
						 NewPinsByCategory.FindOrAdd(CategoryKey).Add(NewPin);
					}
			  }
		 	
			  for (UEdGraphPin* OldPin : OldNode->Pins)
			  {
					if (OldPin->ParentPin)
					{
						 // Skip split pins - they'll be handled by parent pins
						 continue;
					}
					
					if (OldPin->PinName == UEdGraphSchema_K2::PN_Self)
					{
						 OldToNewPinMap.Add(OldPin->PinName, NAME_None);
						 continue;
					}

					// First try to find exact match by name
					if (UEdGraphPin* ExactMatch = NewNode->FindPin(OldPin->PinName))
					{
						 OldToNewPinMap.Add(OldPin->PinName, ExactMatch->PinName);
						 continue;
					}

					// If no exact match, try to find a pin of the same type and direction
					const FString CategoryKey = OldPin->PinType.PinCategory.ToString() + 
						 InternalBlueprintEditorLibrary::GetPinDirectionStr(OldPin->Direction);
					
					if (const TArray<UEdGraphPin*>* MatchingPins = NewPinsByCategory.Find(CategoryKey))
					{
						 if (MatchingPins->Num() > 0)
						 {
							  UEdGraphPin* MatchingPin = (*MatchingPins)[0];
							  OldToNewPinMap.Add(OldPin->PinName, MatchingPin->PinName);
						 	
							  NewPinsByCategory.FindOrAdd(CategoryKey).Remove(MatchingPin);
							  continue;
						 }
					}
			  	
					OldToNewPinMap.Add(OldPin->PinName, NAME_None);
			  }

			  // Let the engine handle the replacement with our custom mapping
			  if (InternalBlueprintEditorLibrary::ReplaceOldNodeWithNew(OldNode, NewNode))
			  {
					NewNode->ReconstructNode();
			  	
					if (UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetOuter()))
					{
						 FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
					}
					
					Graph->NotifyGraphChanged();
			  }
			  else
			  {
					Graph->RemoveNode(NewNode);
			  }
		 }
	}
}

void FMounteaDialogueFixUtilities::ReconnectPins(UK2Node* OldNode, UK2Node* NewNode)
{
	TArray<UEdGraphPin*> OldPins = OldNode->Pins;
	TArray<UEdGraphPin*> NewPins = NewNode->Pins;

	for (UEdGraphPin* OldPin : OldPins)
	{
		if (UEdGraphPin* NewPin = NewNode->FindPin(OldPin->GetFName()))
		{
			if (OldPin->Direction == NewPin->Direction)
			{
				for (UEdGraphPin* LinkedPin : OldPin->LinkedTo)
				{
					NewPin->MakeLinkTo(LinkedPin);
				}
			}
		}
	}
}

UFunction* FMounteaDialogueFixUtilities::FindFunction(const FString& ParentPath, const FString& FunctionName)
{
	if (UClass* Class = LoadObject<UClass>(nullptr, *ParentPath))
	{
		return Class->FindFunctionByName(*FunctionName);
	}
	return nullptr;
}
