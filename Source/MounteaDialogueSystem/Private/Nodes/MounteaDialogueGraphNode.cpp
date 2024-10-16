// All rights reserved Dominik Pavlicek 2023

#include "Nodes/MounteaDialogueGraphNode.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Helpers/MounteaDialogueSystemBFC.h"

#define LOCTEXT_NAMESPACE "MounteaDialogueNode"

UMounteaDialogueGraphNode::UMounteaDialogueGraphNode(): Graph(nullptr), OwningWorld(nullptr)
{
	NodeGUID = FGuid::NewGuid();
	bInheritGraphDecorators = true;

#if WITH_EDITORONLY_DATA
	CompatibleGraphType = UMounteaDialogueGraph::StaticClass();

	BackgroundColor = FLinearColor::Black;

	bAllowInputNodes = true;
	bAllowOutputNodes = true;

	bAllowCopy = true;
	bAllowCut = true;
	bAllowDelete = true;
	bAllowPaste = true;
	bAllowManualCreate = true;
	bCanRenameNode = true;

	NodeTypeName = LOCTEXT("MounteaDialogueNode_InternalName", "MounteaDialogueGraphNode");
	NodeTooltipText = LOCTEXT("MounteaDialogueNode_Tooltip",
	                          "Mountea Dialogue Base Node.\n\nChild Nodes provide more Information.");
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

void UMounteaDialogueGraphNode::SetNewWorld(UWorld* NewWorld)
{
	if (!NewWorld) return;
	if (NewWorld == OwningWorld) return;

	OwningWorld = NewWorld;
}

void UMounteaDialogueGraphNode::RegisterTick_Implementation( const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().AddUniqueDynamic(this, &UMounteaDialogueGraphNode::TickMounteaEvent);
	}
}

void UMounteaDialogueGraphNode::UnregisterTick_Implementation( const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable)
{
	if (ParentTickable.GetObject() && ParentTickable.GetInterface())
	{
		ParentTickable->GetMounteaDialogueTickHandle().RemoveDynamic(this, &UMounteaDialogueGraphNode::TickMounteaEvent);
	}
}

void UMounteaDialogueGraphNode::TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime)
{
	NodeTickEvent.Broadcast(this, ParentTick, DeltaTime);
}

void UMounteaDialogueGraphNode::InitializeNode_Implementation(UWorld* InWorld)
{
	SetNewWorld(InWorld);

	if (Graph) SetNodeIndex(Graph->AllNodes.Find(this));
}

void UMounteaDialogueGraphNode::PreProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager)
{
	// Child Classes Implementations
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
	
	UMounteaDialogueContext* Context = Manager->GetDialogueContext();
	if (!Context || !UMounteaDialogueSystemBFC::IsContextValid(Context))
	{
		Manager->GetDialogueFailedEventHandle().Broadcast(TEXT("[ProcessNode] Invalid Dialogue Context!"));
		return;
	}
	
	UMounteaDialogueSystemBFC::ExecuteDecorators(this, Context);
	
	Manager->GetDialogueNodeStartedEventHandle().Broadcast(Context);
}

TArray<FMounteaDialogueDecorator> UMounteaDialogueGraphNode::GetNodeDecorators() const
{
	TArray<FMounteaDialogueDecorator> TempReturn;
	TArray<FMounteaDialogueDecorator> Return;
	
	for (auto Itr : NodeDecorators)
	{
		if (Itr.DecoratorType != nullptr)
		{
			TempReturn.AddUnique(Itr);
		}
	}

	/* TODO: Cleanup duplicates
	for (auto Itr : TempReturn)
	{
		
	}
	*/
	
	Return = TempReturn;
	return Return;
}

bool UMounteaDialogueGraphNode::CanStartNode_Implementation() const
{
	return EvaluateDecorators();
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

void UMounteaDialogueGraphNode::SetNodeIndex(const int32 NewIndex)
{
	check(NewIndex>INDEX_NONE);
	NodeIndex = NewIndex;
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
	return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Dialogue-Nodes");
}

FText UMounteaDialogueGraphNode::GetNodeTooltipText_Implementation() const
{
	return FText::Format(LOCTEXT("MounteaDialogueGraphNode_FinalTooltip", "{0}\n\n{1}"), GetDefaultTooltipBody(), NodeTooltipText);
}

FLinearColor UMounteaDialogueGraphNode::GetBackgroundColor() const
{
	return BackgroundColor;
}

void UMounteaDialogueGraphNode::SetNodeTitle(const FText& NewTitle)
{
	NodeTitle = NewTitle;
}

bool UMounteaDialogueGraphNode::CanCreateConnection(UMounteaDialogueGraphNode* Other, enum EEdGraphPinDirection Direction, FText& ErrorMessage)
{
	if (Other == nullptr)
	{
		ErrorMessage = FText::FromString("Invalid Other Node!");
	}

	if (Other->GetMaxChildNodes() > -1 && Other->ChildrenNodes.Num() >= Other->GetMaxChildNodes())
	{
		const FString TextReturn =
		FString(Other->GetNodeTitle().ToString()).
		Append(": Cannot have more than ").Append(FString::FromInt(Other->GetMaxChildNodes())).Append(" Children Nodes!");

		ErrorMessage = FText::FromString(TextReturn);
		return false;
	}

	if (Direction == EGPD_Output)
	{
		
		// Fast checking for native classes
		if ( AllowedInputClasses.Contains(Other->GetClass()) )
		{
			return true;
		}

		// Slower iterative checking for child classes
		for (auto Itr : AllowedInputClasses)
		{
			if (Other->GetClass()->IsChildOf(Itr))
			{
				return true;
			}
		}
		
		ErrorMessage = FText::FromString("Invalid Node Connection!");
		return false;
	}

	return true;
}

bool UMounteaDialogueGraphNode::ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat)
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
		
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
		
		ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
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
		
				ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));

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
		
				ValidationsMessages.Add(FText::FromString(RichFormat ? RichTextReturn : TextReturn));
			}
		}

		for (auto Itr : GetNodeDecorators())
		{
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
		
					ValidationsMessages.Add(FText::FromString(RichFormat ? ErrorTextRich : ErrorTextSimple));

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
	
	return FText::Format(LOCTEXT("UMounteaDialogueGraphNode_BaseTooltip", "{0} ({1})\n\n{2}\n{3}\nNode Execution Order: {4}\nNode Index: {5}"),
		NodeTypeName,
		FText::FromString(NodeGUID.ToString(EGuidFormats::DigitsWithHyphensLower)),
		Inherits, Implements, ExecutionOrder, NodeIndex);
}

#endif

#undef LOCTEXT_NAMESPACE
