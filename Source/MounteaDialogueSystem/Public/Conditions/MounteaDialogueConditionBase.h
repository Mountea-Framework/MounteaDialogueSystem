// Copyright (C) 2025 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MounteaDialogueConditionBase.generated.h"

/**
 * Defines how multiple conditions on a single edge are evaluated.
 */
UENUM(BlueprintType)
enum class EConditionEvaluationMode : uint8
{
	All		UMETA(DisplayName = "All",	ToolTip = "All conditions must pass for the edge to be traversable."),
	Any		UMETA(DisplayName = "Any",	ToolTip = "At least one condition must pass for the edge to be traversable.")
};

/**
 * Mountea Dialogue Condition Base
 *
 * Abstract base for all edge conditions. Subclass in C++ or Blueprint to implement
 * custom traversal logic. Conditions are instanced (EditInlineNew) so each edge
 * owns its own instances.
 */
UCLASS(Blueprintable, BlueprintType, EditInlineNew,
	ClassGroup=("Mountea|Dialogue"),
	AutoExpandCategories=("Mountea", "Dialogue"),
	HideCategories=("Private"),
	meta=(DisplayName="Mountea Dialogue Condition Base"))
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueConditionBase : public UObject
{
	GENERATED_BODY()

public:

	UMounteaDialogueConditionBase();

public:

	/**
	 * Evaluates whether this condition is satisfied.
	 * Override in subclasses to implement custom logic.
	 *
	 * @param Context  Arbitrary context object (e.g. dialogue manager or participant).
	 * @return True if the condition passes; false otherwise.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Condition")
	bool EvaluateCondition(UObject* Context) const;
	virtual bool EvaluateCondition_Implementation(UObject* Context) const;

	/**
	 * Returns the human-readable name of this condition.
	 * Defaults to the class name. Override for a friendlier display string.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Condition")
	FString GetConditionName() const;
	virtual FString GetConditionName_Implementation() const;

	/**
	 * Returns a URL to the documentation page for this condition.
	 * Used in editor tooltips.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category="Condition")
	FString GetConditionDocumentationLink() const;
	virtual FString GetConditionDocumentationLink_Implementation() const;

	/**
	 * Returns the stable GUID for this condition instance.
	 * Set from the Dialoguer import ID so instances can be tracked across reimports.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Condition",
		meta=(CustomTag="MounteaK2Getter"))
	FGuid GetConditionGUID() const;

	/**
	 * Overwrites the stable GUID for this instance.
	 * Called by the import pipeline to restore the original Dialoguer id.
	 */
	UFUNCTION(BlueprintCallable, Category="Condition")
	void SetConditionGUID(const FGuid& NewGUID);

public:
	
	/** Name of the condition. Used for mapping from Dialoguer and to simplify display in UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Condition",
		meta=(NoResetToDefault))
	FName ConditionName = NAME_None;

private:
	
	/** Stable per-instance GUID. Auto-generated on creation; overwritten on import with the Dialoguer id. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Private", 
		AdvancedDisplay,
		meta=(NoResetToDefault),
		meta=(AllowPrivateAccess))
	FGuid ConditionGUID;
};

/**
 * A single condition entry on an edge, bundling the instanced condition object
 * with an optional negation flag.
 */
USTRUCT(BlueprintType)
struct MOUNTEADIALOGUESYSTEM_API FMounteaDialogueCondition
{
	GENERATED_BODY()

	/** The condition object. Must be a non-abstract subclass of UMounteaDialogueConditionBase. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, NoClear,
		Category="Condition",
		meta=(NoResetToDefault),
		meta=(ShowInnerProperties),
		meta=(AllowAbstract="false"))
	TObjectPtr<UMounteaDialogueConditionBase> ConditionClass = nullptr;

	/** When true, the condition result is inverted before being combined with others. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Condition",
		meta=(NoResetToDefault))
	bool bNegate = false;
};

/**
 * Holds the full condition set for a single edge, together with the evaluation mode.
 */
USTRUCT(BlueprintType)
struct MOUNTEADIALOGUESYSTEM_API FMounteaDialogueEdgeConditions
{
	GENERATED_BODY()

	/** Ordered list of conditions. Empty list means the edge is always traversable. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conditions",
		meta=(NoResetToDefault),
		meta=(ShowOnlyInnerProperties))
	TArray<FMounteaDialogueCondition> Rules;

	/** Whether all or any conditions must pass. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conditions",
		meta=(NoResetToDefault))
	EConditionEvaluationMode Mode = EConditionEvaluationMode::All;
};
