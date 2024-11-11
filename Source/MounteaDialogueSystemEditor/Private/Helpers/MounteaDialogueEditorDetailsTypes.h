// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "Interfaces/MounteaDialogueParticipantInterface.h"

struct FPIEInstanceData
{
	int32 InstanceId = -1;
	FString InstanceType;
	FString NameOverride;
	const FWorldContext* Context = nullptr;
	TArray<TWeakInterfacePtr<IMounteaDialogueParticipantInterface>> Participants;

	FPIEInstanceData() = default;
	FPIEInstanceData(int32 InId, const FString& InType, const FWorldContext* InContext)
		: InstanceId(InId)
		, InstanceType(InType)
		, Context(InContext)
	{}

	FString GetParticipantsDescription() const
	{
		if (Participants.IsEmpty())
			return TEXT("No Active Participants");

		FString Result;
		for (int32 i = 0; i < Participants.Num(); ++i)
		{
			if (const auto Participant = Participants[i].Get())
			{
				if (Participant == nullptr)
				{
					Result.Append(TEXT("Invalid Participant"));
					continue;
				}
			
				if (i > 0) Result.Append(TEXT(", "));
				if (AActor* Owner = Participant->Execute_GetOwningActor(Participant->_getUObject()))
					Result.Append(Owner->GetActorLabel());
				else
					Result.Append(TEXT("Unknown Participant"));
			}
		}
		return Result;
	};
};
