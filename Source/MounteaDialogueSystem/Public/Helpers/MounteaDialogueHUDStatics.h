// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/UMG/MounteaFocusableWidgetInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Layout/Anchors.h"
#include "Layout/Margin.h"
#include "MounteaDialogueHUDStatics.generated.h"

class IMounteaDialogueManagerInterface;
class UMounteaDialogueGraphNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;

struct FDialogueRowData;
struct FDialogueOptionData;
struct FDialogueRow;
struct FWidgetDialogueRow;
class IMounteaDialogueOptionsContainerInterface;

USTRUCT(BlueprintType)
struct FWidgetAdditionParams
{
	GENERATED_BODY()

	FWidgetAdditionParams()
	{
		ZOrder = 0;
		Anchors = FAnchors(0.f, 0.f, 1.f, 1.f);
		Margin = FMargin();
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Helpers|UI")
	int32 ZOrder;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Helpers|UI")
	FAnchors Anchors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mountea|Dialogue|Helpers|UI")
	FMargin Margin;
	
	static FWidgetAdditionParams GetDefault()
	{
		return FWidgetAdditionParams();
	}
};

/**
 * 
 */
UCLASS(DisplayName="Mountea Dialogue HUD Function Library")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueHUDStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Creates a new dialogue option data structure.
	 * 
	 * @param Node The GUID of the dialogue node for which the option is created.
	 * @param DialogueRow The row data associated with the dialogue node.
	 * @return A new `FDialogueOptionData` structure containing the node and row data.
	 */
	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Setter"))
	static FDialogueOptionData NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow);

	/**
	 * Creates a new dialogue widget row data structure.
	 * 
	 * @param SourceRow The source row from which to create the widget data.
	 * @param SourceRowData Additional data associated with the source row.
	 * @return A new `FWidgetDialogueRow` structure containing the source row and its data.
	 */
	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Setter"))
	static FWidgetDialogueRow NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData);

	/**
	 * Retrieves the GUID of a dialogue node.
	 * 
	 * @param FromNode The dialogue node from which to retrieve the GUID.
	 * @return The GUID associated with the specified node.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FGuid GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	/**
	 * Retrieves the dialogue row associated with a specific node.
	 * 
	 * @param FromNode The dialogue node from which to retrieve the row.
	 * @return The dialogue row associated with the specified node.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	/**
	 * Filters an array of dialogue nodes to only include dialogue-friendly nodes.
	 * 
	 * @param RawNodes The array of raw dialogue nodes to be filtered.
	 * @return An array of nodes that are classified as dialogue-friendly.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static TArray<UMounteaDialogueGraphNode_DialogueNodeBase*> FilterDialogueFriendlyNodes(const TArray<UMounteaDialogueGraphNode*>& RawNodes);

	/**
	 * Replaces text in a source string using a regular expression.
	 * 
	 * @param Regex The regular expression pattern to search for in the source text.
	 * @param Replacement The text to replace the matched pattern.
	 * @param SourceText The original text where the replacement will occur.
	 * @return The modified text with the replacements applied.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Setter"))
	static FText ReplaceRegexInText(const FString& Regex, const FText& Replacement, const FText& SourceText);

	/**
	 * Gets the Z-order of the specified widget within the viewport or its parent container.
	 *
	 * @param Widget         The widget for which to retrieve the Z-order.
	 * @param WorldContext   The context object for the current world, typically passed in for blueprint purposes.
	 * @return               The Z-order of the widget, or -1 if the widget is not found or an error occurs.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static int32 GetWidgetZOrder(class UUserWidget* Widget, UObject* WorldContext);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static EDialogueOptionState GetFocusState(class UUserWidget* Widget);

	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Setter"))
	static void SetFocusState(UUserWidget* Widget, const bool IsFocused);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static int32 GetSafeOptionIndex(UObject* Container, const EUINavigation Direction);
	
	// --- HUD Interface functions ------------------------------

	/**
	 * Retrieves the base widget class used for the viewport from the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @return                   The subclass of UUserWidget used as the base class for the viewport, or nullptr if an error occurs.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Getter"))
	static TSubclassOf<UUserWidget> GetViewportBaseClass(AActor* ViewportManager);
	
	/**
	 * Initializes the viewport widget using the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Setter"))
	static void InitializeViewportWidget(AActor* ViewportManager);

	/**
	 * Retrieves the viewport widget from the specified viewport manager that implements the MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @return                   The UUserWidget representing the viewport, or nullptr if the viewport manager does not implement the interface or an error occurs.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Getter"))
	static UUserWidget* GetViewportWidget(AActor* ViewportManager);
	
	/**
	 * Adds a child widget to the viewport using the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @param ChildWidget        The child widget to be added to the viewport.
	 * @param WidgetParams   Contains 3 options:\n 1. the anchors for the widget, determining how the widget is positioned relative to its parent\n2. the margin for the widget, defining the padding or offset from the parent's bounds\n3. The Z-order index of the child widget, determining its rendering order within the parent
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Setter"))
	static void AddChildWidgetToViewport(AActor* ViewportManager, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams = FWidgetAdditionParams());

	/**
	 * Removes a child widget from the viewport using the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @param ChildWidget        The child widget to be removed from the viewport.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Setter"))
	static void RemoveChildWidgetFromViewport(AActor* ViewportManager, UUserWidget* ChildWidget);

	// --- Viewport Widget Interface functions ------------------------------
	
	/**
	 * Adds a child widget to the specified parent widget that implements the MounteaDialogueViewportWidgetInterface.
	 *
	 * @param ParentWidget    The parent widget that should implement the MounteaDialogueViewportWidgetInterface.
	 * @param ChildWidget     The child widget to be added.
	 * @param WidgetParams   Contains 3 options:\n 1. the anchors for the widget, determining how the widget is positioned relative to its parent\n2. the margin for the widget, defining the padding or offset from the parent's bounds\n3. The Z-order index of the child widget, determining its rendering order within the parent
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Setter"))
	static void AddChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget, const FWidgetAdditionParams& WidgetParams = FWidgetAdditionParams());

	/**
	 * Removes a child widget from the specified parent widget that implements the MounteaDialogueViewportWidgetInterface.
	 *
	 * @param ParentWidget    The parent widget that should implement the MounteaDialogueViewportWidgetInterface.
	 * @param ChildWidget     The child widget to be removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|HUD|Viewport", meta=(CustomTag="MounteaK2Setter"))
	static void RemoveChildWidget(UUserWidget* ParentWidget, UUserWidget* ChildWidget);

	// --- Dialogue Base UI Interface functions ------------------------------

	/**
	 * Generic helper function to provide a global way to bind UI events.
	 * 
	 * @param Target    The widget that should implement the MounteaDialogueUIBaseInterface.
	 * @return Binding result.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	static bool BindEvents(UUserWidget* Target);

	/**
	 * Generic helper function to provide a global way to unbind UI events.
	 * 
	 * @param Target    The widget that should implement the MounteaDialogueUIBaseInterface.
	 * @return Binding result.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	static bool UnbindEvents(UUserWidget* Target);

	/**
	 * Generic helper function to provide easy way of sending commands around without need of binding.
	 * 
	 * @param Target    The widget that should implement the MounteaDialogueUIBaseInterface.
	 * @param Command				Required string command to drive inner logic.
	 * @param OptionalPayload	Optional payload which can contain data for command.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessStringCommand(UUserWidget* Target, const FString& Command, UObject* OptionalPayload = nullptr);

	/**
	 * Helper function to provide easy way to request Theme update.
	 * Theme logic not provided to abstract the idea from any code.
	 *
	 * @param Target    The widget that should implement the MounteaDialogueUIBaseInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface", meta=(CustomTag="MounteaK2Setter"))
	static void ApplyTheme(UUserWidget* Target);

	// --- Dialogue Widget Interface functions ------------------------------
	
	/**
	 * This event should be called when you want to refresh UI data.
	 *
	 * @param Target    The widget that should implement the MounteaDialogueWBPInterface. Usually Widget Blueprint.
	 * @param DialogueManager	Dialogue Manager Interface reference. Request 'GetDialogueContext' to retrieve data to display.
	 * @param Command			String command. All commands are defined in ProjectSettings/MounteaFramework/MounteaDialogueSystem.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Dialogue", meta=(CustomTag="MounteaK2Setter"))
	static void RefreshDialogueWidget(UObject* Target, const TScriptInterface<IMounteaDialogueManagerInterface>& DialogueManager, const FString& Command);

	/**
	 * Called when an option has been selected.
	 *
	 * @param Target    The widget that should implement the MounteaDialogueWBPInterface. Usually Widget Blueprint.
	 * @param SelectionGUID The GUID of the selected option.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Dialogue")
	static void OnOptionSelected(UObject* Target, const FGuid& SelectionGUID);
	
	// --- Dialogue Option Interface functions ------------------------------

	/**
	 * Gets the current dialogue option data.
	 *
	 * @param DialogueOption The object that should implement the MounteaDialogueOptionInterface. Usually Widget Blueprint.
	 * @return The current dialogue option data.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|UserInterface|Option", meta=(CustomTag="MounteaK2Getter"))
	static FDialogueOptionData GetDialogueOptionData(UObject* DialogueOption);

	/**
	 * Sets new dialogue option data.
	 *
	 * @param DialogueOption The object that should implement the MounteaDialogueOptionInterface. Usually Widget Blueprint.
	 * @param NewData The new data to set for the dialogue option.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Option", meta=(CustomTag="MounteaK2Setter"))
	static void SetNewDialogueOptionData(UObject* DialogueOption, const FDialogueOptionData& NewData);

	/**
	 * Resets the dialogue option data to its default state.
	 *
	 * @param DialogueOption The object that should implement the MounteaDialogueOptionInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Option", meta=(CustomTag="MounteaK2Setter"))
	static void ResetDialogueOptionData(UObject* DialogueOption);

	/**
	 * Responsible for setting visual data from Dialogue Option data.
	 *
	 * @param DialogueOption The object that should implement the MounteaDialogueOptionInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Option", meta=(CustomTag="MounteaK2Setter"))
	static void InitializeDialogueOption(UObject* DialogueOption);

	/**
	 * Processes the event of a dialogue option being selected.
	 *
	 * @param DialogueOption The object that should implement the MounteaDialogueOptionInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Option", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessOptionSelected(UObject* DialogueOption);
	
	// --- Options Container Interface functions ------------------------------

	/**
	 * Sets the parent dialogue widget.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param NewParentDialogueWidget The UUserWidget instance to be set as the parent dialogue widget. The widget must implement the MounteaDialogueWBPInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void SetParentDialogueWidget(UObject* ContainerObject, UUserWidget* NewParentDialogueWidget);

	/**
	 * Gets the parent dialogue widget.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @return A UUserWidget instance that serves as the parent dialogue widget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Getter"))
	static UUserWidget* GetParentDialogueWidget(UObject* ContainerObject);

	/**
	 * Gets the class type of the dialogue option widget.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @return A soft class pointer to the UUserWidget that represents the dialogue option.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Getter"))
	static TSoftClassPtr<UUserWidget> GetDialogueOptionClass(UObject* ContainerObject);

	/**
	 * Sets the class type of the dialogue option widget.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param NewDialogueOptionClass A soft class pointer to the new UUserWidget class to be used for dialogue options.
	 * The class must implement the MounteaDialogueOptionInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void SetDialogueOptionClass(UObject* ContainerObject, const TSoftClassPtr<UUserWidget>& NewDialogueOptionClass);
	
	/**
	 * Adds a new dialogue option widget.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param NewDialogueOption The UUserWidget instance to be added as a new dialogue option.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void AddNewDialogueOption(UObject* ContainerObject, UMounteaDialogueGraphNode_DialogueNodeBase* NewDialogueOption);

	/**
	 * Adds multiple new dialogue option widgets.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param NewDialogueOptions An array of UUserWidget instances to be added as new dialogue options.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void AddNewDialogueOptions(UObject* ContainerObject, const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& NewDialogueOptions);

	/**
	 * Removes a specific dialogue option widget.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param DirtyDialogueOption The UUserWidget instance to be removed from the dialogue options.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void RemoveDialogueOption(UObject* ContainerObject, UMounteaDialogueGraphNode_DialogueNodeBase* DirtyDialogueOption);

	/**
	 * Removes multiple dialogue option widgets.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param DirtyDialogueOptions An array of UUserWidget instances to be removed from the dialogue options.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void RemoveDialogueOptions(UObject* ContainerObject, const TArray<UMounteaDialogueGraphNode_DialogueNodeBase*>& DirtyDialogueOptions);

	/**
	 * Clears all dialogue option widgets.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * Removes all currently stored dialogue options, effectively resetting the container.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void ClearDialogueOptions(UObject* ContainerObject);

	/**
	 * Processes the selected option.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param SelectedOption The selected option's identifier.
	 * @param CallingWidget The widget that triggered the selection process.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void ProcessContainerOptionSelected(UObject* ContainerObject, const FGuid& SelectedOption, UUserWidget* CallingWidget);
	
	/**
	 * Returns all Dialogue Options from the specified Dialogue Options Container parent widget that implements the MounteaDialogueOptionsContainerInterface.
	 *
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Getter"))
	static TArray<UUserWidget*> GetDialogueOptions(UObject* ContainerObject);

	/**
	 *	Gets focused option. If options are empty then -1 is returned.
	 *	
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @return Index of currently focused Option
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Getter"))
	static int32 GetFocusedOptionIndex(UObject* ContainerObject);

	/**
	 *	Set focus to new Option.
	 *	
	 * @param ContainerObject The object that should implement the MounteaDialogueOptionsContainerInterface. Usually Widget Blueprint.
	 * @param NewFocusedOption Index of newly focused Option.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|OptionsContainer", meta=(CustomTag="MounteaK2Setter"))
	static void SetFocusedOption(UObject* ContainerObject, const int32 NewFocusedOption);

	// --- Dialogue Row Interface functions ------------------------------

	/**
	 * Retrieves the dialogue row data associated with the widget.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 * @return FWidgetDialogueRow containing the dialogue row data.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Getter"))
	static FWidgetDialogueRow GetDialogueWidgetRowData(UObject* RowObject);

	/**
	 * Sets new dialogue row data for the widget.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 * @param NewData The new dialogue row data to set.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void SetNewWidgetDialogueRowData(UObject* RowObject, const FWidgetDialogueRow& NewData);

	/**
	 * Resets the widget's dialogue row data to the default state.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void ResetWidgetDialogueRow(UObject* RowObject);

	/**
	 * Initializes the widget's dialogue row data.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void InitializeWidgetDialogueRow(UObject* RowObject);

	/**
	 * Stops the typewriter effect and finishes displaying the text.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void StopTypeWriterEffect(UObject* RowObject);

	/**
	 * Starts the typewriter effect on the specified text for a given duration.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 * @param SourceText The full text to display.
	 * @param Duration The duration over which the typewriter effect should play.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void StartTypeWriterEffect(UObject* RowObject, const FText& SourceText, float Duration);

	/**
	 * Enables or disables the typewriter effect based on the specified parameter.
	 *
	 * @param RowObject The object that should implement the MounteaDialogueRowInterface. Usually Widget Blueprint.
	 * @param bEnable Whether to enable or disable the effect.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|DialogueRow", meta=(CustomTag="MounteaK2Setter"))
	static void EnableTypeWriterEffect(UObject* RowObject, bool bEnable);

	// --- Skip Interface functions ------------------------------

	/**
	 * Requests the widget to show with a fade-in effect.
	 *
	 * @param SkipObject The object that should implement the MounteaDialogueSkipInterface. Usually Widget Blueprint.
	 * @param FadeProgressDuration A vector specifying the fade-in progress duration for the widget.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Skip", meta=(CustomTag="MounteaK2Setter"))
	static void RequestShowWidget(UObject* SkipObject, const FVector2D& FadeProgressDuration);

	/**
	 * Requests the widget to hide with a fade-out effect.
	 *
	 * @param SkipObject The object that should implement the MounteaDialogueSkipInterface. Usually Widget Blueprint.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|UserInterface|Skip", meta=(CustomTag="MounteaK2Setter"))
	static void RequestHideWidget(UObject* SkipObject);

private:
	static TScriptInterface<IMounteaDialogueOptionsContainerInterface> GetOptionsContainerInterface(UObject* ContainerObject);
};
