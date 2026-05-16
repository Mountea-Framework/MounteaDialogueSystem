// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "MounteaDialogueLocalMonologueSubsystem.generated.h"

class UMounteaDialogueLocalMonologueComponent;

UCLASS()
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueLocalMonologueSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:

	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Setter"))
	bool TryStartMonologue(
		UMounteaDialogueLocalMonologueComponent* Component,
		const FDialogueStartRequest& Request,
		FString& OutErrorMessage);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Setter"))
	void StopMonologue(UMounteaDialogueLocalMonologueComponent* Component);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Validate"))
	bool HasActiveLocalMonologue() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Validate"))
	bool CanStartLocalMonologue(FString& OutReason) const;

	void RegisterMonologueComponent(UMounteaDialogueLocalMonologueComponent* Component);
	void UnregisterMonologueComponent(UMounteaDialogueLocalMonologueComponent* Component);
	void ReleaseMonologueLock(UMounteaDialogueLocalMonologueComponent* Component, const FGuid& SessionGUID);

private:

	bool HasAnyActiveRegularManager(FString& OutReason) const;

private:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMounteaDialogueLocalMonologueComponent>> RegisteredMonologueComponents;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMounteaDialogueLocalMonologueComponent> ActiveMonologueComponent;

	UPROPERTY(Transient)
	FGuid ActiveMonologueSessionGUID;
};
