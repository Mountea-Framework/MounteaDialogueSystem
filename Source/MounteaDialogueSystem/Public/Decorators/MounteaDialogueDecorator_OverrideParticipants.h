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
 * Implements native support for selecting Dialogue Row data.
 */
UCLASS( NotBlueprintable, BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea, Dialogue"), DisplayName="Override Dialogue Participants")
class MOUNTEADIALOGUESYSTEM_API  UMounteaDialogueDecorator_OverrideParticipants : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:

	virtual void InitializeDecorator_Implementation(UWorld* World) override;
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual const FString GetDecoratorDocumentationLink_Implementation() override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Override-Dialogue-Participants"); }

protected:

	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault))
	bool bOverridePlayerParticipant;
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, EditCondition="bOverridePlayerParticipant"))
	TWeakObjectPtr<AActor>NewPlayerParticipant;

	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault))
	bool bOverrideDialogueParticipant;
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, EditCondition="bOverrideDialogueParticipant"))
	TWeakObjectPtr<AActor>NewDialogueParticipant;

	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault))
	bool bOverrideActiveParticipant;
	UPROPERTY(SaveGame, Category="Override", EditAnywhere, BlueprintReadOnly, meta=(DisplayThumbnail=false, NoResetToDefault, EditCondition="bOverrideActiveParticipant"))
	TWeakObjectPtr<AActor>NewActiveParticipant;

private:
	
	UMounteaDialogueContext* Context = nullptr;
	TScriptInterface<IMounteaDialogueManagerInterface> Manager = nullptr;
};
