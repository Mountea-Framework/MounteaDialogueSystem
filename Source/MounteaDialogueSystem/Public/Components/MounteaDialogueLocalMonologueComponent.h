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
#include "Components/MounteaDialogueManager.h"
#include "MounteaDialogueLocalMonologueComponent.generated.h"

class ULocalPlayer;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;
class UMounteaDialogueLocalMonologueSubsystem;
class IMounteaDialogueParticipantInterface;

struct FDialogueRow;
struct FDialogueRowData;

/**
 * Local-only monologue manager.
 *
 * Uses the existing manager/UI contracts but runs entirely client-side:
 * - no GameState session
 * - no server RPC flow
 * - no replicated payload dependency
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"),
	meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Local Monologue Manager"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueLocalMonologueComponent : public UMounteaDialogueManager
{
	GENERATED_BODY()

public:

	UMounteaDialogueLocalMonologueComponent();

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	void HandleLocalDialogueFailed(const FString& ErrorMessage);

public:

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Setter"))
	bool RequestStartLocalMonologue(const FDialogueStartRequest& Request, FString& OutErrorMessage);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Setter"))
	void RequestCloseLocalMonologue();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Validate"))
	bool IsLocalMonologueActive() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Monologue",
		meta=(CustomTag="MounteaK2Getter"))
	FGuid GetActiveLocalMonologueSessionGUID() const;

	// Called by UMounteaDialogueLocalMonologueSubsystem after arbitration.
	bool StartLocalMonologueInternal(const FDialogueStartRequest& Request, FString& OutErrorMessage);

public:

	virtual void SetManagerState(const EDialogueManagerState NewState) override;
	virtual void SetDefaultManagerState(const EDialogueManagerState NewState) override;

	virtual void RequestStartDialogue_Implementation(AActor* DialogueInitiator, const FDialogueParticipants& InitialParticipants) override;
	virtual void RequestCloseDialogue_Implementation() override;

	virtual void StartDialogue_Implementation() override;
	virtual void CloseDialogue_Implementation() override;

	virtual void PrepareNode_Implementation() override;
	virtual void NodePrepared_Implementation() override;
	virtual void ProcessNode_Implementation() override;
	virtual void NodeProcessed_Implementation() override;
	virtual void SelectNode_Implementation(const FGuid& NodeGuid) override;

	virtual void ProcessDialogueRow_Implementation() override;
	virtual void DialogueRowProcessed_Implementation(const bool bForceFinish = false) override;
	virtual void SkipDialogueRow_Implementation() override;

	virtual void ExecuteWidgetCommand_Implementation(const FString& Command) override;

private:

	ULocalPlayer* ResolveOwningLocalPlayer() const;
	UMounteaDialogueLocalMonologueSubsystem* ResolveLocalMonologueSubsystem() const;
	bool ResolveDialogueParticipants(
		const FDialogueStartRequest& Request,
		TScriptInterface<IMounteaDialogueParticipantInterface>& OutMainParticipant,
		TArray<TScriptInterface<IMounteaDialogueParticipantInterface>>& OutAllParticipants,
		FString& OutErrorMessage) const;
	UMounteaDialogueGraph* ResolveActiveDialogueGraph(
		const FDialogueStartRequest& Request,
		const TScriptInterface<IMounteaDialogueParticipantInterface>& MainParticipant,
		FString& OutErrorMessage) const;
	void ApplyNodeSwitchForLinearMonologue(UMounteaDialogueContext* Context, UMounteaDialogueGraphNode* NewActiveNode);
};
