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

#include "MounteaDialogueSetupUtilities.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameMapsSettings.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Editor.h"

#include "Components/MounteaDialogueParticipant.h"
#include "Components/MounteaDialogueParticipantUserInterfaceComponent.h"
#include "Components/MounteaDialogueManager.h"
#include "Components/MounteaDialogueSession.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/HUD/MounteaDialogueHUDClassInterface.h"

bool FSetupDefaultsReport::AllSucceeded() const
{
	for (const FSetupItemResult& item : Items)
	{
		if (item.Status != ESetupItemStatus::AlreadyPresent && item.Status != ESetupItemStatus::Added)
			return false;
	}
	return Items.Num() > 0;
}

bool FSetupDefaultsReport::AnySucceeded() const
{
	for (const FSetupItemResult& item : Items)
	{
		if (item.Status == ESetupItemStatus::AlreadyPresent || item.Status == ESetupItemStatus::Added)
			return true;
	}
	return false;
}

bool FSetupDefaultsReport::AllFailed() const
{
	for (const FSetupItemResult& item : Items)
	{
		if (item.Status == ESetupItemStatus::AlreadyPresent || item.Status == ESetupItemStatus::Added || item.Status == ESetupItemStatus::Skipped)
			return false;
	}
	return Items.Num() > 0;
}

FSetupDefaultsReport FMounteaDialogueSetupUtilities::RunSetupDefaults()
{
	FSetupDefaultsReport report;

	if (!GEditor)
		return report;

	UWorld* world = GEditor->GetEditorWorldContext().World();
	if (!world)
		return report;

	AWorldSettings* worldSettings = world->GetWorldSettings();
	TSubclassOf<AGameModeBase> gameModeClass = worldSettings ? worldSettings->DefaultGameMode : nullptr;

	if (!gameModeClass)
	{
		const UGameMapsSettings* gameMapsSettings = GetDefault<UGameMapsSettings>();
		if (gameMapsSettings)
		{
			const FString defaultGameModePath = gameMapsSettings->GetGlobalDefaultGameMode();
			if (!defaultGameModePath.IsEmpty())
				gameModeClass = LoadClass<AGameModeBase>(nullptr, *defaultGameModePath);
		}
	}

	if (!gameModeClass || gameModeClass == AGameModeBase::StaticClass() || gameModeClass->GetName() == TEXT("GameMode"))
	{
		report.bIsDefaultGameMode = true;
		report.GameModeClassName = gameModeClass ? gameModeClass->GetName() : TEXT("None");
		return report;
	}

	report.GameModeClassName = gameModeClass->GetName();

	const AGameModeBase* cdo = gameModeClass->GetDefaultObject<AGameModeBase>();
	if (!cdo)
		return report;

	report.Items.Add(CheckAndSetupSlot(ESetupActorRole::Pawn,				cdo->DefaultPawnClass,          UMounteaDialogueParticipant::StaticClass(),                      nullptr));
	report.Items.Add(CheckAndSetupSlot(ESetupActorRole::PlayerController,  cdo->PlayerControllerClass,     UMounteaDialogueParticipantUserInterfaceComponent::StaticClass(), nullptr));
	report.Items.Add(CheckAndSetupSlot(ESetupActorRole::PlayerState,       cdo->PlayerStateClass,          UMounteaDialogueManager::StaticClass(),                           nullptr));
	report.Items.Add(CheckAndSetupSlot(ESetupActorRole::GameState,         cdo->GameStateClass,            UMounteaDialogueSession::StaticClass(),                           nullptr));
	report.Items.Add(CheckAndSetupSlot(ESetupActorRole::HUD,               cdo->HUDClass,                  nullptr,                                                          UMounteaDialogueHUDClassInterface::StaticClass()));

	return report;
}

FString FMounteaDialogueSetupUtilities::GetRoleDisplayName(const ESetupActorRole Role)
{
	switch (Role)
	{
		case ESetupActorRole::Pawn:             return TEXT("Pawn");
		case ESetupActorRole::PlayerController: return TEXT("Player Controller");
		case ESetupActorRole::PlayerState:      return TEXT("Player State");
		case ESetupActorRole::GameState:        return TEXT("Game State");
		case ESetupActorRole::HUD:              return TEXT("HUD");
	}
	return TEXT("Unknown");
}

FSetupItemResult FMounteaDialogueSetupUtilities::CheckAndSetupSlot(const ESetupActorRole Role, UClass* TargetClass, const TSubclassOf<UActorComponent> RequiredComponent, const UClass* RequiredInterface)
{
	FSetupItemResult result;
	result.Role = Role;

	if (!TargetClass)
	{
		result.ClassName = TEXT("None");
		result.Status = ESetupItemStatus::Skipped;
		result.Details = TEXT("No class assigned in GameMode.");
		return result;
	}

	result.ClassName = TargetClass->GetName();

	UBlueprintGeneratedClass* blueprintGenClass = Cast<UBlueprintGeneratedClass>(TargetClass);
	if (!blueprintGenClass)
	{
		result.Status = ESetupItemStatus::CppClass;
		result.Details = TEXT("C++ class cannot be modified automatically.");
		return result;
	}

	UBlueprint* blueprinObj = Cast<UBlueprint>(blueprintGenClass->ClassGeneratedBy);
	if (!blueprinObj)
	{
		result.Status = ESetupItemStatus::Failed;
		result.Details = TEXT("Could not retrieve Blueprint asset.");
		return result;
	}

	if (RequiredComponent)
	{
		if (BlueprintHasComponent(blueprintGenClass, RequiredComponent))
		{
			result.Status = ESetupItemStatus::AlreadyPresent;
			result.Details = FString::Printf(TEXT("%s already present."), *RequiredComponent->GetName());
			return result;
		}

		const FName nodeName = FName(*FString::Printf(TEXT("MDS_%s"), *RequiredComponent->GetName()));
		if (BlueprintAddComponent(blueprintGenClass, RequiredComponent, nodeName))
		{
			result.Status = ESetupItemStatus::Added;
			result.Details = FString::Printf(TEXT("%s added successfully."), *RequiredComponent->GetName());
		}
		else
		{
			result.Status = ESetupItemStatus::Failed;
			result.Details = FString::Printf(TEXT("Failed to add %s."), *RequiredComponent->GetName());
		}
		return result;
	}

	if (RequiredInterface)
	{
		if (BlueprintHasInterface(blueprinObj, RequiredInterface))
		{
			result.Status = ESetupItemStatus::AlreadyPresent;
			result.Details = FString::Printf(TEXT("%s already implemented."), *RequiredInterface->GetName());
			return result;
		}

		if (BlueprintAddInterface(blueprinObj, RequiredInterface))
		{
			result.Status = ESetupItemStatus::Added;
			result.Details = FString::Printf(TEXT("%s interface added successfully."), *RequiredInterface->GetName());
		}
		else
		{
			result.Status = ESetupItemStatus::Failed;
			result.Details = FString::Printf(TEXT("Failed to add %s interface."), *RequiredInterface->GetName());
		}
		return result;
	}

	result.Status = ESetupItemStatus::Skipped;
	result.Details = TEXT("No requirement specified.");
	return result;
}

bool FMounteaDialogueSetupUtilities::BlueprintHasComponent(const UBlueprintGeneratedClass* BPGC, const TSubclassOf<UActorComponent> CompClass)
{
	if (!BPGC || !BPGC->SimpleConstructionScript || !CompClass)
		return false;

	for (USCS_Node* node : BPGC->SimpleConstructionScript->GetAllNodes())
	{
		if (node && node->ComponentClass && node->ComponentClass->IsChildOf(CompClass))
			return true;
	}
	return false;
}

bool FMounteaDialogueSetupUtilities::BlueprintAddComponent(const UBlueprintGeneratedClass* BPGC, const TSubclassOf<UActorComponent> CompClass, const FName NodeName)
{
	if (!BPGC || !BPGC->SimpleConstructionScript || !CompClass)
		return false;

	UBlueprint* bp = Cast<UBlueprint>(BPGC->ClassGeneratedBy);
	if (!bp)
		return false;

	USCS_Node* newNode = BPGC->SimpleConstructionScript->CreateNode(CompClass, NodeName);
	if (!newNode)
		return false;

	BPGC->SimpleConstructionScript->AddNode(newNode);
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(bp);
	FKismetEditorUtilities::CompileBlueprint(bp);
	return true;
}

bool FMounteaDialogueSetupUtilities::BlueprintHasInterface(UBlueprint* BP, const UClass* InterfaceClass)
{
	if (!BP || !InterfaceClass)
		return false;

	for (const FBPInterfaceDescription& desc : BP->ImplementedInterfaces)
	{
		if (desc.Interface && desc.Interface->IsChildOf(InterfaceClass))
			return true;
	}
	return false;
}

bool FMounteaDialogueSetupUtilities::BlueprintAddInterface(UBlueprint* BP, const UClass* InterfaceClass)
{
	if (!BP || !InterfaceClass)
		return false;

	if (!FBlueprintEditorUtils::ImplementNewInterface(BP, FTopLevelAssetPath(InterfaceClass->GetPackage()->GetFName(), InterfaceClass->GetFName())))
		return false;

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	return true;
}
