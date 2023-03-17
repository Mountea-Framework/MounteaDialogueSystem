// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "MounteaDialogueDecorator_SaveNodeAsStart.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueManagerInterface;
/**
 *	Mountea Dialogue Decorators
 *
 * Implements native support for Saving its Owning Node as new Start Node.
 */
UCLASS( BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea","Dialogue"), DisplayName="Save Node as Start Node")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecorator_SaveNodeAsStart : public UMounteaDialogueDecoratorBase
{
	GENERATED_BODY()

public:

	virtual void InitializeDecorator_Implementation(UWorld* World) override;
	virtual void CleanupDecorator_Implementation() override;
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages) override;
	virtual void ExecuteDecorator_Implementation() override;

	virtual  FString GetDecoratorDocumentationLink_Implementation() const override
	{ return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Decorator:-Set-Node-as-Start"); }

private:
	
	UMounteaDialogueContext* Context = nullptr;
	TScriptInterface<IMounteaDialogueManagerInterface> Manager = nullptr;
};
