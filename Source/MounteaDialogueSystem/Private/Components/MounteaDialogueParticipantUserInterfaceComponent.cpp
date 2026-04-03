// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/MounteaDialogueParticipantUserInterfaceComponent.h"

#include "Helpers/MounteaDialogueManagerStatics.h"
#include "Helpers/MounteaDialogueSystemBFC.h"
#include "TimerManager.h"

UMounteaDialogueParticipantUserInterfaceComponent::UMounteaDialogueParticipantUserInterfaceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMounteaDialogueParticipantUserInterfaceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* world = GetWorld())
	{
		FTimerHandle requestManagerTimerHandle;
		world->GetTimerManager().SetTimer(
			requestManagerTimerHandle,
			this,
			&UMounteaDialogueParticipantUserInterfaceComponent::RequestManager,
			1.0f,
			false
		);
	}
}

void UMounteaDialogueParticipantUserInterfaceComponent::RequestManager()
{
	if (!UMounteaDialogueSystemBFC::IsLocalPlayer(GetOwner()))
		return;
	
	int searchDepth = 0;
	ParentManager = UMounteaDialogueManagerStatics::FindDialogueManagerInterface(GetOwner(), searchDepth);
}

void UMounteaDialogueParticipantUserInterfaceComponent::SetupBindings()
{
	if (!UMounteaDialogueSystemBFC::IsLocalPlayer(GetOwner()))
		return;
	
	if (!IsValid(ParentManager.GetObject()))
		return;
	
	ParentManager->GetDialogueStartedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueStarted);
	ParentManager->GetDialogueClosedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueClosed);
	ParentManager->GetDialogueFailedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueFailed);
	
	ParentManager->GetDialogueNodeStartedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeStarted);
	ParentManager->GetDialogueNodeFinishedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeFinished);
	ParentManager->GetDialogueNodeSelectedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeSelected);
	
	ParentManager->GetDialogueRowStartedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowStarted);
	ParentManager->GetDialogueRowFinishedEventHandle().AddUniqueDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowFinished);
}

void UMounteaDialogueParticipantUserInterfaceComponent::RemoveBindings()
{
	if (!UMounteaDialogueSystemBFC::IsLocalPlayer(GetOwner()))
		return;
	
	if (!IsValid(ParentManager.GetObject()))
		return;
	
	ParentManager->GetDialogueStartedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueStarted);
	ParentManager->GetDialogueClosedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueClosed);
	ParentManager->GetDialogueFailedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueFailed);
	
	ParentManager->GetDialogueNodeStartedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeStarted);
	ParentManager->GetDialogueNodeFinishedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeFinished);
	ParentManager->GetDialogueNodeSelectedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeSelected);
	
	ParentManager->GetDialogueRowStartedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowStarted);
	ParentManager->GetDialogueRowFinishedEventHandle().RemoveDynamic(this, &UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowFinished);
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueStarted(UMounteaDialogueContext* Context)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueClosed(UMounteaDialogueContext* Context)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueFailed(const FString& ErrorMessage)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeStarted(UMounteaDialogueContext* Context)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeFinished(UMounteaDialogueContext* Context)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueNodeSelected(UMounteaDialogueContext* Context)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowStarted(UMounteaDialogueContext* Context)
{
	
}

void UMounteaDialogueParticipantUserInterfaceComponent::OnDialogueRowFinished(UMounteaDialogueContext* Context)
{
	
}

