// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode.h"

#include "Algo/AnyOf.h"
#include "Data/MounteaDialogueContext.h"
#include "Decorators/MounteaDialogueDecorator_OnlyFirstTime.h"
#include "Decorators/MounteaDialogueDecorator_OverrideOnlyFirstTime.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueContextStatics.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueParticipantStatics.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNode"

namespace MounteaDialogueNodeRuntime
{
	void ExecuteDecorators(const UMounteaDialogueContext* DialogueContext)
	{
		if (!IsValid(DialogueContext))
			return;

		const TScriptInterface<IMounteaDialogueParticipantInterface> graphOwner =
			UMounteaDialogueParticipantStatics::GetGraphOwnerParticipant(DialogueContext->DialogueParticipants);
		if (!graphOwner.GetObject() || !graphOwner.GetInterface())
			return;

		UObject* participantObject = graphOwner.GetObject();
		const UMounteaDialogueGraph* dialogueGraph = graphOwner->Execute_GetDialogueGraph(participantObject);
		const UMounteaDialogueGraphNode* activeNode = DialogueContext->GetActiveNode();
		if (!IsValid(dialogueGraph) || !IsValid(activeNode))
			return;

		TArray<FMounteaDialogueDecorator> allDecorators;
		allDecorators.Append(activeNode->GetNodeDecorators());
		if (activeNode->DoesInheritDecorators())
			allDecorators.Append(dialogueGraph->GetGraphDecorators());

		for (const auto& decorator : allDecorators)
			decorator.ExecuteDecorator();
	}
}

UMounteaDialogueGraphNode::UMounteaDialogueGraphNode(): Graph(nullptr), OwningWorld(nullptr)
{
	NodeGUID = FGuid::NewGuid();
	bInheritGraphDecorators = true;

	NodeTypeName = LOCTEXT("MounteaDialogueNode_InternalName", "MounteaDialogueGraphNode");

#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UMounteaDialogueGraph::StaticClass();

	EditorNodeColour = FLinearColor::FromSRGBColor(FColor::FromHex(TEXT("f97316")));
	EditorHeaderForegroundColour = FLinearColor::White;

	bAllowInputNodes = true;
	bAllowOutputNodes = true;

	bAllowCopy = true;
	bAllowCut = true;
	bAllowDelete = true;
	bAllowPaste = true;
	bAllowManualCreate = true;
	bCanRenameNode = true;	
	NodeTooltipText = LOCTEXT("MounteaDialogueNode_Tooltip", "Mountea Dialogue Base Node.\n\nChild Nodes provide more Information.");
#endif

	bAutoStarts = false;
}

void UMounteaDialogueGraphNode::SetNodeGUID(const FGuid& NewGuid)
{
	NodeGUID = NewGuid;
}

UMounteaDialogueGraph* UMounteaDialogueGraphNode::GetGraph() const
{
	return Graph;
}

FGuid UMounteaDialogueGraphNode::GetGraphGUID() const
{
	return Graph ? Graph->GetGraphGUID() : FGuid();
}

void UMounteaDialogueGraphNode::CleanupNode_Implementation()
{
	OwningWorld = nullptr;

	Execute_UnregisterTick(this, Graph);
	
	for (const auto& nodeDecorator : NodeDecorators)
	{
		if (!IsValid(nodeDecorator.DecoratorType))
			continue;

		nodeDecorator.DecoratorType->CleanupDecorator();
	}
	
	OnNodeStateChanged.Clear();
}

void UMounteaDialogueGraphNode::SetNewWorld(UWorld* NewWorld)
{
	if (!NewWorld) return;
	if (NewWorld == OwningWorld) return;

	OwningWorld = NewWorld;
}

TArray<TSubclassOf<UMounteaDialogueGraphNode>> UMounteaDialogueGraphNode::GetAllowedInputClasses_Implementation() const
{
	return AllowedInputClasses;
}

void UMounteaDialogueGraphNode::RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().AddUniqueDynamic(this, &UMounteaDialogueGraphNode::TickMounteaEvent);
		
		for (const auto& nodeDecorator : NodeDecorators)
		{
			if (!IsValid(nodeDecorator.DecoratorType))
				continue;
			
			if (nodeDecorator.DecoratorType->Implements<UMounteaDialogueTickableObject>())
				IMounteaDialogueTickableObject::Execute_RegisterTick(nodeDecorator.DecoratorType, this);
		}
	}
}

void UMounteaDialogueGraphNode::UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().RemoveDynamic(this, &UMounteaDialogueGraphNode::TickMounteaEvent);
		
		for (const auto& nodeDecorator : NodeDecorators)
		{
			if (!IsValid(nodeDecorator.DecoratorType))
				continue;
			
			if (nodeDecorator.DecoratorType->Implements<UMounteaDialogueTickableObject>())
				IMounteaDialogueTickableObject::Execute_UnregisterTick(nodeDecorator.DecoratorType, this);
		}
	}
}

void UMounteaDialogueGraphNode::TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime)
{
	NodeTickEvent.Broadcast(this, ParentTick, DeltaTime);
}

void UMounteaDialogueGraphNode::InitializeNode_Implementation(UWorld* InWorld)
{
	SetNewWorld(InWorld);
	
	OnNodeStateChanged.Broadcast(this);
}

void UMounteaDialogueGraphNode::PreProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	const AActor* managerOwner = IMounteaDialogueManagerInterface::Execute_GetOwningActor(Manager.GetObject());
	if (!IsValid(managerOwner))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[PreProcessNode] Invalid owning Actor!"));
		return;
	}
	
	InitializeNode(managerOwner->GetWorld());
	
	Execute_RegisterTick(this, Graph);

	UMounteaDialogueContext* dialogueContext = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager.GetObject());
	TScriptInterface<IMounteaDialogueParticipantInterface> activeParticipant = nullptr;
	if (IsValid(dialogueContext))
		activeParticipant = dialogueContext->ActiveDialogueParticipant;

	for (const auto& nodeDecorator : NodeDecorators)
	{
		if (!IsValid(nodeDecorator.DecoratorType))
			continue;

		nodeDecorator.DecoratorType->InitializeDecorator(
			managerOwner->GetWorld(),
			activeParticipant,
			Manager);
	}
	
	IMounteaDialogueManagerInterface::Execute_NodePrepared(Manager.GetObject());
}

void UMounteaDialogueGraphNode::ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	if (!Manager) return;
	
	if (!GetWorld())
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[ProcessNode] Cannot find World!"));
		return;
	}

	if (!GetGraph())
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[ProcessNode] Invalid owning Graph!"));
		return;
	}
	
	UMounteaDialogueContext* Context = IMounteaDialogueManagerInterface::Execute_GetDialogueContext(Manager.GetObject());
	if (!Context || !UMounteaDialogueContextStatics::IsContextValid(Context))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[ProcessNode] Invalid Dialogue Context!"));
		return;
	}
	
	MounteaDialogueNodeRuntime::ExecuteDecorators(Context);
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueGraphNode::GetNodeDecorators() const
{
	TArray<FMounteaDialogueDecorator> TempReturn;
	TArray<FMounteaDialogueDecorator> Return;
	
	for (auto Itr : NodeDecorators)
	{
		if (Itr.DecoratorType != nullptr)
			TempReturn.AddUnique(Itr);
	}	
	Return = TempReturn;
	return Return;
}

bool UMounteaDialogueGraphNode::CanStartNode_Implementation() const
{
	return true;
}

bool UMounteaDialogueGraphNode::EvaluateDecorators_Implementation() const
{
	if (GetGraph() == nullptr)
	{
		LOG_ERROR(TEXT("[EvaluateDecorators] Graph is null (invalid)!"))
		return false;
	}
	
	bool bSatisfied = true;
	TArray<FMounteaDialogueDecorator> AllDecorators;
	if (bInheritGraphDecorators)
	{
		// Add those Decorators rather than asking Graph to evaluate, because Nodes might introduce specific context
		AllDecorators.Append(GetGraph()->GetGraphDecorators());
	}

	AllDecorators.Append(GetNodeDecorators());

	if (AllDecorators.Num() == 0) return bSatisfied;

	for (auto Itr : AllDecorators)
	{
		if (Itr.EvaluateDecorator() == false) bSatisfied = false;
	}

	return bSatisfied;
}

FText UMounteaDialogueGraphNode::GetNodeTitle_Implementation() const
{
	return NodeTitle;
}

#if WITH_EDITOR

FText UMounteaDialogueGraphNode::GetDescription_Implementation() const
{
	return LOCTEXT("NodeDesc", "Mountea Dialogue Graph Node");
}

FText UMounteaDialogueGraphNode::GetNodeCategory_Implementation() const
{
	return LOCTEXT("NodeCategory", "Mountea Dialogue Tree Node");
}

FString UMounteaDialogueGraphNode::GetNodeDocumentationLink_Implementation() const
{
	return TEXT("https://mountea.tools/docs/DialogueSystem/DialogueNodes/DialogueNode/");
}

FText UMounteaDialogueGraphNode::GetNodeTooltipText_Implementation() const
{
	return FText::Format(LOCTEXT("MounteaDialogueGraphNode_FinalTooltip", "{0}\n\n{1}"), GetDefaultTooltipBody(), NodeTooltipText);
}

FLinearColor UMounteaDialogueGraphNode::GetBackgroundColor() const
{
#if WITH_EDITORONLY_DATA
	return EditorNodeColour;
#else
	return FLinearColor::Black;
#endif
}

void UMounteaDialogueGraphNode::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}

bool UMounteaDialogueGraphNode::CanCreateConnection(UMounteaDialogueGraphNode* Other, EEdGraphPinDirection Direction, FText& ErrorMessage)
{
	// Validate input
	if (!IsValid(Other))
	{
		ErrorMessage = FText::FromString("Invalid Other Node!");
		return false;
	}

	// Enforce max child nodes
	if (Other->GetMaxChildNodes() > -1 && Other->ChildrenNodes.Num() >= Other->GetMaxChildNodes())
	{
		ErrorMessage = FText::Format(
			NSLOCTEXT("MounteaDialogue", "MaxChildrenReached", "{0}: Cannot have more than {1} Children Nodes!"),
			Other->GetNodeTitle(),
			FText::AsNumber(Other->GetMaxChildNodes())
		);
		return false;
	}

	// Check allowed input classes (only applies for output pins)
	if (Direction == EGPD_Output)
	{
		const TArray<TSubclassOf<UMounteaDialogueGraphNode>> allowedClasses = Execute_GetAllowedInputClasses(this);

		const UClass* otherClass = Other->GetClass();

		const bool bIsAllowed = Algo::AnyOf(allowedClasses, [otherClass](const TSubclassOf<UMounteaDialogueGraphNode>& allowedClass)
		{
			return otherClass->IsChildOf(allowedClass);
		});

		if (!bIsAllowed)
		{
			ErrorMessage = FText::FromString("Invalid Node Connection: Target node type is not allowed.\nIf connection is required, please modify the AllowedInputClasses in the Dialogue Configuration.");
			return false;
		}
	}

	return true;
}

bool UMounteaDialogueGraphNode::ValidateNode(FDataValidationContext& Context, const bool RichFormat) const
{
	bool bResult = true;
	if (ParentNodes.Num() == 0 && ChildrenNodes.Num() == 0)
	{
		bResult = false;
		
		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": This Node has no Connections!");

		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": This Node has no Connections!");
		
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	if (bAllowInputNodes && ParentNodes.Num() == 0)
	{
		bResult = false;
		
		const FString RichTextReturn =
		FString("* ").
		Append("<RichTextBlock.Bold>").
		Append(NodeTitle.ToString()).
		Append("</>").
		Append(": This Node requires Inputs, however, none are found!");
		
		const FString TextReturn =
		FString(NodeTitle.ToString()).
		Append(": This Node requires Inputs, however, none are found!");
		
		Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
	}

	// DECORATORS VALIDATION
	{
		TArray<UMounteaDialogueDecoratorBase*> UsedNodeDecorators;
		for (int i = 0; i < NodeDecorators.Num(); i++)
		{
			if (NodeDecorators.IsValidIndex(i) && NodeDecorators[i].DecoratorType && UsedNodeDecorators.Contains(NodeDecorators[i].DecoratorType) == false)
			{
				UsedNodeDecorators.Add(NodeDecorators[i].DecoratorType);
			}
			else
			{
				const FString RichTextReturn =
				FString("* ").
				Append( TEXT("<RichTextBlock.Bold>")).
				Append(GetNodeTitle().ToString()).
				Append(TEXT("</>")).
				Append(": has ").
				Append(TEXT("<RichTextBlock.Bold>invalid</> Node Decorator at Index: ")).
				Append(FString::FromInt(i )).
				Append(".");

				FString TextReturn = GetNodeTitle().ToString();
				TextReturn.
				Append(": has ").
				Append(TEXT("INVALID Node Decorator at Index: ")).
				Append(FString::FromInt(i )).
				Append(".");
		
				Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));

				bResult = false;
			}
		}

		TMap<UClass*, int32> DuplicatedDecoratorsMap;
		for (const auto& Itr : UsedNodeDecorators)
		{
			if (!Itr) continue;

			if (Itr->IsDecoratorStackable()) continue;;
			
			int32 ClassAppearance = 1;
			for (const auto& Itr2 : UsedNodeDecorators)
			{
				if (Itr != Itr2 && Itr->GetClass() == Itr2->GetClass())
				{
					auto A = Itr->GetClass()->GetName();
					ClassAppearance++;
				}
			}

			if (ClassAppearance > 1 && DuplicatedDecoratorsMap.Contains(Itr->GetClass()) == false)
			{
				DuplicatedDecoratorsMap.Add(Itr->GetClass(), ClassAppearance);
			}
		}

		if (DuplicatedDecoratorsMap.Num() > 0)
		{
			for (const auto& Itr : DuplicatedDecoratorsMap)
			{
				bResult = false;
		
				const FString RichTextReturn =
				FString("* ").
				Append("<RichTextBlock.Bold>").
				Append(NodeTitle.ToString()).
				Append("</>").
				Append(": has Node Decorator ").
				Append("<RichTextBlock.Bold>").
				Append(Itr.Key->GetName().LeftChop(2)).
				Append("</> ").
				Append(FString::FromInt(Itr.Value)).
				Append("x times! Please, avoid duplicates!");
			
				const FString TextReturn =
				FString(NodeTitle.ToString()).
				Append(NodeTitle.ToString()).
				Append(": has Node Decorator ").
				Append( Itr.Key->GetName().LeftChop(2)).
				Append(" ").
				Append(FString::FromInt(Itr.Value)).
				Append("x times! Please, avoid duplicates!");
		
				Context.AddError(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
			}
		}

		for (auto Itr : GetNodeDecorators())
		{
			if (Itr.DecoratorType
				&& (Itr.DecoratorType->IsA<UMounteaDialogueDecorator_OnlyFirstTime>()
					|| Itr.DecoratorType->IsA<UMounteaDialogueDecorator_OverrideOnlyFirstTime>()))
			{
				const FString RichTextWarning =
				FString(TEXT("* ")).
				Append(TEXT("<RichTextBlock.Bold>")).
				Append(NodeTitle.ToString()).
				Append(TEXT("</>: Decorator ")).
				Append(TEXT("<RichTextBlock.Bold>")).
				Append(Itr.DecoratorType->GetClass()->GetDisplayNameText().ToString()).
				Append(TEXT("</> is deprecated. Use incoming edge condition ")).
				Append(TEXT("<RichTextBlock.Bold>Only First Time</> for traversal gating. Use condition negation for repeat-path branches."));

				const FString TextWarning =
				FString(NodeTitle.ToString()).
				Append(TEXT(": Decorator ")).
				Append(Itr.DecoratorType->GetClass()->GetDisplayNameText().ToString()).
				Append(TEXT(" is deprecated. Use incoming edge condition Only First Time for traversal gating. Use condition negation for repeat-path branches."));

				Context.AddWarning(FText::FromString(RichFormat ? RichTextWarning : TextWarning));
			}

			TArray<FText> DecoratorErrors;
			if (Itr.ValidateDecorator(DecoratorErrors) == false)
			{
				for (auto Error : DecoratorErrors)
				{
					const FString ErrorTextRich =
					FString("* ").
					Append("<RichTextBlock.Bold>").
					Append(NodeTitle.ToString()).
					Append("</>: ").
					Append(FString(Error.ToString()));

					auto ClassName = GetClass()->GetDisplayNameText().ToString();
					const FString ErrorTextSimple =
					ClassName.
					Append(": ").
					Append(FString(Error.ToString()));
		
					Context.AddError(FText::FromString(RichFormat ? ErrorTextRich : ErrorTextSimple));

					bResult = false;
				}
			}
		}
	}
	
	return bResult;
}

void UMounteaDialogueGraphNode::OnPasted()
{
	NodeGUID = FGuid::NewGuid();

	ParentNodes.Empty();
	ChildrenNodes.Empty();
	Edges.Empty();
}

FText UMounteaDialogueGraphNode::GetDefaultTooltipBody() const
{
	const FText InheritsValue = bInheritGraphDecorators ? LOCTEXT("True","Yes") : LOCTEXT("False","No");
	const FText Inherits = FText::Format(LOCTEXT("UMounteaDialogueGraphNode_InheritsTooltip", "Inherits Graph Decorators: {0}"), InheritsValue);
	FText ImplementsNumber;
	if (NodeDecorators.Num() == 0) ImplementsNumber = LOCTEXT("None","-");
	else ImplementsNumber = FText::FromString(FString::FromInt(NodeDecorators.Num()));
	
	const FText Implements = FText::Format(LOCTEXT("UMounteaDialogueGraphNode_ImplementsTooltip", "Implements Decorators: {0}"), ImplementsNumber);
	
	return FText::Format(LOCTEXT("UMounteaDialogueGraphNode_BaseTooltip", "{0} ({1})\n\n{2}\n{3}\nNode Execution Order: {4}"),
		NodeTypeName,
		FText::FromString(NodeGUID.ToString(EGuidFormats::DigitsWithHyphensLower)),
		Inherits, Implements, ExecutionOrder);
}

#endif

#undef LOCTEXT_NAMESPACE
