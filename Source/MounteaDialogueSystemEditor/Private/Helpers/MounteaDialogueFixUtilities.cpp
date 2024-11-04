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

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
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
    if (UClass* Class = LoadObject<UClass>(nullptr, *NewNodeDef.Parent))
    {
        if (UFunction* NewFunction = Class->FindFunctionByName(*NewNodeDef.Function))
        {
            // Store old node's connections before replacing
            TArray<UEdGraphPin*> OldPins = OldNode->Pins;
            
            // Create the correct node type
            UK2Node_MounteaDialogueCallFunction* NewNode = NewObject<UK2Node_MounteaDialogueCallFunction>(Graph);
            NewNode->SetFromFunction(NewFunction);
            NewNode->bIsInterfaceCall = NewNodeDef.bIsInterfaceCall;
            
            if (UK2Node_CallFunction* OldFuncNode = Cast<UK2Node_CallFunction>(OldNode))
            {
                NewNode->bIsPureFunc = OldFuncNode->bIsPureFunc;
            }
            
            Graph->AddNode(NewNode, false);
            NewNode->NodePosX = OldNode->NodePosX;
            NewNode->NodePosY = OldNode->NodePosY;

            // Reconstruct the node to create all pins
            NewNode->ReconstructNode();
            
            // Now reconnect pins after reconstruction
            for (UEdGraphPin* OldPin : OldPins)
            {
                if (UEdGraphPin* NewPin = NewNode->FindPin(OldPin->GetFName()))
                {
                    // Copy default values if any
                    NewPin->DefaultValue = OldPin->DefaultValue;
                    NewPin->DefaultObject = OldPin->DefaultObject;
                    NewPin->DefaultTextValue = OldPin->DefaultTextValue;
                    
                    // Reconnect links
                    for (UEdGraphPin* LinkedPin : OldPin->LinkedTo)
                    {
                        NewPin->MakeLinkTo(LinkedPin);
                    }
                }
            }

            Graph->RemoveNode(OldNode);
            
            // Notify graph that it has changed
            Graph->NotifyGraphChanged();
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
