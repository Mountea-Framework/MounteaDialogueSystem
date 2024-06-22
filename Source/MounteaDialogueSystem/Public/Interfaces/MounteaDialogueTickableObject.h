// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueTickableObject.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMounteaDialogueTick, UObject*, SelfRef, UObject*, ParentTickable, float, DeltaTime);

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueTickableObject : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueTickableObject
{
	GENERATED_BODY()

public:
	
	/**
	 * Registers a tickable object for dialogue.
	 * 
	 * @param ParentTickable The tickable object to register.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Tick")
	void RegisterTick(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable);
	virtual void RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable) = 0;

	/**
	 * Unregisters a tickable object from dialogue.
	 * 
	 * @param ParentTickable The tickable object to unregister.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Tick")
	void UnregisterTick(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable);
	virtual void UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable) = 0;

	/**                                                                           
	 * Executes logic for ticking the dialogue graph.
	 * 
	 * @param SelfRef The reference to this object.param ParentTick
	 * @param ParentTick The parent object responsible for ticking.param DeltaTime
	 * @param DeltaTime The time since the last tick.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Mountea|Dialogue|Tick")
	void TickMounteaEvent(UObject* SelfRef, UObject* ParentTick, float DeltaTime);
	virtual void TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime) = 0;

	/**
	 * Retrieves the handle for dialogue ticking.
	 * 
	 * @return The dialogue tick handle.
	 */
	virtual FMounteaDialogueTick& GetMounteaDialogueTickHandle() = 0;
};
