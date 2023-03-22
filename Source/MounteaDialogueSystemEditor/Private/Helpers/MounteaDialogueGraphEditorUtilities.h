#pragma once
#include "ClassViewerFilter.h"
#include "K2Node_Event.h"

class UMounteaDialogueGraph;
class FAssetEditor_MounteaDialogueGraph;

enum class EMounteaDialogueBlueprintOpenType : uint8
{
	None = 0,
	Function,
	Event
};

class FMounteaDialogueClassViewerFilter : public IClassViewerFilter
{
public:
	// All children of these classes will be included unless filtered out by another setting.
	TSet<const UClass*> AllowedChildrenOfClasses;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
	{
		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
	}

private:
	// Disallowed class flags.
	EClassFlags DisallowedClassFlags = CLASS_Deprecated;
};

class FMounteaDialogueGraphEditorUtilities
{
	
public:

	static bool PickChildrenOfClass(const FText& TitleText, UClass*& OutChosenClass, UClass* Class);
	static bool OpenBlueprintEditor
	(
		UBlueprint* Blueprint,
		EMounteaDialogueBlueprintOpenType OpenType = EMounteaDialogueBlueprintOpenType::None,
		FName FunctionNameToOpen = NAME_None,
		bool bForceFullEditor = true,
		bool bAddBlueprintFunctionIfItDoesNotExist = false
	);

	static UEdGraph* BlueprintGetOrAddFunction(UBlueprint* Blueprint, FName FunctionName, UClass* FunctionClassSignature);
	
	static UEdGraph* BlueprintGetFunction(UBlueprint* Blueprint, FName FunctionName, UClass* FunctionClassSignature);
	
	static UK2Node_Event* BlueprintGetOrAddEvent(UBlueprint* Blueprint, FName EventName, UClass* EventClassSignature);
	
	static UK2Node_Event* BlueprintGetEvent(UBlueprint* Blueprint, FName EventName, UClass* EventClassSignature);

	static bool OpenEditorForAsset(const UObject* Asset);

	static bool IsABlueprintClass(const UClass* Class)
	{
		return Cast<UBlueprintGeneratedClass>(Class) != nullptr;
	}

	static bool GetAllChildClassesOf(const UClass* ParentClass, TArray<UClass*>& OutNativeClasses, TArray<UClass*>& OutBlueprintClasses)
	{
		// Iterate over UClass, this might be heavy on performance
		for (TObjectIterator<UClass> It; It; ++It)
		{
			UClass* ChildClass = *It;
			if (!ChildClass->IsChildOf(ParentClass))
			{
				continue;
			}

			// It is a child of the Parent Class
			// make sure we don't include our parent class in the array
			if (ChildClass == ParentClass)
			{
				continue;
			}

			if (IsABlueprintClass(ChildClass))
			{
				// Blueprint
				OutBlueprintClasses.Add(ChildClass);
			}
			else
			{
				// Native
				OutNativeClasses.Add(ChildClass);
			}
		}

		return OutNativeClasses.Num() > 0 || OutBlueprintClasses.Num() > 0;
	}

	static bool OpenEditorAndJumpToGraphNode(TWeakPtr<FAssetEditor_MounteaDialogueGraph> DialogueEditorPtr, const UEdGraphNode* GraphNode, bool bFocusIfOpen = false);

	static UMounteaDialogueGraph* GetDialogueFromGraphNode(const UEdGraphNode* GraphNode);

	static IAssetEditorInstance* FindEditorForAsset(UObject* Asset, bool bFocusIfOpen);
};


