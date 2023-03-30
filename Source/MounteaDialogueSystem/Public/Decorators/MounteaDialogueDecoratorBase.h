// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "Helpers/MounteaDialogueGraphHelpers.h"
#include "UObject/Object.h"
#include "Engine/Level.h"
#include "MounteaDialogueDecoratorBase.generated.h"

class IMounteaDialogueParticipantInterface;
class UMounteaDialogueGraph;
class UMounteaDialogueGraphNode;

UENUM(BlueprintType)
enum class EDecoratorState : uint8
{
	Uninitialized,
	Initialized
};

#define LOCTEXT_NAMESPACE "NodeDecoratorBase"

/**
 *	Mountea Dialogue Decorator
 *
 * Decorators are instanced and exist only as "triggers".
 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, ClassGroup=("Mountea|Dialogue"), AutoExpandCategories=("Mountea, Dialogue"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecoratorBase : public UObject
{
	GENERATED_BODY()

public:

	FORCEINLINE ULevel* GetLevel() const
	{
		return GetTypedOuter<ULevel>();
	}

public:
	
	virtual UWorld* GetWorld() const override
	{
		if (OwningWorld) return OwningWorld;
		
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
	}

	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	FString GetDecoratorDocumentationLink() const;
	virtual FString GetDecoratorDocumentationLink_Implementation() const
	{
		return TEXT("https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Dialogue-Decorators");
	}

public:

	/**
	 * Initializes the Decorator.
	 * In C++ saves the World for later use.
	 * In Blueprints should be used to cache values to avoid overhead in 'ExecuteDecorator'.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	void InitializeDecorator(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant);
	virtual void InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant)
	{
		OwningWorld = World;
		if (World)
		{
			DecoratorState = EDecoratorState::Initialized;
		}

		if (OwningParticipant)
		{
			OwnerParticipant = OwningParticipant;
		}
	};

	/**
	 * Cleans up the Decorator.
	 * In Blueprints should be used to reset cached values to avoid blocking garbage collector.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	void CleanupDecorator();
	virtual void CleanupDecorator_Implementation()
	{
		DecoratorState = EDecoratorState::Uninitialized;
	};

	/**
	 * Validates the Decorator.
	 * Called for each Node it is attached to.
	 * Works as safety measure to avoid calling broken Decorators with invalid references.
	 *
	 * False value stops Dialogue whatsoever.
	 * Validation is called before Context is initialized!
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorators")
	bool ValidateDecorator(TArray<FText>& ValidationMessages);
	virtual bool ValidateDecorator_Implementation(TArray<FText>& ValidationMessages);
	
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

	/**
	 * Stores reference to World.
	 * World is needed to perform World affecting tasks.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Decorators")
	void StoreWorldReference(UWorld* World)
	{
		if (OwningWorld != World) OwningWorld = World;
	}

	/**
	 * Returns Owning World this Decorator belongs to.
	 *
	 * ❗ Should not return Null, but possibly can.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Decorators", meta=(CompactNodeTitle="World"))
	UWorld* GetOwningWorld() const
	{ return OwningWorld; };

	/**
	 * Returns Owning Node of this Decorator.
	 *
	 * ❗ Might return Null if this Decorator is owned by Graph!
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorators", meta=(CompactNodeTitle="OwningNode"))
	UMounteaDialogueGraphNode* GetOwningNode() const;
	/**
	 * Returns Owning Graph of this Decorator.
	 *
	 * ❗ Might return Null if this Decorator is owned by Node!
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorators", meta=(CompactNodeTitle="OwningGraph"))
	UMounteaDialogueGraph* GetOwningGraph() const;
	/**
	 * Returns Owning Object of this Decorator.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorators", meta=(CompactNodeTitle="Owner"))
	UObject* GetOwner() const;
	/**
	 * Returns Owner Participant Interface.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorators", meta=(CompactNodeTitle="OwnerParticipant"))
	TScriptInterface<IMounteaDialogueParticipantInterface> GetOwnerParticipant() const
	{ return OwnerParticipant; };

	FText GetDecoratorName() const;
	
private:

	UPROPERTY()
	EDecoratorState DecoratorState = EDecoratorState::Uninitialized;

	UPROPERTY()
	UWorld* OwningWorld = nullptr;
	UPROPERTY()
	TScriptInterface<IMounteaDialogueParticipantInterface> OwnerParticipant = nullptr;
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

	void InitializeDecorator(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant) const
	{
		if (DecoratorType)
		{
			DecoratorType->InitializeDecorator(World, OwningParticipant);
			return;
		}

		LOG_ERROR(TEXT("[InitializeDecorator] DecoratorType is null (invalid)!"))
		return;
	}

	bool ValidateDecorator(TArray<FText>& ValidationMessages) const
	{
		if (DecoratorType)
		{
			return DecoratorType->ValidateDecorator(ValidationMessages);
		}
		
		LOG_ERROR(TEXT("[EvaluateDecorator] DecoratorType is null (invalid)!"))
		return false;
	}

	void CleanupDecorator() const
	{
		if (DecoratorType)
		{
			DecoratorType->CleanupDecorator();
			return;
		}

		LOG_ERROR(TEXT("[CleanupDecorator] DecoratorType is null (invalid)!"))
		return;
	}

	bool EvaluateDecorator() const
	{
		if (DecoratorType)
		{
			return DecoratorType->EvaluateDecorator();
		}
		
		LOG_ERROR(TEXT("[EvaluateDecorator] DecoratorType is null (invalid)!"))
		return false;
	};
	
	void ExecuteDecorator() const
	{
		if (DecoratorType)
		{
			DecoratorType->ExecuteDecorator();
			return;
		}
		
		LOG_ERROR(TEXT("[ExecuteDecorator] DecoratorType is null (invalid)!"))
		return;
	};

public:

	/**
	 * Decorators can help out with enhancing the Dialogue flow.
	 * Those Decorators are instanced and exist only as "triggers".
	 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mountea|Dialogue", meta=(NoResetToDefault, AllowAbstract = "false", BlueprintBaseOnly = "true"))
	UMounteaDialogueDecoratorBase* DecoratorType = nullptr;

public:

	bool operator==(const FMounteaDialogueDecorator& Other) const
	{
		return DecoratorType == Other.DecoratorType;	
	};
	
};

#undef LOCTEXT_NAMESPACE