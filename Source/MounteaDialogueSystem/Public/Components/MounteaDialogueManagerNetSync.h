// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "Interfaces/Core/MounteaDialogueManagerInterface.h"
#include "Kismet/GameplayStatics.h"
#include "MounteaDialogueManagerNetSync.generated.h"

class IMounteaDialogueManagerInterface;
struct FMounteaDialogueContextReplicatedStruct;

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

private:
	static FString SanitizeInstancePath(const FString& OriginalPath)
	{
		FString SanitizedPath = OriginalPath;
#if WITH_EDITOR
		static const FString PIEPrefix(TEXT("UEDPIE_"));
		int32 PIEIndex = SanitizedPath.Find(PIEPrefix);
		if (PIEIndex != INDEX_NONE)
		{
			int32 UnderscoreIndex = SanitizedPath.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, PIEIndex + PIEPrefix.Len());
			if (UnderscoreIndex != INDEX_NONE)
				SanitizedPath.RemoveAt(PIEIndex, (UnderscoreIndex - PIEIndex) + 1);
		}
#endif
		return SanitizedPath;
	}

	static UObject* FindInstanceObject(const FString& ObjectPath, UClass* ExpectedClass = nullptr)
	{
		if (UObject* FoundObject = StaticFindObject(nullptr, nullptr, *ObjectPath))
			return FoundObject;
		
		FString SanitizedPath = SanitizeInstancePath(ObjectPath);
		if (UObject* FoundObject = StaticFindObject(nullptr, nullptr, *SanitizedPath))
			return FoundObject;
		
		if (UWorld* World = GEngine->GetCurrentPlayWorld())
		{
			if (ExpectedClass)
			{
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					if (PC->IsA(ExpectedClass))
						return PC;
				}
				
				TArray<AActor*> FoundActors;
				UGameplayStatics::GetAllActorsOfClass(World, ExpectedClass, FoundActors);
				if (FoundActors.Num() > 0)
					return FoundActors[0];
			}

#if WITH_EDITOR
			FString WorldPrefix;
			const FString WorldName = World->GetOutermost()->GetName();
			if (WorldName.StartsWith(TEXT("UEDPIE_")))
			{
				int32 UnderscoreIndex;
				if (WorldName.FindChar('_', UnderscoreIndex))
					WorldPrefix = WorldName.Left(UnderscoreIndex + 1);

				FString AltPath = ObjectPath;
				AltPath.InsertAt(AltPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 1), *WorldPrefix);
				if (UObject* FoundObject = StaticFindObject(nullptr, nullptr, *AltPath))
					return FoundObject;
			}
#endif
		}
		return nullptr;
	}

	static void SerializeDialogueParticipantsImpl(FArchive& Ar, FDialogueParticipants& Value)
	{
		SerializeObjectReference(Ar, Value.MainParticipant);
		int32 Count = Value.OtherParticipants.Num();
		Ar << Count;

		if (Ar.IsLoading())
		{
			Value.OtherParticipants.SetNum(Count);
			for (int32 i = 0; i < Count; ++i)
				SerializeObjectReference(Ar, Value.OtherParticipants[i]);
		}
		else
			for (int32 i = 0; i < Count; ++i)
				SerializeObjectReference(Ar, Value.OtherParticipants[i]);
	}

	template<typename T>
	static void SerializeObjectReference(FArchive& Ar, TObjectPtr<T>& Value)
	{
		if (Ar.IsLoading())
		{
			FName ObjectPath;
			Ar << ObjectPath;
			
			if (!ObjectPath.IsNone())
			{
				if (UObject* LoadedObject = FindInstanceObject(ObjectPath.ToString(), T::StaticClass()))
				{
					Value = Cast<T>(LoadedObject);
					if (!Value.Get())
						LOG_WARNING(TEXT("Found object but failed to cast: %s"), *ObjectPath.ToString())
				}
				else
				{
					Value = nullptr;
					LOG_WARNING(TEXT("Failed to find object at path: %s"), *ObjectPath.ToString())
				}
			}
			else
				Value = nullptr;
		}
		else
		{
			FName ObjectPath = NAME_None;
			if (UObject* Object = Value.Get())
				ObjectPath = FName(*SanitizeInstancePath(Object->GetPathName()));
			Ar << ObjectPath;
		}
	}

public:
	template<typename... ParamTypes>
	static FGenericRPCPayload Make(ParamTypes&&... Params)
	{
		FGenericRPCPayload Payload;
		FMemoryWriter Writer(Payload.SerializedParams);
		SerializeParams(Writer, Forward<ParamTypes>(Params)...);
		return Payload;
	}

	template<typename... ParamTypes>
	void Unpack(ParamTypes&... Params) const
	{
		FMemoryReader Reader(SerializedParams);
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

	static void SerializeSingleParam(FArchive& Ar, FDialogueParticipants& Value)
	{ SerializeDialogueParticipantsImpl(Ar, Value); }

	static void SerializeSingleParam(FArchive& Ar, const FDialogueParticipants& Value)
	{ SerializeDialogueParticipantsImpl(Ar, const_cast<FDialogueParticipants&>(Value)); }

	template<typename T>
	static void SerializeSingleParam(FArchive& Ar, T*& Value)
	{
		if (Ar.IsLoading())
		{
			FName ObjectPath;
			Ar << ObjectPath;
			
			if (!ObjectPath.IsNone())
			{
				if (UObject* LoadedObject = FindInstanceObject(ObjectPath.ToString(), T::StaticClass()))
				{
					Value = Cast<T>(LoadedObject);
					if (!Value)
						LOG_WARNING(TEXT("Found object but failed to cast: %s"), *ObjectPath.ToString())
				}
				else
				{
					Value = nullptr;
					LOG_WARNING(TEXT("Failed to find object at path: %s"), *ObjectPath.ToString())
				}
			}
			else Value = nullptr;
		}
		else
		{
			FName ObjectPath = NAME_None;
			if (Value != nullptr)
				ObjectPath = FName(*SanitizeInstancePath(Value->GetPathName()));
			Ar << ObjectPath;
		}
	}

	template<typename T>
	static void SerializeSingleParam(FArchive& Ar, const T*& Value)
	{
		T* MutablePtr = const_cast<T*>(Value);
		SerializeSingleParam(Ar, MutablePtr);
		Value = MutablePtr;
	}

	template<typename T>
	static typename TEnableIf<!TIsPointer<T>::Value && !TIsTObjectPtr<T>::Value && 
							 !std::is_same<typename TRemoveCV<T>::Type, FDialogueParticipants>::value>::Type
	SerializeSingleParam(FArchive& Ar, T& Value)
	{ Ar << Value; }
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
 * Must be attached to Player Controller!
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
	
	UPROPERTY(VisibleAnywhere, Category="Dialogue", meta=(DisplayThumbnail="false", DisplayName="Registered Managers"))
	TArray<TScriptInterface<IMounteaDialogueManagerInterface>> Managers;
};