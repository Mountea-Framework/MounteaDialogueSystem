// All rights reserved Dominik Morse (Pavlicek) 2024.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Widgets/Layout/Anchors.h"
#include "MounteaDialogueUIBFL.generated.h"

class UMounteaDialogueGraphNode;
class UMounteaDialogueGraphNode_DialogueNodeBase;

struct FDialogueRowData;
struct FDialogueOptionData;
struct FDialogueRow;
struct FWidgetDialogueRow;

/**
 * 
 */
UCLASS(DisplayName="Mountea Dialogue UI Function Library")
class MOUNTEADIALOGUESYSTEM_API UMounteaDialogueUIBFL : public UBlueprintFunctionLibrary
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
	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta = (BlueprintThreadSafe), meta=(CustomTag="MounteaK2Setter"))
	static FDialogueOptionData NewDialogueOptionData(const FGuid& Node, const FDialogueRow& DialogueRow);

	/**
	 * Creates a new dialogue widget row data structure.
	 * 
	 * @param SourceRow The source row from which to create the widget data.
	 * @param SourceRowData Additional data associated with the source row.
	 * @return A new `FWidgetDialogueRow` structure containing the source row and its data.
	 */
	UFUNCTION(BlueprintPure, Category="Mountea|Dialogue|Helpers|UI", meta = (BlueprintThreadSafe), meta=(CustomTag="MounteaK2Setter"))
	static FWidgetDialogueRow NewDialogueWidgetRowData(const FDialogueRow& SourceRow, const FDialogueRowData& SourceRowData);

	/**
	 * Retrieves the GUID of a dialogue node.
	 * 
	 * @param FromNode The dialogue node from which to retrieve the GUID.
	 * @return The GUID associated with the specified node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FGuid GetDialogueNodeGuid(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	/**
	 * Retrieves the dialogue row associated with a specific node.
	 * 
	 * @param FromNode The dialogue node from which to retrieve the row.
	 * @return The dialogue row associated with the specified node.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
	static FDialogueRow GetDialogueNodeRow(UMounteaDialogueGraphNode_DialogueNodeBase* FromNode);

	/**
	 * Filters an array of dialogue nodes to only include dialogue-friendly nodes.
	 * 
	 * @param RawNodes The array of raw dialogue nodes to be filtered.
	 * @return An array of nodes that are classified as dialogue-friendly.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|UI", meta=(CustomTag="MounteaK2Getter"))
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

	/**
	 * Retrieves the base widget class used for the viewport from the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @return                   The subclass of UUserWidget used as the base class for the viewport, or nullptr if an error occurs.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Getter", HideSelfPin="true"))
	static TSubclassOf<UUserWidget> GetViewportBaseClass(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueHUDClassInterface")) AActor* ViewportManager);
	
	/**
	 * Initializes the viewport widget using the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Setter", HideSelfPin="true"))
	static void InitializeViewportWidget(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueHUDClassInterface")) AActor* ViewportManager);

	/**
	 * Retrieves the viewport widget from the specified viewport manager that implements the MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @return                   The UUserWidget representing the viewport, or nullptr if the viewport manager does not implement the interface or an error occurs.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Setter", HideSelfPin="true"))
	static UUserWidget* GetViewportWidget(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueHUDClassInterface")) AActor* ViewportManager);
	
	/**
	 * Adds a child widget to the viewport using the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @param ChildWidget        The child widget to be added to the viewport.
	 * @param ZOrder             The Z-order index of the child widget, determining its rendering order within the viewport.
	 * @param WidgetAnchors      The anchors for the widget, determining how the widget is positioned relative to the viewport.
	 * @param WidgetMargin       The margin for the widget, defining the padding or offset from the viewport bounds.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Setter", HideSelfPin="true"))
	static void AddChildWidgetToViewport(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueHUDClassInterface")) AActor* ViewportManager, UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(), const FMargin& WidgetMargin = FMargin());

	/**
	 * Removes a child widget from the viewport using the specified viewport manager that implements MounteaDialogueHUDClassInterface.
	 *
	 * @param ViewportManager    The viewport manager, an actor that implements the MounteaDialogueHUDClassInterface.
	 * @param ChildWidget        The child widget to be removed from the viewport.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Setter", HideSelfPin="true"))
	static void RemoveChildWidgetFromViewport(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueHUDClassInterface")) AActor* ViewportManager, UUserWidget* ChildWidget);
	
	/**
	 * Adds a child widget to the specified parent widget that implements the MounteaDialogueViewportWidgetInterface.
	 *
	 * @param ParentWidget    The parent widget that should implement the MounteaDialogueViewportWidgetInterface.
	 * @param ChildWidget     The child widget to be added.
	 * @param ZOrder          The Z-order index of the child widget, determining its rendering order within the parent.
	 * @param WidgetAnchors   The anchors for the widget, determining how the widget is positioned relative to its parent.
	 * @param WidgetMargin    The margin for the widget, defining the padding or offset from the parent's bounds.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Setter", HideSelfPin="true"))
	static void AddChildWidget(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueViewportWidgetInterface")) UUserWidget* ParentWidget, UUserWidget* ChildWidget, const int32 ZOrder = 0, const FAnchors WidgetAnchors = FAnchors(), const FMargin& WidgetMargin = FMargin());

	/**
	 * Removes a child widget from the specified parent widget that implements the MounteaDialogueViewportWidgetInterface.
	 *
	 * @param ParentWidget    The parent widget that should implement the MounteaDialogueViewportWidgetInterface.
	 * @param ChildWidget     The child widget to be removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Mountea|Dialogue|Helpers|HUD", meta=(CustomTag="MounteaK2Setter", HideSelfPin="true"))
	static void RemoveChildWidget(UPARAM(meta=(MustImplement="/Script/MounteaDialogueSystem.MounteaDialogueViewportWidgetInterface")) UUserWidget* ParentWidget, UUserWidget* ChildWidget);

};
