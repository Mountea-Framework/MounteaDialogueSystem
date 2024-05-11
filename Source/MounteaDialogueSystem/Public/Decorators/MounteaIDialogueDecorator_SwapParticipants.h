// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaIDialogueDecorator_SwapParticipants.generated.h"

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

	virtual void InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& OwningManager) override;
	virtual void CleanupDecorator_Implementation() override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Swap-Participants"); }

private:
	
	class UMounteaDialogueContext* Context = nullptr;
	TScriptInterface<IMounteaDialogueManagerInterface> Manager = nullptr;
};