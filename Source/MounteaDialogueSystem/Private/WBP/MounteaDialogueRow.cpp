// All rights reserved Dominik Morse (Pavlicek) 2024.


#include "WBP/MounteaDialogueRow.h"
#include "TimerManager.h"

UMounteaDialogueRow::UMounteaDialogueRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bUseTypeWriterEffect(false)
{
}

void UMounteaDialogueRow::StopTypeWriterEffect_Implementation()
{
	CompleteTypeWriterEffect_Callback(DialogueRowData.DialogueRowBody);
}

void UMounteaDialogueRow::StartTypeWriterEffect_Implementation(const FText& SourceText, float Duration)
{
	if (TimerHandle_TypeWriterDuration.IsValid() || TimerHandle_TypeWriterUpdateInterval.IsValid())
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	const FString sourceTextString = SourceText.ToString();
	const int32 sourceTextLength = sourceTextString.Len();
	const float updateInterval = Duration / sourceTextLength;

	FTimerDelegate TimerDelegate_TypeWriterUpdateInterval;
	TimerDelegate_TypeWriterUpdateInterval.BindUFunction(this, "UpdateTypeWriterEffect_Callback", SourceText, 0, Duration);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_TypeWriterUpdateInterval, TimerDelegate_TypeWriterUpdateInterval, updateInterval, false);

	FTimerDelegate TimerDelegate_TypeWriterDuration;
	TimerDelegate_TypeWriterDuration.BindUFunction(this, "CompleteTypeWriterEffect_Callback", SourceText);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_TypeWriterDuration, TimerDelegate_TypeWriterDuration, Duration, false);
}

void UMounteaDialogueRow::EnableTypeWriterEffect_Implementation(bool bEnable)
{
	if (bEnable != bUseTypeWriterEffect)
	{
		bUseTypeWriterEffect = bEnable;

		OnTypeWriterEffectChanged.Broadcast(bUseTypeWriterEffect);
	}
}

void UMounteaDialogueRow::UpdateTypeWriterEffect_Callback(const FText& SourceText, int32 CurrentCharacterIndex, float TotalDuration)
{
	const FString sourceTextString = SourceText.ToString();
	const int32 totalCharacters = sourceTextString.Len();
	
	if (!GetWorld() || CurrentCharacterIndex >= totalCharacters)
	{
		return;
	}

	CurrentCharacterIndex++;

	FString updatedSourceString = sourceTextString.Left(CurrentCharacterIndex);
	FText updatedSourceText = FText::FromString(updatedSourceString);

	const float ElapsedTime = GetWorld()->GetTimerManager().GetTimerElapsed(TimerHandle_TypeWriterDuration);
	const float Alpha = ElapsedTime / TotalDuration;

	OnTypeWriterEffectUpdated(updatedSourceText, Alpha);
	
	FTimerDelegate TimerDelegate_TypeWriterUpdateInterval;
	TimerDelegate_TypeWriterUpdateInterval.BindUFunction(this, "UpdateTypeWriterEffect_Callback", SourceText, CurrentCharacterIndex, TotalDuration);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle_TypeWriterUpdateInterval, TimerDelegate_TypeWriterUpdateInterval, TotalDuration / totalCharacters, false);
}

void UMounteaDialogueRow::CompleteTypeWriterEffect_Callback(const FText& SourceText)
{
	if (!GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TypeWriterUpdateInterval);
	GetWorld()->GetTimerManager().ClearTimer(TimerHandle_TypeWriterDuration);

	OnTypeWriterEffectUpdated(SourceText, 1.0f);
	OnTypeWriterEffectFinished();
}

FWidgetDialogueRow UMounteaDialogueRow::GetDialogueWidgetRowData_Implementation() const
{
	return DialogueRowData;
}

void UMounteaDialogueRow::SetNewWidgetDialogueRowData_Implementation(const FWidgetDialogueRow& NewData)
{
	if (DialogueRowData != NewData)
	{
		DialogueRowData = NewData;
	}
}

void UMounteaDialogueRow::ResetWidgetDialogueRow_Implementation()
{
	DialogueRowData.ResetRow();
}

void UMounteaDialogueRow::InitializeWidgetDialogueRow_Implementation()
{
	// ...
}
