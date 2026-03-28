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

#include "Subsystem/MounteaDialogueWorldSubsystem.h"
#include "Components/MounteaDialogueManager.h"
#include "Components/MounteaDialogueSession.h"

void UMounteaDialogueWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UMounteaDialogueWorldSubsystem::Deinitialize()
{
	RegisteredManagers.Empty();
	ActiveSessions.Empty();

	Super::Deinitialize();
}

void UMounteaDialogueWorldSubsystem::RegisterManager(UMounteaDialogueManager* Manager)
{
	if (!Manager) return;
	RegisteredManagers.AddUnique(Manager);
}

void UMounteaDialogueWorldSubsystem::UnregisterManager(UMounteaDialogueManager* Manager)
{
	if (!Manager) return;
	RegisteredManagers.Remove(Manager);
}
