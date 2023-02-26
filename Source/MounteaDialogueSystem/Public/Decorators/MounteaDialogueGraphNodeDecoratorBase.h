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
	 * A list of Decorators that can help out with enhancing the Dialogue flow.
	 * Those Decorators are instanced and exist only as "triggers".
	 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
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
 *	Mountea Dialogue Node Decorators.
 *
 * Decorators are instanced and exist only as "triggers".
 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), HideCategories=("Hidden, Private"), AutoExpandCategories=("Mountea, Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNodeDecoratorBase : public UObject
{
	GENERATED_BODY()

public:

	FORCEINLINE ULevel* GetLevel() const
	{
		return GetTypedOuter<ULevel>();
	}

public:
	
	//UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue")
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

public:

	/**
	 * Tries to execute the Decorator.
	 * Useful for adding additional conditions to CanStartDialogue, let's say that specific Lead node has Decorator that makes it require a special Item to enter that dialogue part. With decorators, this effect can be achieved easily.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	bool ExecuteDecorator();
	virtual bool ExecuteDecorator_Implementation();
};
