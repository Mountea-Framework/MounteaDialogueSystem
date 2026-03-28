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

#include "Components/MounteaDialogueSession.h"
#include "Net/UnrealNetwork.h"

UMounteaDialogueSession::UMounteaDialogueSession()
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = false;
}

void UMounteaDialogueSession::BeginPlay()
{
	Super::BeginPlay();
}

void UMounteaDialogueSession::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMounteaDialogueSession, ContextPayload);
}

void UMounteaDialogueSession::OnRep_ContextPayload()
{
	// Stage 2 implementation: iterate registered managers and call OnContextPayloadUpdated
}
