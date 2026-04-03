// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/MounteaDialogueContext.h"
#include "MounteaDialogueParticipantUserInterfaceComponent.generated.h"

class UMounteaDialogueContext;
class IMounteaDialogueManagerInterface;

UCLASS(ClassGroup=(Mountea), Blueprintable, 
	hideCategories=(Collision, AssetUserData, Cooking, Activation, Rendering, Sockets), 
	AutoExpandCategories=("Mountea", "Dialogue"), 
	meta=(BlueprintSpawnableComponent, DisplayName = "Mountea Dialogue User Interface Component"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueParticipantUserInterfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMounteaDialogueParticipantUserInterfaceComponent();

protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
		meta=(CustomTag="MounteaK2Setter"))
	void RequestManager();
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
			meta=(CustomTag="MounteaK2Setter"))
	void SetupBindings();
	
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Participant|UI",
			meta=(CustomTag="MounteaK2Setter"))
	void RemoveBindings();

	UFUNCTION()
	void OnDialogueStarted(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueClosed(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueFailed(const FString& ErrorMessage);
	
	
	UFUNCTION()
	void OnDialogueNodeStarted(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeFinished(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueNodeSelected(UMounteaDialogueContext* Context);
	
	UFUNCTION()
	void OnDialogueRowStarted(UMounteaDialogueContext* Context);
	UFUNCTION()
	void OnDialogueRowFinished(UMounteaDialogueContext* Context);
	
public:
	
	/**
	 * 
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Participant|User Interface",
		meta=(NoResetToDefault))
	TScriptInterface<IMounteaDialogueManagerInterface> ParentManager;

	/**
	 * 
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Participant|User Interface",
		meta=(NoResetToDefault))
	TObjectPtr<UObject> UserInterface;
};
