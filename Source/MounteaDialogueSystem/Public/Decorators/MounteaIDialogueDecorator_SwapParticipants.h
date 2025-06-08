// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaIDialogueDecorator_SwapParticipants.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueManagerInterface;

/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support swap Dialogue Participants, like switching whether the Player or NPC is the Active one.
 */
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Swap Participants")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_SwapParticipants : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:

	virtual void CleanupDecorator_Implementation() override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Swap-Participants"); }

protected:

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category="Settings")
	FGameplayTag NewParticipantTag;
	
private:
	
	UPROPERTY()
	TObjectPtr<UMounteaDialogueContext> Context = nullptr;
};