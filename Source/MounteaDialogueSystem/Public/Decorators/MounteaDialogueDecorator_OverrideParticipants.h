// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Data/MounteaDialogueContext.h"
#include "MounteaDialogueDecorator_OverrideParticipants.generated.h"

class IMounteaDialogueManagerInterface;
/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support for selecting new Participants for selected Dialogue.
 * Useful when Multiple participants are present, for example:
 * * 2 NPCs are talking
 *
 * This Decorator allows skipping from one NPC to another.
 */
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Override Dialogue Participants")
class MOUNTEADIALOGUESYSTEM_API  UMounteaDialogueDecorator_OverrideParticipants : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:
	
	virtual void InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager) override;
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Override-Dialogue-Participants"); }

protected:

	// Enables setting NewPlayerParticipant to a value
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, InlineEditConditionToggle))
	bool bOverridePlayerParticipant;
	// Non-nullable reference to Actor from Level. Either must implement 'MounteaDialogueParticipantInterface' or must contain at least 1 component which implements such interface.
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, EditCondition="bOverridePlayerParticipant"))
	TSoftObjectPtr<AActor>NewPlayerParticipant;

	// Enables setting NewDialogueParticipant to a value
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, InlineEditConditionToggle))
	bool bOverrideDialogueParticipant;
	// Non-nullable reference to Actor from Level. Either must implement 'MounteaDialogueParticipantInterface' or must contain at least 1 component which implements such interface.
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, EditCondition="bOverrideDialogueParticipant"))
	TSoftObjectPtr<AActor>NewDialogueParticipant;

	// Enables setting NewActiveParticipant to a value
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(NoResetToDefault, InlineEditConditionToggle))
	bool bOverrideActiveParticipant;
	// Non-nullable reference to Actor from Level. Either must implement 'MounteaDialogueParticipantInterface' or must contain at least 1 component which implements such interface.
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, EditCondition="bOverrideActiveParticipant"))
	TSoftObjectPtr<AActor>NewActiveParticipant;

private:
	
	UPROPERTY()
	TObjectPtr<UMounteaDialogueContext> Context = nullptr;

	TScriptInterface<IMounteaDialogueParticipantInterface> Override_PlayerParticipantInterface = nullptr;
	TScriptInterface<IMounteaDialogueParticipantInterface> Override_ParticipantInterface = nullptr;
	TScriptInterface<IMounteaDialogueParticipantInterface> Override_ActiveParticipantInterface = nullptr;

private:

	bool ValidateInterfaceActor(TSoftObjectPtr<AActor> Actor, TArray<FText>& ValidationMessages) const;
	TScriptInterface<IMounteaDialogueParticipantInterface> GetParticipantFromActorRef(const TSoftObjectPtr<AActor>& Actor) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
};

