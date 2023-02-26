// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNodeDecoratorBase.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FMounteaDialogueDecorator
{
	GENERATED_BODY()

	FMounteaDialogueDecorator(): DecoratorType(nullptr)
	{};

public:

	/**
	 * Explanation goes here.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mountea|Dialogue")
	UMounteaDialogueGraphNodeDecoratorBase* DecoratorType = nullptr;

public:

	bool operator==(const FMounteaDialogueDecorator& Other) const
	{
		return DecoratorType == Other.DecoratorType;	
	};
	
};

/**
 * 
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew) //, ClassGroup=("Mountea|Dialogue"), HideCategories=("Hidden"), AutoExpandCategories=("Mountea, Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNodeDecoratorBase : public UObject
{
	GENERATED_BODY()

public:

	FORCEINLINE ULevel* GetLevel() const
	{
		return GetTypedOuter<ULevel>();
	}

	//UFUNCTION(BlueprintCallable, Category="Inventory Transaction")
	virtual UWorld* GetWorld() const override
	{
		// CDO objects do not belong to a world
		// If the actors outer is destroyed or unreachable we are shutting down and the world should be nullptr
		if (
			!HasAnyFlags(RF_ClassDefaultObject) && ensureMsgf(GetOuter(), TEXT("Actor: %s has a null OuterPrivate in AActor::GetWorld()"), *GetFullName())
			&& !GetOuter()->HasAnyFlags(RF_BeginDestroyed) && !GetOuter()->IsUnreachable()
			)
		{
			if (ULevel* Level = GetLevel())
			{
				return Level->OwningWorld;
			}
		}
		return nullptr;
	};

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Mountea|Dialogue")
	void InvokeDecorator();
	virtual void InvokeDecorator_Implementation();
};
