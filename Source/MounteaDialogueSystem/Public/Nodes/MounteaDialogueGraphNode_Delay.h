// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "MounteaDialogueGraphNode.h"
#include "MounteaDialogueGraphNode_Delay.generated.h"

/**
 * Mountea Dialogue Delay Node.
 * 
 * ...
 */
UCLASS(ClassGroup=("Mountea|Dialogue"), NotBlueprintable, DisplayName="Delay Node", meta=(ToolTip="Mountea Dialogue Tree: Delay Node"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNode_Delay : public UMounteaDialogueGraphNode
{
	GENERATED_BODY()

public:

	UMounteaDialogueGraphNode_Delay();

	virtual void ProcessNode_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& Manager) override;
	virtual FText GetNodeTitle_Implementation() const override;

protected:

	UPROPERTY(SaveGame, Category="Mountea|Dialogue", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault,Units = "s", UIMin = 0.01, ClampMin = 0.01))
	int32 DelayDuration;

private:

	UFUNCTION()
	void OnDelayDurationExpired(const TScriptInterface<IMounteaDialogueManagerInterface>& MounteaDialogueManagerInterface);
	
private:

	FTimerHandle TimerHandle_NodeDelay;

#if WITH_EDITOR
	virtual FText GetDescription_Implementation() const override;
	virtual FText GetNodeCategory_Implementation() const override;
	
	virtual bool ValidateNode(TArray<FText>& ValidationsMessages, const bool RichFormat) override;
#endif
};
