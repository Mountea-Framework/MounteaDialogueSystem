// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "K2Nodes/MounteaK2Node_StartDialogue.h"

#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "Interfaces/MounteaDialogueManagerInterface.h"

#define LOCTEXT_NAMESPACE "UMounteaK2Node_StartDialogue"

void UMounteaK2Node_StartDialogue::AllocateDefaultPins()
{
	// Set the function to be executed by this node
	SetFunction();

	Super::AllocateDefaultPins();
}

FSlateIcon UMounteaK2Node_StartDialogue::GetIconAndTint(FLinearColor& OutColor) const
{
	OutColor = FLinearColor::White;
	return FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.RefPinIcon");
}

void UMounteaK2Node_StartDialogue::SetFunction()
{
	Function = UMounteaDialogueManagerInterface::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(IMounteaDialogueManagerInterface, StartDialogue));
	if (Function)
	{
		this->FunctionReference.SetExternalMember(Function->GetFName(), Function->GetOuterUClass());
	}
}

FText UMounteaK2Node_StartDialogue::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("UMounteaK2Node_StartDialogue_Title", "Start Dialogue\nMountea Framework");
}

FText UMounteaK2Node_StartDialogue::GetTooltipText() const
{
	return Function ? Function->GetToolTipText(true) : LOCTEXT("UMounteaK2Node_StartDialogue_Tooltip", "Starts the Dialogue if possible..");
}

void UMounteaK2Node_StartDialogue::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);
}

FLinearColor UMounteaK2Node_StartDialogue::GetNodeTitleColor() const
{
	return FLinearColor::Red;
}

FLinearColor UMounteaK2Node_StartDialogue::GetNodeBodyTintColor() const
{
	return FLinearColor::Black;
}

FText UMounteaK2Node_StartDialogue::GetMenuCategory() const
{
	return FText::FromString(TEXT("Mountea|Dialogue"));
}


void UMounteaK2Node_StartDialogue::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto localFunction = Function;
	if (!localFunction)
	{
		localFunction = UMounteaDialogueManagerInterface::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(IMounteaDialogueManagerInterface, StartDialogue));
	}
	if (localFunction && (localFunction->HasMetaData(TEXT("CustomTag")) && localFunction->GetMetaData(TEXT("CustomTag")) == TEXT("MounteaK2Candidate")))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(UMounteaK2Node_StartDialogue::StaticClass());
		ActionRegistrar.AddBlueprintAction(localFunction->GetOwnerClass(), NodeSpawner);
		
		localFunction->SetMetaData(TEXT("BlueprintInternalUseOnly"), TEXT("true"));
		localFunction->SetMetaData(TEXT("Keywords"), TEXT("start,execute"));

		localFunction->RemoveMetaData(TEXT("BlueprintCallable"));
	}
}

#undef LOCTEXT_NAMESPACE



