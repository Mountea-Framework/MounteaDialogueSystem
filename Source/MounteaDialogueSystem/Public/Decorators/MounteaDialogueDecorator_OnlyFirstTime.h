// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaDialogueDecorator_OnlyFirstTime.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueManagerInterface;
/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support for triggering logic if Owning node has been entered for the first time.
 * Base class that should expanded with Child Classes.
 *
 * Child Classes must implement logic!
 * Should do something only if its Owning Node has never been called before.
 *
 * Provides validations and data for Child classes.
 * By itself has no logic within.
 *
 * For proper usage, save Dialogue Participants when saving game!
 */
UCLASS( Abstract, BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Only First Time Base")
class MOUNTEADIALOGUESYSTEM_API  UMounteaDialogueDecorator_OnlyFirstTime : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:

	virtual void InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager) override;
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual bool EvaluateDecorator_Implementation() override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Only-First-Time-Base"); }

	// Returns whether Owning Node has never been called before.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorators")
	bool IsFirstTime() const;

protected:

	UMounteaDialogueContext* GetContext() const
	{ return Context; };

private:
	
	TObjectPtr<UMounteaDialogueContext> Context = nullptr;
	
};
