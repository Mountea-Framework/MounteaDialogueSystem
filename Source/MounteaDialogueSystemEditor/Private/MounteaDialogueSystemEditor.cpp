#include "MounteaDialogueSystemEditor.h"

#include "AssetToolsModule.h"
#include "AssetActions/MounteaDialogueGraphAssetAction.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Ed/SEdNode_MounteaDialogueGraphNode.h"
#include "Ed/SEdNode_MounteaDialogueGraphEdge.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"

#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define LOCTEXT_NAMESPACE "FMounteaDialogueSystemEditor"

class FGraphPanelNodeFactory_MounteaDialogueGraph : public FGraphPanelNodeFactory
{
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override
	{
		
		if (UEdNode_MounteaDialogueGraphNode* EdNode_GraphNode = Cast<UEdNode_MounteaDialogueGraphNode>(Node))
		{
			return SNew(SEdNode_MounteaDialogueGraphNode, EdNode_GraphNode);
		}
		else if (UEdNode_MounteaDialogueGraphEdge* EdNode_Edge = Cast<UEdNode_MounteaDialogueGraphEdge>(Node))
		{
			return SNew(SEdNode_MounteaDialogueGraphEdge, EdNode_Edge);
		}
		
		return nullptr;
	}
};

void FMounteaDialogueSystemEditor::StartupModule()
{
	FMounteaDialogueGraphEditorStyle::Initialize();

	// Register new Graph
	{
		GraphPanelNodeFactory_MounteaDialogueGraph = MakeShareable(new FGraphPanelNodeFactory_MounteaDialogueGraph());
		FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory_MounteaDialogueGraph);
	}
	
	// Register new Category
	{
		FAssetToolsModule::GetModule().Get().RegisterAdvancedAssetCategory(FName("Mountea Dialogue"), FText::FromString("Mountea Dialogue"));
	}

	// Asset Actions
	{
		MounteaDialogueGraphAssetActions = MakeShared<FMounteaDialogueGraphAssetAction>();
		FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(MounteaDialogueGraphAssetActions.ToSharedRef());
	}

	// Thumbnails and Icons
	{
		DialogueTreeSet = MakeShareable(new FSlateStyleSet("DialogueTreeSet"));

		const TSharedPtr<IPlugin> PluginPtr = IPluginManager::Get().FindPlugin("MounteaDialogueSystem");

		if (PluginPtr.IsValid())
		{
			const FString ContentDir = IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir();
        	
			// Interactor Component
			{
				DialogueTreeSet->SetContentRoot(ContentDir);
        		
				FSlateImageBrush* DialogueTreeSetClassThumb = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueTreeIcon_128"), TEXT(".png")), FVector2D(128.f, 128.f));
				FSlateImageBrush* DialogueTreeSetClassIcon = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueTreeIcon_16"), TEXT(".png")), FVector2D(16.f, 16.f));
				if (DialogueTreeSetClassThumb && DialogueTreeSetClassIcon)
				{
					DialogueTreeSet->Set("ClassThumbnail.MounteaDialogueGraph", DialogueTreeSetClassThumb);
					DialogueTreeSet->Set("ClassIcon.MounteaDialogueGraph", DialogueTreeSetClassIcon);
     
					//Register the created style
					FSlateStyleRegistry::RegisterSlateStyle(*DialogueTreeSet.Get());
				}
			}
		}
	}
	
	EditorLOG_WARNING(TEXT("MounteaDialogueSystemEditor module has been loaded"));
}

void FMounteaDialogueSystemEditor::ShutdownModule()
{
	// Thumbnails and Icons
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(DialogueTreeSet->GetStyleSetName());
	}

	// Asset Types Cleanup
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		{
			FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(MounteaDialogueGraphAssetActions.ToSharedRef());
		}
	}

	// Unregister all the asset types that we registered
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}

	if (GraphPanelNodeFactory_MounteaDialogueGraph.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_MounteaDialogueGraph);
		GraphPanelNodeFactory_MounteaDialogueGraph.Reset();
	}

	FMounteaDialogueGraphEditorStyle::Shutdown();
	
	EditorLOG_WARNING(TEXT("MounteaDialogueSystemEditor module has been unloaded"));
}

void FMounteaDialogueSystemEditor::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMounteaDialogueSystemEditor, MounteaDialogueSystemEditor)