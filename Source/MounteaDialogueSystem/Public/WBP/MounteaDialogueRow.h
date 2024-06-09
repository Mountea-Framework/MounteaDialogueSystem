// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/UMG/MounteaDialogueRowInterface.h"
#include "MounteaDialogueRow.generated.h"

/**
 * UMounteaDialogueRow
 *
 * A UserWidget class that implements the 'MounteaDialogueRowInterface', providing functionalities for dialogue rows in the Mountea Dialogue System.
 */
UCLASS(DisplayName="Mountea Dialogue Row", ClassGroup=Mountea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueRow : public UUserWidget, public IMounteaDialogueRowInterface
{
	GENERATED_BODY()

protected:
	
	// IMounteaDialogueWBPInterface implementation
	virtual		FWidgetDialogueRow	GetDialogueWidgetRowData_Implementation			() const															override;
	virtual		void								SetNewWidgetDialogueRowData_Implementation	(const FWidgetDialogueRow& NewData)		override;
	virtual		void								ResetWidgetDialogueRow_Implementation				()																		override;
	virtual		void								InitializeWidgetDialogueRow_Implementation			()																		override;

protected:

	/**
	 * Dialogue Row Data.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue", meta=(ExposeOnSpawn=true))
	FWidgetDialogueRow					DialogueRowData;
};
