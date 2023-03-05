// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "UObject/Object.h"
#include "MounteaDialogueGraphNodeDecoratorBase.generated.h"

/**
 *	Mountea Dialogue Node Decorators.
 *
 * Decorators are instanced and exist only as "triggers".
 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea, Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueGraphNodeDecoratorBase : public UObject
{
	GENERATED_BODY()

public:

	FORCEINLINE ULevel* GetLevel() const
	{
		return GetTypedOuter<ULevel>();
	}

public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue")
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
	 * Evaluates the Decorator.
	 * Called for each Node it is attached to.
	 * Could enhance Node's 'CanStartNode'. Example would be: BP_RequireItem decorator, which would return true if Player has specific Item in inventory. Otherwise it returns false and its Node is not available in Selection of Answers.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	bool EvaluateDecorator();
	virtual bool EvaluateDecorator_Implementation();

	/**
	 * Executes the Decorator.
	 * Useful for triggering special events per Node, for instance, switching dialogue cameras.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	void ExecuteDecorator();
	virtual void ExecuteDecorator_Implementation();
};


/**
 * Dialogue Decorator Structure.
 * Holds reference to its Instanced Decorator.
 */
USTRUCT(BlueprintType)
struct FMounteaDialogueDecorator
{
	GENERATED_BODY()

	FMounteaDialogueDecorator(): DecoratorType(nullptr)
	{};

public:

	bool EvaluateDecorator() const
	{
		if (DecoratorType)
		{
			return DecoratorType->EvaluateDecorator();
		}
		
		LOG_ERROR(TEXT("[EvaluateDecorator] DecoratorType is null (invalid)!"))
		return false;
	};
	
	void ExecuteDecorator()
	{
		if (DecoratorType)
		{
			DecoratorType->ExecuteDecorator();
		}
		
		LOG_ERROR(TEXT("[ExecuteDecorator] DecoratorType is null (invalid)!"))
		return;
	};

public:

	/**
	 * A list of Decorators that can help out with enhancing the Dialogue flow.
	 * Those Decorators are instanced and exist only as "triggers".
	 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mountea|Dialogue", meta=(NoResetToDefault, AllowAbstract = "false", BlueprintBaseOnly = "true"))
	UMounteaDialogueGraphNodeDecoratorBase* DecoratorType = nullptr;

public:

	bool operator==(const FMounteaDialogueDecorator& Other) const
	{
		return DecoratorType == Other.DecoratorType;	
	};
	
};