// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
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
#include "MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "MounteaDialogueGraphNode_DialogueNode.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), DisplayName="Dialogue Speech Node", 
	meta=(ToolTip="Mountea Dialogue Tree: Dialogue Speech Node"))
class MOUNTEADIALOGUESYSTEM_API
	UMounteaDialogueGraphNode_DialogueNode : public UMounteaDialogueGraphNode, public IMounteaDialogueSpeechDataInterface
{
	GENERATED_BODY()
	
public:
	
	UMounteaDialogueGraphNode_DialogueNode();
	
	inline virtual FDialogueRow GetSpeechData_Implementation() const override
	{ return Dialogue; };
	virtual bool SetSpeechData_Implementation(const FDialogueRow& NewSpeechData) override;
	
protected:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dialogue",
		meta=(NoResetToDefault),
		meta=(ShowOnlyInnerProperties))
	FDialogueRow Dialogue;
	
public:
	
#if WITH_EDITORONLY_DATA

	/**
	 * Shows read-only Texts with localization of selected Dialogue Row.
	 */
	UPROPERTY(Transient, VisibleAnywhere, Category="Base", 
		meta=(MultiLine=true, ShowOnlyInnerProperties),
		meta=(HiddenInGraph))
	TArray<FText> Preview;

	FSimpleDelegate PreviewsUpdated;

#endif
	
public:
	
	virtual FText GetNodeTitle_Implementation() const override;
	
#if WITH_EDITOR
	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
	
	virtual bool ValidateNode(FDataValidationContext& Context, const bool RichFormat) const override;
#endif
};
