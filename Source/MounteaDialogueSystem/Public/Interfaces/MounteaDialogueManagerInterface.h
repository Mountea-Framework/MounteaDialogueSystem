// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueManagerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UMounteaDialogueManagerInterface : public UInterface
{
	GENERATED_BODY()
};

class UMounteaDialogueContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueInitialized, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdated, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueNodeEvent, UMounteaDialogueContext*, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueRowEvent, UMounteaDialogueContext*, Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueFailed, const FString&, ErrorMessage);

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	virtual void StartDialogue() = 0;
	virtual void CloseDialogue() = 0;
	virtual void ProcessNode() = 0;
	virtual void ProcessNode_Complete() = 0;
	virtual void ProcessNode_Dialogue() = 0;

	virtual void StartExecuteDialogueRow() = 0;
	virtual void FinishedExecuteDialogueRow() = 0;

	virtual UMounteaDialogueContext* GetDialogueContext() const = 0;
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) = 0;

	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueStartedEventHandle() = 0;
	virtual FDialogueEvent& GetDialogueClosedEventHandle() = 0;
	
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() = 0;

	virtual FDialogueNodeEvent& GetDialogueNodeStartedEventHandle() = 0;
	virtual FDialogueNodeEvent& GetDialogueNodeFinishedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowStartedEventHandle() = 0;
	virtual FDialogueRowEvent& GetDialogueRowFinishedEventHandle() = 0;

	virtual FDialogueFailed& GetDialogueFailedEventHandle() = 0;
};
