// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/HUD/MounteaDialogueUIBaseInterface.h"
#include "Interfaces/UMG/MounteaDialogueRowInterface.h"
#include "MounteaDialogueRow.generated.h"

/**
 * UMounteaDialogueRow
 *
 * A UserWidget class that implements the 'MounteaDialogueRowInterface', providing functionalities for dialogue rows in the Mountea Dialogue System.
 */
UCLASS(DisplayName="Mountea Dialogue Row", ClassGroup=Mountea)
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueRow : public UUserWidget, public IMounteaDialogueRowInterface, public IMounteaDialogueUIBaseInterface
{
	GENERATED_BODY()

public:
	
	UMounteaDialogueRow(const FObjectInitializer& ObjectInitializer);

protected:
	
	/**
	 * Called every time the text is updated during the typewriter effect.
	 * @param UpdatedText		The text that has been updated.
	 * @param Alpha					The progress of the typewriter effect (0 to 1).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Monutea|Dialogue")
	void				OnTypeWriterEffectUpdated								(const FText& UpdatedText, float Alpha);
	
	/**
     * Called when the typewriter effect is finished.
     */
	UFUNCTION(BlueprintImplementableEvent, Category="Monutea|Dialogue")
	void				OnTypeWriterEffectFinished								();
	
	UFUNCTION()
	void				UpdateTypeWriterEffect_Callback							(const FText& SourceText, int32 CurrentCharacterIndex, float TotalDuration);
	UFUNCTION()
	void				CompleteTypeWriterEffect_Callback						(const FText& SourceText);


protected:

	// Timer Handle responsible for the whole Duration of the effect
	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	FTimerHandle								TimerHandle_TypeWriterDuration;

	// Timer Handle responsible for each character update
	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	FTimerHandle								TimerHandle_TypeWriterUpdateInterval;

protected:
	
	// IMounteaDialogueUIBaseInterface implementation
	virtual		bool				BindEvents_Implementation					()																override
	{return true;};
	virtual		bool				UnbindEvents_Implementation					() 																override
	{return true;};
	virtual		void				ProcessStringCommand_Implementation			(const FString& Command, UObject* OptionalPayload = nullptr)	override
	{};
	virtual		void				ApplyTheme_Implementation					()																override
	{};
	
protected:
	
	// IMounteaDialogueRowInterface implementation
	virtual		FWidgetDialogueRow	GetDialogueWidgetRowData_Implementation		() const														override;
	virtual		void				SetNewWidgetDialogueRowData_Implementation	(const FWidgetDialogueRow& NewData)								override;
	virtual		void				ResetWidgetDialogueRow_Implementation		()																override;
	virtual		void				InitializeWidgetDialogueRow_Implementation	()																override;
	virtual		void				StopTypeWriterEffect_Implementation			()																override;
	virtual		void				StartTypeWriterEffect_Implementation		(const FText& SourceText, float Duration)						override;
	virtual		void				EnableTypeWriterEffect_Implementation		(bool bEnable)													override;

protected:

	/**
	 * Dialogue Row Data.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue", meta=(ExposeOnSpawn=true))
	FWidgetDialogueRow					DialogueRowData;

	/**
	 * Defines whether the Row is using Type-Writer effect to display text.
	 */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category="Mountea|Dialogue", meta=(ExposeOnSpawn=true))
	uint8								bUseTypeWriterEffect	: 1;

	/**
	 * Event triggered upon updating 'bUseTypeWriterEffect'
	 */
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly, Category="Mountea|Dialogue", meta=(CustomTag="MounteaK2Delegate"))
	FOnTypeWriterEffectChanged			OnTypeWriterEffectChanged;
};
