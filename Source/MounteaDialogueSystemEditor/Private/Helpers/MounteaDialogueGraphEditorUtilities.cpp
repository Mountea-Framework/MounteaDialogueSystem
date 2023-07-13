#include "MounteaDialogueGraphEditorUtilities.h"

#include "AssetEditor/AssetEditor_MounteaDialogueGraph.h"
#include "Ed/EdGraph_MounteaDialogueGraph.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Layout/AssetEditorTabs.h"

bool FMounteaDialogueGraphEditorUtilities::PickChildrenOfClass(const FText& TitleText, UClass*& OutChosenClass, UClass* Class)
{
	// Create filter
	TSharedPtr<FMounteaDialogueClassViewerFilter> Filter = MakeShareable(new FMounteaDialogueClassViewerFilter);
	Filter->AllowedChildrenOfClasses.Add(Class);

	// Fill in options
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.bShowUnloadedBlueprints = true;
	Options.ClassFilter = Filter;

	Options.DisplayMode = EClassViewerDisplayMode::TreeView;
	
	Options.bShowNoneOption = false;
	Options.InitiallySelectedClass = Class;

	Options.bExpandRootNodes = true;
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;
	
	return SClassPickerDialog::PickClass(TitleText, Options, OutChosenClass, Class);
}

bool FMounteaDialogueGraphEditorUtilities::OpenBlueprintEditor(UBlueprint* Blueprint, EMounteaDialogueBlueprintOpenType OpenType, FName FunctionNameToOpen, bool bForceFullEditor, bool bAddBlueprintFunctionIfItDoesNotExist)
{
	if (!Blueprint)
	{
		return false;
	}

	Blueprint->bForceFullEditor = bForceFullEditor;

	// Find Function Graph
	UObject* ObjectToFocusOn = nullptr;
	if (OpenType != EMounteaDialogueBlueprintOpenType::None && FunctionNameToOpen != NAME_None)
	{
		UClass* Class = Blueprint->GeneratedClass;
		check(Class);

		if (OpenType == EMounteaDialogueBlueprintOpenType::Function)
		{
			ObjectToFocusOn = bAddBlueprintFunctionIfItDoesNotExist
				? BlueprintGetOrAddFunction(Blueprint, FunctionNameToOpen, Class)
				: BlueprintGetFunction(Blueprint, FunctionNameToOpen, Class);
		}
		else if (OpenType == EMounteaDialogueBlueprintOpenType::Event)
		{
			ObjectToFocusOn = bAddBlueprintFunctionIfItDoesNotExist
				? BlueprintGetOrAddEvent(Blueprint, FunctionNameToOpen, Class)
				: BlueprintGetEvent(Blueprint, FunctionNameToOpen, Class);
		}
	}

	// Default to the last uber graph
	if (ObjectToFocusOn == nullptr)
	{
		ObjectToFocusOn = Blueprint->GetLastEditedUberGraph();
	}
	if (ObjectToFocusOn)
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ObjectToFocusOn);
		return true;
	}

	return OpenEditorForAsset(Blueprint);
}

UEdGraph* FMounteaDialogueGraphEditorUtilities::BlueprintGetOrAddFunction(UBlueprint* Blueprint, FName FunctionName, UClass* FunctionClassSignature)
{
	if (!Blueprint || Blueprint->BlueprintType != BPTYPE_Normal)
	{
		return nullptr;
	}

	// Find existing function
	if (UEdGraph* GraphFunction = BlueprintGetFunction(Blueprint, FunctionName, FunctionClassSignature))
	{
		return GraphFunction;
	}

	// Create a new function
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FunctionName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, /*bIsUserCreated=*/ false, FunctionClassSignature);
	Blueprint->LastEditedDocuments.Add(NewGraph);
	return NewGraph;
}

UEdGraph* FMounteaDialogueGraphEditorUtilities::BlueprintGetFunction(UBlueprint* Blueprint, FName FunctionName, UClass* FunctionClassSignature)
{
	if (!Blueprint || Blueprint->BlueprintType != BPTYPE_Normal)
	{
		return nullptr;
	}

	// Find existing function
	for (UEdGraph* GraphFunction : Blueprint->FunctionGraphs)
	{
		if (FunctionName == GraphFunction->GetFName())
		{
			return GraphFunction;
		}
	}

	// Find in the implemented Interfaces Graphs
	for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
	{
		for (UEdGraph* GraphFunction : Interface.Graphs)
		{
			if (FunctionName == GraphFunction->GetFName())
			{
				return GraphFunction;
			}
		}
	}

	return nullptr;
}

UK2Node_Event* FMounteaDialogueGraphEditorUtilities::BlueprintGetOrAddEvent(UBlueprint* Blueprint, FName EventName, UClass* EventClassSignature)
{
	if (!Blueprint || Blueprint->BlueprintType != BPTYPE_Normal)
	{
		return nullptr;
	}

	// Find existing event
	if (UK2Node_Event* EventNode = BlueprintGetEvent(Blueprint, EventName, EventClassSignature))
	{
		return EventNode;
	}

	// Create a New Event
	if (Blueprint->UbergraphPages.Num())
	{
		int32 NodePositionY = 0;
		UK2Node_Event* NodeEvent = FKismetEditorUtilities::AddDefaultEventNode(
			Blueprint,
			Blueprint->UbergraphPages[0],
			EventName,
			EventClassSignature,
			NodePositionY
		);
		NodeEvent->SetEnabledState(ENodeEnabledState::Enabled);
		NodeEvent->NodeComment = "";
		NodeEvent->bCommentBubbleVisible = false;
		return NodeEvent;
	}

	return nullptr;
}

UK2Node_Event* FMounteaDialogueGraphEditorUtilities::BlueprintGetEvent(UBlueprint* Blueprint, FName EventName, UClass* EventClassSignature)
{
	if (!Blueprint || Blueprint->BlueprintType != BPTYPE_Normal)
	{
		return nullptr;
	}

	TArray<UK2Node_Event*> AllEvents;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_Event>(Blueprint, AllEvents);
	for (UK2Node_Event* EventNode : AllEvents)
	{
		if (EventNode->bOverrideFunction && EventNode->EventReference.GetMemberName() == EventName)
		{
			return EventNode;
		}
	}

	return nullptr;
}

bool FMounteaDialogueGraphEditorUtilities::OpenEditorForAsset(const UObject* Asset)
{
	if (!IsValid(Asset) || !GEditor)
	{
		return false;
	}

	return GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(const_cast<UObject*>(Asset));
}

bool FMounteaDialogueGraphEditorUtilities::OpenEditorAndJumpToGraphNode(TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr, const UEdGraphNode* GraphNode, bool bFocusIfOpen)
{
	if (!IsValid(GraphNode))
	{
		return false;
	}

	if (!DialogueEditorPtr.IsValid())
	{
		return false;
	}

	// Open if not already.
	UMounteaDialogueGraph* Dialogue = GetDialogueFromGraphNode(GraphNode);
	if (!OpenEditorForAsset(Dialogue))
	{
		return false;
	} 

	// Could still fail focus on the graph node
	if (IAssetEditorInstance* EditorInstance = FindEditorForAsset(Dialogue, bFocusIfOpen))
	{
		EditorInstance->FocusWindow(const_cast<UEdGraphNode*>(GraphNode));

		UEdGraph_MounteaDialogueGraph* GraphEditor = Cast<UEdGraph_MounteaDialogueGraph>(Dialogue->EdGraph);
		if (GraphEditor)
		{
			TSet<const UEdGraphNode*> SelectedNodes;
			SelectedNodes.Add(GraphNode);
			
			GraphEditor->SelectNodeSet(SelectedNodes);
			DialogueEditorPtr.Pin()->JumpToNode(GraphNode);
		}
		return true;
	}

	return false;
}

UMounteaDialogueGraph* FMounteaDialogueGraphEditorUtilities::GetDialogueFromGraphNode(const UEdGraphNode* GraphNode)
{
	if (const UEdNode_MounteaDialogueGraphNode* DialogueBaseNode = Cast<UEdNode_MounteaDialogueGraphNode>(GraphNode))
	{
		return DialogueBaseNode->GetDialogueGraphEdGraph()->GetMounteaDialogueGraph();
	}

	// Last chance
	if (const UEdGraph_MounteaDialogueGraph* DialogueGraph = Cast<UEdGraph_MounteaDialogueGraph>(GraphNode->GetGraph()))
	{
		return Cast<UMounteaDialogueGraph>(DialogueGraph->GetMounteaDialogueGraph());
	}

	return nullptr;
}

IAssetEditorInstance* FMounteaDialogueGraphEditorUtilities::FindEditorForAsset(UObject* Asset, bool bFocusIfOpen)
{
	if (!IsValid(Asset) || !GEditor)
	{
		return nullptr;
	}

	return GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Asset, bFocusIfOpen);
}

