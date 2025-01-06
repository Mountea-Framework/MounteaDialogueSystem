// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MounteaDialogueOptionsContainerInterface.generated.h"

class UMounteaDialogueGraphNode_DialogueNodeBase;
class IMounteaDialogueWBPInterface;
class IMounteaDialogueOptionInterface;

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType, Blueprintable)
class UMounteaDialogueOptionsContainerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class MOUNTEADIALOGUESYSTEM_API IMounteaDialogueOptionsContainerInterface
{
	GENERATED_BODY()
public:
	
	/**
	 * Sets the parent dialogue widget.
	 * 
	 * @param NewParentDialogueWidget The UUserWidget instance to be set as the parent dialogue widget. The widget must implement the MounteaDialogueWBPInterface.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void SetParentDialogueWidget(  UPARAM(Meta = (MustImplement = "/Script/MounteaDialogueSystem.MounteaDialogueWBPInterface")) UUserWidget* NewParentDialogueWidget );
	virtual void SetParentDialogueWidget_Implementation(UUserWidget* NewParentDialogueWidget) = 0;

	/**
	 * Gets the parent dialogue widget.
	 * 
	 * @return A UUserWidget instance that serves as the parent dialogue widget.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	UUserWidget* GetParentDialogueWidget() const;
	virtual UUserWidget* GetParentDialogueWidget_Implementation() const = 0;
	
	/**
	 * Gets the class type of the dialogue option widget.
	 * 
	 * @return A soft class pointer to the UUserWidget that represents the dialogue option.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	TSoftClassPtr<UUserWidget> GetDialogueOptionClass() const;
	virtual TSoftClassPtr<UUserWidget> GetDialogueOptionClass_Implementation() const = 0;

	/**
	 * Sets the class type of the dialogue option widget.
	 * 
	 * @param NewDialogueOptionClass A soft class pointer to the new UUserWidget class to be used for dialogue options.
	 * The class must implement the MounteaDialogueOptionInterface.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void SetDialogueOptionClass(UPARAM(Meta = (MustImplement = "/Script/MounteaDialogueSystem.MounteaDialogueOptionInterface")) const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass);
	virtual void SetDialogueOptionClass_Implementation(const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass) = 0;

	/**
	 * Adds a new dialogue option widget.
	 * 
	 * @param NewDialogueOption The UUserWidget instance to be added as a new dialogue option.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void AddNewDialogueOption( UMounteaDialogueGraphNode_DialogueNodeBase* NewDialogueOption);
	virtual void AddNewDialogueOption_Implementation(UMounteaDialogueGraphNode_DialogueNodeBase* NewDialogueOption) = 0;

	/**
	 * Adds multiple new dialogue option widgets.
	 * 
	 * @param NewDialogueOptions An array of UUserWidget instances to be added as new dialogue options.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void AddNewDialogueOptions(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& NewDialogueOptions);
	virtual void AddNewDialogueOptions_Implementation(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& NewDialogueOptions) = 0;

	/**
	 * Removes a specific dialogue option widget.
	 * 
	 * @param DirtyDialogueOption The UUserWidget instance to be removed from the dialogue options.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void RemoveDialogueOption(UMounteaDialogueGraphNode_DialogueNodeBase* DirtyDialogueOption);
	virtual void RemoveDialogueOption_Implementation(UMounteaDialogueGraphNode_DialogueNodeBase* DirtyDialogueOption) = 0;

	/**
	 * Removes multiple dialogue option widgets.
	 * 
	 * @param DirtyDialogueOptions An array of UUserWidget instances to be removed from the dialogue options.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void RemoveDialogueOptions(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& DirtyDialogueOptions);
	virtual void RemoveDialogueOptions_Implementation(const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& DirtyDialogueOptions) = 0;

	/**
	 * Clears all dialogue option widgets.
	 * 
	 * Removes all currently stored dialogue options, effectively resetting the container.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void ClearDialogueOptions();
	virtual void ClearDialogueOptions_Implementation() = 0;

	/**
	 * Processes the selected option.
	 * 
	 * @param SelectedOption The selected option's identifier.
	 * @param CallingWidget The widget that triggered the selection process.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void ProcessOptionSelected(const FGuid& SelectedOption, UUserWidget* CallingWidget);
	virtual void ProcessOptionSelected_Implementation(const FGuid& SelectedOption, UUserWidget* CallingWidget) = 0;

	/**
	 * Returns all Dialogue Options from the specified Dialogue Options Container parent widget that implements the MounteaDialogueOptionsContainerInterface.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	TArray<UUserWidget*> GetDialogueOptions() const;
	virtual TArray<UUserWidget*> GetDialogueOptions_Implementation() const = 0;

	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void ToggleForcedFocus(const bool bEnable);
	virtual void ToggleForcedFocus_Implementation(const bool bEnable) = 0;

	/**
	 *	Gets focused option. If options are empty then -1 is returned.
	 * 
	 * @return Index of currently focused Option
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	int32 GetFocusedOptionIndex() const;
	virtual int32 GetFocusedOptionIndex_Implementation() const = 0;

	/**
	 *	Set focus to new Option.
	 * 
	 * @param NewFocusedOption Index of newly focused Option.
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Mountea|Dialogue|UserInterface|OptionsContainer")
	void SetFocusedOption(const int32 NewFocusedOption);
	virtual void SetFocusedOption_Implementation(const int32 NewFocusedOption) = 0;
};
