// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManagerNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

// Add this operator outside the struct
inline FArchive& operator<<(FArchive& Ar, FMounteaDialogueContextReplicatedStruct& Value)
{
	bool bSuccess;
	Value.NetSerialize(Ar, nullptr, bSuccess);
	return Ar;
}

USTRUCT()
struct FGenericRPCPayload
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<uint8> SerializedParams;

	template<typename... ParamTypes>
	static FGenericRPCPayload Make(ParamTypes&&... Params)
	{
		FGenericRPCPayload Payload;
		FMemoryWriter Writer(Payload.SerializedParams);
		// Writer is always in "saving" mode as we're writing TO the payload
		SerializeParams(Writer, Forward<ParamTypes>(Params)...);
		return Payload;
	}

	template<typename... ParamTypes>
	void Unpack(ParamTypes&... Params) const
	{
		FMemoryReader Reader(SerializedParams);
		// Reader is always in "loading" mode as we're reading FROM the payload
		SerializeParams(Reader, Params...);
	}

private:
	static void SerializeParams(FArchive& Ar) {}

	template<typename T, typename... Rest>
	static void SerializeParams(FArchive& Ar, T&& First, Rest&&... rest)
	{
		SerializeSingleParam(Ar, Forward<T>(First));
		SerializeParams(Ar, Forward<Rest>(rest)...);
	}

	// Handle FDialogueParticipants
	static void SerializeDialogueParticipantsImpl(FArchive& Ar, FDialogueParticipants& Value)
	{
		// Main participant
		AActor* MainActor = Value.MainParticipant.Get();
		Ar << MainActor;
		if (Ar.IsLoading())
		{
			Value.MainParticipant = MainActor;
		}

		// Other participants
		int32 Count = Value.OtherParticipants.Num();
		Ar << Count;

		if (Ar.IsLoading())
		{
			Value.OtherParticipants.SetNum(Count);
			for (int32 i = 0; i < Count; ++i)
			{
				AActor* Actor = nullptr;
				Ar << Actor;
				Value.OtherParticipants[i] = Actor;
			}
		}
		else // We're writing TO the payload
		{
			for (int32 i = 0; i < Count; ++i)
			{
				AActor* Actor = Cast<AActor>(Value.OtherParticipants[i].Get());
				Ar << Actor;
			}
		}
	}

	// Non-const version
	static void SerializeSingleParam(FArchive& Ar, FDialogueParticipants& Value)
	{
		SerializeDialogueParticipantsImpl(Ar, Value);
	}

	// Const version - will be called during Make (saving to payload)
	static void SerializeSingleParam(FArchive& Ar, const FDialogueParticipants& Value)
	{
		// We can always write FROM a const value TO our payload
		SerializeDialogueParticipantsImpl(Ar, const_cast<FDialogueParticipants&>(Value));
	}

	// Handle UObject pointers
	template<typename T>
	static void SerializeSingleParam(FArchive& Ar, T*& Value)
	{
		Ar << Value;
	}

	template<typename T>
	static void SerializeSingleParam(FArchive& Ar, const T*& Value)
	{
		// We can always write FROM a const pointer TO our payload
		T* MutablePtr = const_cast<T*>(Value);
		Ar << MutablePtr;
	}

	// Handle TObjectPtr
	template<typename T>
	static void SerializeSingleParam(FArchive& Ar, TObjectPtr<T>& Value)
	{
		T* RawPtr = Value.Get();
		Ar << RawPtr;
		if (Ar.IsLoading())
		{
			Value = RawPtr;
		}
	}

	// Handle basic types
	template<typename T>
	static typename TEnableIf<!TIsPointer<T>::Value && 
							 !TIsTObjectPtr<T>::Value && 
							 !std::is_same<typename TRemoveCV<T>::Type, FDialogueParticipants>::value>::Type
	SerializeSingleParam(FArchive& Ar, T& Value)
	{
		Ar << Value;
	}
};

template<typename ReturnType, typename Func, typename... Args>
static ReturnType ExecuteIfImplements(UObject* Target, const TCHAR* FunctionName, Func Function, Args&&... args)
{
	if (!IsValid(Target))
	{
		LOG_ERROR(TEXT("[%s] Invalid Target provided!"), FunctionName);
		if constexpr (!std::is_void_v<ReturnType>)
			return ReturnType{};
		else return;
	}

	if (Target->Implements<UMounteaDialogueManagerInterface>())
	{
		if constexpr (std::is_void_v<ReturnType>)
		{
			Function(Target, Forward<Args>(args)...);
			return;
		}
		return Function(Target, Forward<Args>(args)...);
	}

	LOG_ERROR(TEXT("[%s] Target does not implement 'MounteaDialogueManagerInterface'!"), FunctionName);
	if constexpr (!std::is_void_v<ReturnType>)
		return ReturnType{};
	else return;
}

/**
 * Component that enables network synchronization for Mountea Dialogue Managers.
 * Handles RPC routing through PlayerController's network connection and manages dialogue manager registration.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"), meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager Sync"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManagerNetSync : public UActorComponent
{
	GENERATED_BODY()

public:
	UMounteaDialogueManagerNetSync();

	template<typename... ParamTypes>
	void RouteRPC(UFunction* RPCFunction, APlayerController* Instigator, ParamTypes&&... Params)
	{
		if (!RPCFunction || !Instigator)
		{
			LOG_ERROR(TEXT("Invalid RPCFunction or Instigator"));
			return;
		}

		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			FGenericRPCPayload Payload = FGenericRPCPayload::Make(Forward<ParamTypes>(Params)...);
			
			if (Payload.SerializedParams.Num() == 0)
			{
				LOG_ERROR(TEXT("Empty payload created"));
				return;
			}
			
			RouteRPC_Server(RPCFunction, Instigator, Payload);
			return;
		}

		ExecuteRPC(RPCFunction, Instigator, Forward<ParamTypes>(Params)...);
	}
	
	UFUNCTION(Server, Reliable)
	void RouteRPC_Server(UFunction* RPCFunction, APlayerController* Instigator, const FGenericRPCPayload& Payload);

protected:
	UFUNCTION()
	void OnManagerSyncActivated(UActorComponent* Component, bool bReset);
	
	UFUNCTION()
	void OnManagerSyncDeactivated(UActorComponent* Component);
	
	virtual void BeginPlay() override;

public:
	void AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager);
	void RemoveManager(const TScriptInterface<IMounteaDialogueManagerInterface>& OldManager);

protected:
	template<typename... ParamTypes>
	void ExecuteRPC(UFunction* RPCFunction, APlayerController* Instigator, ParamTypes... Params)
	{
		if (!RPCFunction || !Instigator)
			return;

		for (const auto& Manager : Managers)
		{
			if (UObject* Object = Manager.GetObject())
			{
				const bool bIsInterfaceFunction = RPCFunction->GetOwnerClass()->ImplementsInterface(UMounteaDialogueManagerInterface::StaticClass());
				
				if (bIsInterfaceFunction || Object->IsA(RPCFunction->GetOwnerClass()))
				{
					const size_t TotalSize = (0 + ... + sizeof(ParamTypes));
					void* ParamBuffer = FMemory::Malloc(TotalSize, 16);

					void* CurrentPtr = ParamBuffer;
					PackParams(CurrentPtr, Params...);

					if (bIsInterfaceFunction)
					{
						auto ExecuteFunc = [RPCFunction, ParamBuffer](UObject* Target)
						{
							Target->ProcessEvent(RPCFunction, ParamBuffer);
						};
						
						ExecuteIfImplements<void>(Object, *RPCFunction->GetName(), ExecuteFunc);
					}
					else
					{
						Object->ProcessEvent(RPCFunction, ParamBuffer);
					}

					FMemory::Free(ParamBuffer);
					break;
				}
			}
		}
	}

private:
	template<typename T>
	void PackParams(void*& Buffer, T& Param)
	{
		size_t Space = SIZE_MAX;
		void* AlignedPtr = Buffer;
		AlignedPtr = std::align(alignof(T), sizeof(T), AlignedPtr, Space);
		FMemory::Memcpy(AlignedPtr, &Param, sizeof(T));
		Buffer = static_cast<uint8*>(AlignedPtr) + sizeof(T);
	}

	template<typename T, typename... Rest>
	void PackParams(void*& Buffer, T& First, Rest&... A)
	{
		PackParams(Buffer, First);
		PackParams(Buffer, A...);
	}

protected:
	/** Array of registered dialogue managers */
	UPROPERTY(VisibleAnywhere, Category="Dialogue", meta=(DisplayThumbnail="false", DisplayName="Registered Managers"))
	TArray<TScriptInterface<IMounteaDialogueManagerInterface>> Managers;
};