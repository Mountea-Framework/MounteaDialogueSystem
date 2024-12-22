// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaFocusableContainerWidgetInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMounteaFocusedElementChanged, UUserWidget*, Widget, const bool, IsFocused);

UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaFocusableContainerWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaFocusableContainerWidgetInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Focus")
	UUserWidget* GetFocusedElement() const;
	virtual UUserWidget* GetFocusedElement_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Focus")
	void SetFocusedElement(const UUserWidget* FocusedElement);
	virtual void SetFocusedElement_Implementation(const UUserWidget* FocusedElement) = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Focus")
	UUserWidget* GetLastFocusedElement() const;
	virtual UUserWidget* GetLastFocusedElement_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|Focus")
	void SetLastFocusedElement(const UUserWidget* FocusedElement);
	virtual void SetLastFocusedElement_Implementation(const UUserWidget* FocusedElement) = 0;
};
