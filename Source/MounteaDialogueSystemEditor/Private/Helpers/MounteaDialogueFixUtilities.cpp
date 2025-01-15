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
	* @param OldNode			The old node to replace
	* @param NewNode			The new node to put in the old node's place
	* @param CustomPinMapping	Optional predefined mapping
	*/
	static bool ReplaceOldNodeWithNew(UEdGraphNode* OldNode, UEdGraphNode* NewNode, const TMap<FString, FString>* CustomPinMapping = nullptr, const TArray<FString>* IgnorePins = nullptr)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	
	bool bSuccess = false;

	if (Schema && OldNode && NewNode)
	{
		TMap<FName, FName> OldToNewPinMap;

		// Special handling for interface to static function conversion
		UK2Node_CallFunction* OldFuncNode = Cast<UK2Node_CallFunction>(OldNode);
		UK2Node_CallFunction* NewFuncNode = Cast<UK2Node_CallFunction>(NewNode);
		bool bIsInterfaceToStatic = OldFuncNode && NewFuncNode && 
						   OldFuncNode->FunctionReference.GetMemberParentClass() && 
						   OldFuncNode->FunctionReference.GetMemberParentClass()->HasAnyClassFlags(CLASS_Interface) && 
						   (!NewFuncNode->FunctionReference.GetMemberParentClass() || 
							!NewFuncNode->FunctionReference.GetMemberParentClass()->HasAnyClassFlags(CLASS_Interface));

		for (UEdGraphPin* Pin : OldNode->Pins)
		{
			if (Pin->ParentPin)
			{
				continue;
			}

			// Handle pins that should be ignored by mapping them to NAME_None
			if (IgnorePins && IgnorePins->Contains(Pin->PinName.ToString()))
			{
				OldToNewPinMap.Add(Pin->PinName, NAME_None);
				continue;
			}

			if (Pin->PinName == UEdGraphSchema_K2::PN_Self)
			{
				OldToNewPinMap.Add(Pin->PinName, NAME_None);
				continue;
			}

			// Special handling for interface Target pin when converting to static
			if (bIsInterfaceToStatic && Pin->PinName == TEXT("Target"))
			{
				// Map interface Target pin to the first parameter of the static function
				for (UEdGraphPin* NewPin : NewNode->Pins)
				{
					if (!NewPin->ParentPin && 
						NewPin->Direction == EGPD_Input && 
						NewPin->PinName != TEXT("self"))
					{
						OldToNewPinMap.Add(Pin->PinName, NewPin->PinName);
						break;
					}
				}
				continue;
			}

			// Regular pin mapping
			if (CustomPinMapping && CustomPinMapping->Contains(Pin->PinName.ToString()))
			{
				OldToNewPinMap.Add(Pin->PinName, FName(*CustomPinMapping->FindRef(Pin->PinName.ToString())));
			}
			else 
			{
				OldToNewPinMap.Add(Pin->PinName, Pin->PinName);
			}
		}
		
		bSuccess = Schema->ReplaceOldNodeWithNew(OldNode, NewNode, OldToNewPinMap);
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
		(*OldNodeObj)->TryGetBoolField(TEXT("is_blueprint_function"), OldNode.bIsBlueprintFunction);
		
		const TArray<TSharedPtr<FJsonValue>>* IgnorePinsArray;
		if ((*OldNodeObj)->TryGetArrayField(TEXT("ignore_pins"), IgnorePinsArray))
		{
			for (const TSharedPtr<FJsonValue>& Value : *IgnorePinsArray)
			{
				if (Value->Type == EJson::String)
				{
					OldNode.IgnorePins.Add(Value->AsString());
				}
			}
		}
	}

	// Parse new node definition
	const TSharedPtr<FJsonObject>* NewNodeObj;
	if (JsonObject->TryGetObjectField(TEXT("new_node"), NewNodeObj))
	{
		(*NewNodeObj)->TryGetStringField(TEXT("class_name"), NewNode.ClassName);
		(*NewNodeObj)->TryGetStringField(TEXT("type"), NewNode.Type);
		(*NewNodeObj)->TryGetStringField(TEXT("parent"), NewNode.Parent);
		(*NewNodeObj)->TryGetStringField(TEXT("function"), NewNode.Function);
		(*NewNodeObj)->TryGetBoolField(TEXT("is_interface_call"), NewNode.bIsInterfaceCall);
		(*NewNodeObj)->TryGetBoolField(TEXT("is_blueprint_function"), NewNode.bIsBlueprintFunction);

		const TSharedPtr<FJsonObject>* PinMappingObj;
		if ((*NewNodeObj)->TryGetObjectField(TEXT("pin_mapping"), PinMappingObj))
		{
			for (const auto& Pair : (*PinMappingObj)->Values)
			{
				const FString MappedName = Pair.Value->AsString();
				if (!MappedName.IsEmpty())
				{
					NewNode.PinMapping.Add(Pair.Key, *MappedName);
				}
			}
		}
	}

	return !OldNode.Function.IsEmpty() && !NewNode.Function.IsEmpty() && 
			   !NewNode.Parent.IsEmpty();
}

TArray<FNodeReplacementRule> FMounteaDialogueFixUtilities::LoadReplacementRules()
{
	TArray<FNodeReplacementRule> Rules;
	
	FString PluginBaseDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("MounteaDialogueSystem"))
	);
	
	FString ConfigPath = FPaths::Combine(
		PluginBaseDir,
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
	
	TArray<UEdGraph*> AllGraphs;
	Blueprint->GetAllGraphs(AllGraphs);

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
					ReplaceNode(Graph, Node, Rule.NewNode, Rule.OldNode);
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
		
		// For Blueprint functions we need to check both name and parent class
		if (FuncRef.GetMemberGuid().IsValid())  // Is Blueprint function
		{
			return FuncRef.GetMemberName() == *OldNodeDef.Function &&
				   (OldNodeDef.Parent.IsEmpty() || // If parent is empty in config, match any
					FuncRef.GetMemberParentPackage()->GetPathName() == OldNodeDef.Parent);
		}
		
		// For C++ functions
		return FuncRef.GetMemberName() == *OldNodeDef.Function;
	}
	return false;
}

void FMounteaDialogueFixUtilities::ReplaceNode(UEdGraph* Graph, UK2Node* OldNode, const FNodeReplacementRule::FNewNode& NewNodeDef, const FNodeReplacementRule::FOldNode& OldNodeDef)
{
	if (!Graph || !OldNode)
	{
		return;
	}

	UClass* Class = nullptr;
	UFunction* NewFunction = nullptr;
	
	if (NewNodeDef.bIsBlueprintFunction)
	{
		// For Blueprint classes, we need to load the Class directly
		Class = LoadObject<UClass>(nullptr, *NewNodeDef.Parent);
		
		if (!Class)
		{
			// If direct class load failed, try to remove the _C suffix and load as Blueprint
			FString BPPath = NewNodeDef.Parent;
			BPPath.RemoveFromEnd(TEXT("_C"));
			if (UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BPPath))
			{
				Class = Blueprint->GeneratedClass;
			}
		}
	}
	else // C++ functions
	{
		Class = LoadObject<UClass>(nullptr, *NewNodeDef.Parent);
	}

	if (Class)
	{
		NewFunction = Class->FindFunctionByName(*NewNodeDef.Function);
	}

	if (NewFunction)
	{
		Graph->Modify();
			
		UK2Node_MounteaDialogueCallFunction* NewNode = NewObject<UK2Node_MounteaDialogueCallFunction>(Graph);
		NewNode->SetFromFunction(NewFunction);
		NewNode->FunctionReference.SetFromField<UFunction>(NewFunction, NewNodeDef.bIsInterfaceCall);
		if (OldNode->NodeGuid.IsValid())
		{
			NewNode->NodeGuid = OldNode->NodeGuid;
		}
		else
		{
			NewNode->NodeGuid = FGuid::NewGuid();
		}
			
		if (UK2Node_CallFunction* OldFuncNode = Cast<UK2Node_CallFunction>(OldNode))
		{
			NewNode->bDefaultsToPureFunc = OldFuncNode->IsNodePure();
		}
			
		NewNode->NodePosX = OldNode->NodePosX;
		NewNode->NodePosY = OldNode->NodePosY;
			
		Graph->AddNode(NewNode, false);
		NewNode->AllocateDefaultPins();

		// Let the engine handle the replacement
		if (InternalBlueprintEditorLibrary::ReplaceOldNodeWithNew(OldNode, NewNode, 
			NewNodeDef.PinMapping.Num() > 0 ? &NewNodeDef.PinMapping : nullptr,
			OldNodeDef.IgnorePins.Num() > 0 ? &OldNodeDef.IgnorePins : nullptr))
		{
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
