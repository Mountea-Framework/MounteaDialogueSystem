// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueSystemEditor.h"

#include "AssetToolsModule.h"
#include "HttpModule.h"
#include "AssetActions/MounteaDialogueAdditionalDataAssetAction.h"
#include "AssetActions/MounteaDialogueDecoratorAssetAction.h"
#include "AssetActions/MounteaDialogueGraphAssetAction.h"
#include "DetailsPanel/MounteaDialogueDecorator_DetailsPanel.h"
#include "DetailsPanel/MounteaDialogueGraphNode_Details.h"
#include "Ed/EdNode_MounteaDialogueGraphEdge.h"
#include "Ed/EdNode_MounteaDialogueGraphNode.h"
#include "Ed/SEdNode_MounteaDialogueGraphNode.h"
#include "Ed/SEdNode_MounteaDialogueGraphEdge.h"
#include "EditorStyle/FMounteaDialogueGraphEditorStyle.h"

#include "Helpers/MounteaDialogueGraphEditorHelpers.h"

#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Popups/MDSPopup.h"
#include "Serialization/JsonReader.h"
#include "Styling/SlateStyleRegistry.h"

#include "ToolMenus.h"
#include "HelpButton/MDSCommands.h"
#include "HelpButton/MDSHelpStyle.h"
#include "Interfaces/IMainFrameModule.h"

const FString ChangelogURL = FString("https://raw.githubusercontent.com/Mountea-Framework/MounteaDialogueSystem/4.26/CHANGELOG.md");

#define LOCTEXT_NAMESPACE "FMounteaDialogueSystemEditor"

class FGraphPanelNodeFactory_MounteaDialogueGraph : public FGraphPanelNodeFactory
{
public:
	
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
	// Try to request Changelog from GitHub
	{
		Http = &FHttpModule::Get();
		SendHTTPGet();
	}
	
	// Button Icons
	{
		FMounteaDialogueGraphEditorStyle::Initialize();
	}
	
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

		MounteaDialogueAdditionalDataAssetActions = MakeShared<FMounteaDialogueAdditionalDataAssetAction>();
		FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(MounteaDialogueAdditionalDataAssetActions.ToSharedRef());
		
		MounteaDialogueDecoratorAssetAction = MakeShared<FMounteaDialogueDecoratorAssetAction>();
		FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(MounteaDialogueDecoratorAssetAction.ToSharedRef());
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
				}
				
				FSlateImageBrush* DialogueAdditionalDataSetClassThumb = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/AdditionalDialogueData"), TEXT(".png")), FVector2D(128.f, 128.f));
				FSlateImageBrush* DialogueAdditionalDataSetClassIcon = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/AdditionalDialogueData"), TEXT(".png")), FVector2D(16.f, 16.f));
				if (DialogueAdditionalDataSetClassIcon && DialogueAdditionalDataSetClassThumb)
				{
					DialogueTreeSet->Set("ClassThumbnail.DialogueAdditionalData", DialogueAdditionalDataSetClassThumb);
					DialogueTreeSet->Set("ClassIcon.DialogueAdditionalData", DialogueAdditionalDataSetClassIcon);
				}

				FSlateImageBrush* DialogueDecoratorSetClassThumb = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueDecorator"), TEXT(".png")), FVector2D(128.f, 128.f));
				FSlateImageBrush* DialogueDecoratorDataSetClassIcon = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueDecorator_Small"), TEXT(".png")), FVector2D(16.f, 16.f));
				if (DialogueDecoratorSetClassThumb && DialogueDecoratorDataSetClassIcon)
				{
					DialogueTreeSet->Set("ClassThumbnail.MounteaDialogueDecoratorBase", DialogueDecoratorSetClassThumb);
					DialogueTreeSet->Set("ClassIcon.MounteaDialogueDecoratorBase", DialogueDecoratorDataSetClassIcon);
				}

				FSlateImageBrush* DialogueManagerSetClassThumb = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueManagerIcon"), TEXT(".png")), FVector2D(128.f, 128.f));
				FSlateImageBrush* DialogueManagerDataSetClassIcon = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueManagerIcon"), TEXT(".png")), FVector2D(16.f, 16.f));
				if (DialogueManagerSetClassThumb && DialogueManagerDataSetClassIcon)
				{
					DialogueTreeSet->Set("ClassThumbnail.MounteaDialogueManager", DialogueManagerSetClassThumb);
					DialogueTreeSet->Set("ClassIcon.MounteaDialogueManager", DialogueManagerDataSetClassIcon);
				}

				FSlateImageBrush* DialogueParticipantSetClassThumb = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueParticipantIcon"), TEXT(".png")), FVector2D(128.f, 128.f));
				FSlateImageBrush* DialogueParticipantDataSetClassIcon = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueParticipantIcon"), TEXT(".png")), FVector2D(16.f, 16.f));
				if (DialogueParticipantSetClassThumb && DialogueParticipantDataSetClassIcon)
				{
					DialogueTreeSet->Set("ClassThumbnail.MounteaDialogueParticipant", DialogueParticipantSetClassThumb);
					DialogueTreeSet->Set("ClassIcon.MounteaDialogueParticipant", DialogueParticipantDataSetClassIcon);
				}

				//Register the created style
				FSlateStyleRegistry::RegisterSlateStyle(*DialogueTreeSet.Get());
			}
		}
	}

	//Register custom Buttons for Decorators
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		//TODO: Implement showing/hiding custom categories in Graph
		/*
		{
			TArray<FOnGetDetailCustomizationInstance> CustomClassLayouts =
			{
				FOnGetDetailCustomizationInstance::CreateStatic(&FMounteaDialogueGraphNode_Details::MakeInstance),
			};
			RegisteredCustomClassLayouts =
			{
				UMounteaDialogueGraphNode::StaticClass()->GetFName(),
			};
			for (int32 i = 0; i < RegisteredCustomClassLayouts.Num(); i++)
			{
				PropertyModule.RegisterCustomClassLayout(RegisteredCustomClassLayouts[i], CustomClassLayouts[i]);
			}
		}
		*/
		{
			TArray<FOnGetPropertyTypeCustomizationInstance> CustomPropertyTypeLayouts =
		   {
				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMounteaDialogueDecorator_DetailsPanel::MakeInstance),
			};
			RegisteredCustomPropertyTypeLayout =
			{
				FMounteaDialogueDecorator::StaticStruct()->GetFName(),
			};
			for (int32 i = 0; i < RegisteredCustomPropertyTypeLayout.Num(); i++)
			{
				PropertyModule.RegisterCustomPropertyTypeLayout(RegisteredCustomPropertyTypeLayout[i], CustomPropertyTypeLayouts[i]);
			}
		}

		PropertyModule.NotifyCustomizationModuleChanged();
	}

	// Register Help Button
	{
		FMDSHelpStyle::Initialize();
		FMDSHelpStyle::ReloadTextures();

		FMDSCommands::Register();

		PluginCommands = MakeShareable(new FUICommandList);

		PluginCommands->MapAction
		(
			FMDSCommands::Get().PluginAction,
			FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::PluginButtonClicked), 
			FCanExecuteAction()
		);
		/*
		PluginCommands->MapAction
		(
			FMDSCommands::Get().WikiAction,
			FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::PluginButtonClicked), 
			FCanExecuteAction()
		);
		*/
		IMainFrameModule& mainFrame = FModuleManager::Get().LoadModuleChecked<IMainFrameModule>("MainFrame");
		mainFrame.GetMainFrameCommandBindings()->Append(PluginCommands.ToSharedRef());

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMounteaDialogueSystemEditor::RegisterMenus));
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
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			{
				FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(MounteaDialogueGraphAssetActions.ToSharedRef());
			}
		}
	}
	
	// Unregister all the asset types that we registered
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
			{
				AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
			}
		}
	}
	
	// Unregister new Graph
	{
		if (GraphPanelNodeFactory_MounteaDialogueGraph.IsValid())
		{
			FEdGraphUtilities::UnregisterVisualNodeFactory(GraphPanelNodeFactory_MounteaDialogueGraph);
			GraphPanelNodeFactory_MounteaDialogueGraph.Reset();
		}
	}
	
	// Button Icons
	{
		FMounteaDialogueGraphEditorStyle::Shutdown();
	}

	// Help Button Cleanup
	{
		UToolMenus::UnRegisterStartupCallback(this);

		UToolMenus::UnregisterOwner(this);

		FMDSHelpStyle::Shutdown();

		FMDSCommands::Unregister();
	}
		
	EditorLOG_WARNING(TEXT("MounteaDialogueSystemEditor module has been unloaded"));
}

void FMounteaDialogueSystemEditor::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

void FMounteaDialogueSystemEditor::OnGetResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString ResponseBody;
	if (Response.Get() == nullptr) return;
	
	if (Response.IsValid() && Response->GetResponseCode() == 200)
	{
		ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	}

	// Register Popup even if we have no response, this way we can show at least something
	{
		MDSPopup::Register(ResponseBody);
	}
}

void FMounteaDialogueSystemEditor::SendHTTPGet()
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	
	Request->OnProcessRequestComplete().BindRaw(this, &FMounteaDialogueSystemEditor::OnGetResponse);
	Request->SetURL(ChangelogURL);

	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", "text");
	Request->ProcessRequest();
}

void FMounteaDialogueSystemEditor::PluginButtonClicked()
{
	const FString URL = "https://discord.gg/2vXWEEN";

	if (!URL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}
}

void FMounteaDialogueSystemEditor::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	// Register in Window tab
	{
		if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Help"))
		{
			if (Menu->ContainsSection("MounteaFramework") == false)
			{
				FToolMenuSection& Section = Menu->FindOrAddSection("Mountea Framework");
				
				Section.InsertPosition.Position = EToolMenuInsertType::First;
				Section.Label = FText::FromString(TEXT("Mountea Framework"));
				
				FToolMenuEntry SupportEntry = Section.AddMenuEntryWithCommandList
				(
					FMDSCommands::Get().PluginAction,
					PluginCommands,
					NSLOCTEXT("MounteaSupport", "TabTitle", "Mountea Support"),
					NSLOCTEXT("MounteaSupport", "TooltipText", "Opens Mountea Framework Support channel"),
					FSlateIcon(FMDSHelpStyle::GetStyleSetName(), "MDSHelpStyleSet.Toolbar.HelpIcon.small")
				);
				SupportEntry.Name = FName("MounteaFrameworkSupport");
				
				/*
				FToolMenuEntry WikiEntry = Section.AddMenuEntryWithCommandList
				(
					FMDSCommands::Get().WikiAction,
					PluginCommands,
					NSLOCTEXT("MounteaDialogueWiki", "TabTitle", "Dialogue Wiki"),
					NSLOCTEXT("MounteaDialogueWiki", "TooltipText", "Opens Mountea Dialogue System Wiki page"),
					FSlateIcon(FMDSHelpStyle::GetStyleSetName(), "MDSHelpStyleSet.Toolbar.HelpIcon.small")
				);
				WikiEntry.Name = FName("MounteaDialogueWiki");
				*/
			}
		}
	}

	// Register in Level Editor Toolbar
	{
		if (UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
		{
			if (ToolbarMenu->ContainsSection("MounteaFramework") == false)
			{
				FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("Mountea Framework");
				Section.Label = FText::FromString(TEXT("Mountea Framework"));
			
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FMDSCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			
				Entry.InsertPosition.Position = EToolMenuInsertType::First;
				Entry.Icon = FSlateIcon(FMDSHelpStyle::GetStyleSetName(), "MDSHelpStyleSet.Toolbar.HelpIcon");
				Entry.Name = FName("MounteaFrameworkSupport");
			}
		}

	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMounteaDialogueSystemEditor, MounteaDialogueSystemEditor)