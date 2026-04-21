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

enum class ESetupItemStatus : uint8
{
	AlreadyPresent,
	Added,
	CppClass,
	Failed,
	Skipped
};

enum class ESetupActorRole : uint8
{
	Pawn,
	PlayerController,
	PlayerState,
	GameState,
	HUD
};

struct FSetupItemResult
{
	ESetupActorRole Role;
	FString ClassName;
	ESetupItemStatus Status;
	FString Details;
};

struct FSetupDefaultsReport
{
	bool bIsDefaultGameMode = false;
	FString GameModeClassName;
	TArray<FSetupItemResult> Items;

	bool AllSucceeded() const;
	bool AnySucceeded() const;
	bool AllFailed() const;
};

class FMounteaDialogueSetupUtilities
{
public:
	static FSetupDefaultsReport RunSetupDefaults();
	static FString GetRoleDisplayName(ESetupActorRole Role);

private:
	static FSetupItemResult CheckAndSetupSlot(ESetupActorRole Role, UClass* TargetClass, TSubclassOf<UActorComponent> RequiredComponent, const UClass* RequiredInterface);

	static bool BlueprintHasComponent(const UBlueprintGeneratedClass* BPGC, TSubclassOf<UActorComponent> CompClass);
	static bool BlueprintAddComponent(const UBlueprintGeneratedClass* BPGC, TSubclassOf<UActorComponent> CompClass, FName NodeName);
	static bool BlueprintHasInterface(UBlueprint* BP, const UClass* InterfaceClass);
	static bool BlueprintAddInterface(UBlueprint* BP, const UClass* InterfaceClass);
};
