// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "Interfaces/Core/MounteaDialogueParticipantInterface.h"

struct FPIEInstanceData
{
	int32 InstanceId = INDEX_NONE;
	FString InstanceType;
	FString NameOverride;
	const FWorldContext* Context = nullptr;
	TWeakInterfacePtr<IMounteaDialogueParticipantInterface> Participant;

	FPIEInstanceData() = default;
	FPIEInstanceData(int32 InId, const FString& InType, const FWorldContext* InContext)
		: InstanceId(InId)
		, InstanceType(InType)
		, Context(InContext)
	{}

	FString GetParticipantDescription() const
	{
		if (!Participant.IsValid())
			return TEXT("No Participant");

		if (const auto ParticipantPtr = Participant.Get())
		{
			if (AActor* Owner = ParticipantPtr->Execute_GetOwningActor(ParticipantPtr->_getUObject()))
				return Owner->GetActorLabel();
		}
        
		return TEXT("Unknown Participant");
	}
};
