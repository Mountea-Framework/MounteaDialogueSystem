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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueContextUpdated, UMounteaDialogueContext*, Context);

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueManagerInterface
{
	GENERATED_BODY()

public:

	virtual UMounteaDialogueContext* GetDialogueContext() const = 0;
	virtual void SetDialogueContext(UMounteaDialogueContext* NewContext) = 0;

	virtual FDialogueInitialized& GetDialogueInitializedEventHandle() = 0;
	virtual FDialogueContextUpdated& GetDialogueContextUpdatedEventHande() = 0;
};
