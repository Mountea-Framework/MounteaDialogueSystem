// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "MounteaDialogueManagerNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

USTRUCT()
struct FGenericRPCPayload
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<uint8> SerializedParams;

	template<typename... ParamTypes>
	static FGenericRPCPayload Make(ParamTypes... Params)
	{
		FGenericRPCPayload Payload;
		FMemoryWriter Writer(Payload.SerializedParams);
		SerializeParams(Writer, Params...);
		return Payload;
	}

	template<typename... ParamTypes>
	void Unpack(ParamTypes&... Params) const
	{
		FMemoryReader Reader(SerializedParams);
		SerializeParams(Reader, Params...);
	}

private:
	template<typename T>
	struct HasNetSerialize
	{
	private:
		template<typename U>
		static auto Test(U*) -> decltype(std::declval<U>().NetSerialize(std::declval<FArchive&>(), nullptr, std::declval<bool&>()), std::true_type());
		static std::false_type Test(...);
	public:
		static constexpr bool value = decltype(Test((T*)nullptr))::value;
	};

	template<typename T>
	static typename std::enable_if<std::is_same<T, FDialogueParticipants>::value>::type
	SerializeParam(FArchive& Ar, T& Value)
	{
		Ar << Value.MainParticipant;
		Ar << Value.OtherParticipants;
	}

	template<typename T>
	static typename std::enable_if<HasNetSerialize<T>::value>::type
	SerializeParam(FArchive& Ar, T& Value)
	{
		bool bSuccess = true;
		Value.NetSerialize(Ar, nullptr, bSuccess);
	}

	template<typename T>
	static typename std::enable_if<std::is_convertible<T, UObject*>::value>::type
	SerializeParam(FArchive& Ar, T& Value)
	{
		Ar << Value;
	}

	template<typename T>
	static typename std::enable_if<!HasNetSerialize<T>::value && !std::is_convertible<T, UObject*>::value && !std::is_same<T, FDialogueParticipants>::value>::type
	SerializeParam(FArchive& Ar, T& Value)
	{
		Ar << Value;
	}

	template<typename T, typename... Rest>
	static void SerializeParams(FArchive& Ar, T& First, Rest&... Rests)
	{
		SerializeParam(Ar, First);
		SerializeParams(Ar, Rests...);
	}
		
	static void SerializeParams(FArchive& Ar) {}
};

/**
 * Component that enables network synchronization for Mountea Dialogue Managers.
 * Handles RPC routing through PlayerController's network connection and manages dialogue manager registration.
 */
UCLASS(ClassGroup=(Mountea), Blueprintable, AutoExpandCategories=("Mountea","Dialogue","Mountea|Dialogue"),  meta=(BlueprintSpawnableComponent, DisplayName="Mountea Dialogue Manager Sync"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueManagerNetSync : public UActorComponent
{
	GENERATED_BODY()

public:
	UMounteaDialogueManagerNetSync();

	/**
	 * Routes an RPC call through the PlayerController's network connection. Call locally from client without client ownership.
	 * 
	 * @param RPCFunction The function to be executed
	 * @param Instigator The PlayerController initiating the RPC
	 * @param Params Variable number of parameters for the RPC
	 */
	template<typename... ParamTypes>
	void RouteRPC(UFunction* RPCFunction, APlayerController* Instigator, ParamTypes... Params)
	{
		if (!RPCFunction || !Instigator)
			return;

		if (!GetOwner() || !GetOwner()->HasAuthority())
		{
			RouteRPC_Server(RPCFunction, Instigator, FGenericRPCPayload::Make(Params...));
			return;
		}

		ExecuteRPC(RPCFunction, Instigator, Params...);
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
	
	/**
	 * @brief Registers a new dialogue manager with this sync component
	 * @param NewManager The manager interface to register
	 */
	void AddManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewManager);

	/**
	 * @brief Unregisters a dialogue manager from this sync component
	 * @param OldManager The manager interface to unregister
	 */
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
				if (Object->IsA(RPCFunction->GetOwnerClass()))
				{
					const size_t TotalSize = (0 + ... + sizeof(ParamTypes));
					void* ParamBuffer = FMemory::Malloc(TotalSize, 16);
                
					void* CurrentPtr = ParamBuffer;
					PackParams(CurrentPtr, Params...);
                
					Object->ProcessEvent(RPCFunction, ParamBuffer);
                
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
