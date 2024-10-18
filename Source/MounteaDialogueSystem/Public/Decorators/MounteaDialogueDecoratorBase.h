// All rights reserved Dominik Pavlicek 2023

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/Level.h"
#include "Interfaces/MounteaDialogueTickableObject.h"
#include "MounteaDialogueDecoratorBase.generated.h"

class IMounteaDialogueManagerInterface;
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
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueDecoratorBase : public UObject, public IMounteaDialogueTickableObject
{
	GENERATED_BODY()

public:

	UMounteaDialogueDecoratorBase();

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

	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
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
	 * Dialogue Manager will not override if empty. If need to override with nullptr use `SetOwningManager` instead.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	void InitializeDecorator(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager);
	virtual void InitializeDecorator_Implementation(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager);

	/**
	 * @return Owning Dialogue Manager.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	TScriptInterface<IMounteaDialogueManagerInterface> GetManager() const;
	virtual TScriptInterface<IMounteaDialogueManagerInterface> GetManager_Implementation() const
	{ return OwningManager; };

	/**
	 *	Updates Owning Manager. Can be used to clean the decorator.
	 * @param NewOwningManager			Owning Manager that will handle this Decorator.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	void SetOwningManager(const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager);
	virtual void SetOwningManager_Implementation(const TScriptInterface<IMounteaDialogueManagerInterface>& NewOwningManager);

	/**
	 * Cleans up the Decorator.
	 * In Blueprints should be used to reset cached values to avoid blocking garbage collector.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	void CleanupDecorator();
	virtual void CleanupDecorator_Implementation()
	{
		DecoratorState = EDecoratorState::Uninitialized;
		OwningManager = nullptr;
	};

	/**
	 * Validates the Decorator.
	 * Called for each Node it is attached to.
	 * Works as safety measure to avoid calling broken Decorators with invalid references.
	 *
	 * False value stops Dialogue whatsoever.
	 * Validation is called before Context is initialized!
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	bool ValidateDecorator(UPARAM(ref) TArray<FText>& ValidationMessages);
	virtual bool ValidateDecorator_Implementation(UPARAM(ref) TArray<FText>& ValidationMessages);
	
	/**
	 * Evaluates the Decorator.
	 * Called for each Node it is attached to.
	 * Could enhance Node's 'CanStartNode'. Example would be: BP_RequireItem decorator, which would return true if Player has specific Item in inventory. Otherwise it returns false and its Node is not available in Selection of Answers.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	bool EvaluateDecorator();
	virtual bool EvaluateDecorator_Implementation();

	/**
	 * Executes the Decorator.
	 * Useful for triggering special events per Node, for instance, switching dialogue cameras.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	void ExecuteDecorator();
	virtual void ExecuteDecorator_Implementation();

	/**
	 * Stores reference to World.
	 * World is needed to perform World affecting tasks.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Decorator")
	void StoreWorldReference(UWorld* World)
	{
		if (OwningWorld != World) OwningWorld = World;
	}

	/**
	 * Returns Owning World this Decorator belongs to.
	 *
	 * ❗ Should not return Null, but possibly can.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Decorator", meta=(CompactNodeTitle="World"), meta=(CustomTag="MounteaK2Getter"))
	UWorld* GetOwningWorld() const
	{ return OwningWorld; };

	/**
	 * Returns Owning Node of this Decorator.
	 *
	 * ❗ Might return Null if this Decorator is owned by Graph!
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorator", meta=(CompactNodeTitle="OwningNode"), meta=(CustomTag="MounteaK2Getter"))
	UMounteaDialogueGraphNode* GetOwningNode() const;
	
	/**
	 * Returns Owning Graph of this Decorator.
	 *
	 * ❗ Might return Null if this Decorator is owned by Node!
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorator", meta=(CompactNodeTitle="OwningGraph"), meta=(CustomTag="MounteaK2Getter"))
	UMounteaDialogueGraph* GetOwningGraph() const;
	
	/**
	 * Returns Owning Object of this Decorator.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorator", meta=(CompactNodeTitle="Owner"), meta=(CustomTag="MounteaK2Getter"))
	UObject* GetOwner() const;
	
	/**
	 * Returns Owner Participant Interface.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Decorator", meta=(CompactNodeTitle="OwnerParticipant"), meta=(CustomTag="MounteaK2Getter"))
	TScriptInterface<IMounteaDialogueParticipantInterface> GetOwnerParticipant() const
	{ return OwnerParticipant; };

	/**
	 *	Defines whether this Decorator can be attached to Graph directly, or whether only Node attachment is allowed.
	 * 
	 * @return True if can be attached to graph, false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue|Decorator")
	bool IsDecoratorAllowedForGraph() const;
	virtual bool IsDecoratorAllowedForGraph_Implementation() const {  return true;  };

	/**
	 * Defines if more than 1 instance of this decorator can be attached to single Node/Graph.
	 * Generally false, however, in some cases true is required: sending UI commands, playing sound or for generic
	 * use decorators that might be repurposed multiple times.
	 * 
	 * @return Whether decorator can be stacked or not
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Mountea|Dialogue|Decorator")
	bool IsDecoratorStackable() const;
	virtual bool IsDecoratorStackable_Implementation() const {  return false;  };

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintPure, Category = "Mountea|Dialogue|Decorator")
	FText GetDecoratorName() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Mountea|Dialogue|Decorator")
	TSet<TSubclassOf<UMounteaDialogueGraphNode>> GetBlacklistedNodeTypes() const;
	
protected:

	class UMounteaDialogueContext* GetContext() const;

#pragma region TickableInterface
	
public:
	virtual void RegisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable) override;
	virtual void UnregisterTick_Implementation(const TScriptInterface<IMounteaDialogueTickableObject>& ParentTickable) override;
	virtual void TickMounteaEvent_Implementation(UObject* SelfRef, UObject* ParentTick, float DeltaTime) override;
	virtual FMounteaDialogueTick& GetMounteaDialogueTickHandle() override {return DecoratorTickEvent; };

	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue")
	FMounteaDialogueTick DecoratorTickEvent;
	
#pragma endregion
	
protected:

	UPROPERTY(BlueprintReadOnly, Category="Private")
	FText DecoratorName;

	UPROPERTY(BlueprintReadOnly, Category="Private")
	TSet<TSoftClassPtr<UMounteaDialogueGraphNode>> BlacklistedNodes;

	UPROPERTY()
	EDecoratorState	DecoratorState	=	EDecoratorState::Uninitialized;

	UPROPERTY()
	TObjectPtr<UWorld>	OwningWorld	=	nullptr;
	UPROPERTY()
	TScriptInterface<IMounteaDialogueParticipantInterface>	OwnerParticipant	=	nullptr;
	UPROPERTY(BlueprintReadOnly, Category="Mountea|Dialogue|Decorator")
	TScriptInterface<IMounteaDialogueManagerInterface>		OwningManager		=	nullptr;
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

	void InitializeDecorator(UWorld* World, const TScriptInterface<IMounteaDialogueParticipantInterface>& OwningParticipant, const TScriptInterface<IMounteaDialogueManagerInterface>& OwningManager) const;

	bool ValidateDecorator(UPARAM(ref) TArray<FText>& ValidationMessages) const;

	void CleanupDecorator() const;

	bool EvaluateDecorator() const;
	
	void ExecuteDecorator() const;

public:

	/**
	 * Decorators can help out with enhancing the Dialogue flow.
	 * Those Decorators are instanced and exist only as "triggers".
	 * Could be used to start audio, play animation or do some logic behind the curtains, like triggering Cutscene etc.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category = "Mountea|Dialogue", meta=(NoResetToDefault, AllowAbstract = "false", BlueprintBaseOnly = "true"))
	TObjectPtr<UMounteaDialogueDecoratorBase>				DecoratorType		= nullptr;

public:

	bool operator==(const FMounteaDialogueDecorator& Other) const
	{
		return DecoratorType == Other.DecoratorType;
	};
	
};

#undef LOCTEXT_NAMESPACE