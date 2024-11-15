// All rights reserved Dominik Pavlicek 2023

#include "MounteaDialogueSystemEditor.h"

#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "GameplayTagsManager.h"
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
#include "AssetActions/MounteaDialogueConfigurationAssetAction.h"
#include "AssetActions/MounteaDialogueDataTableAssetAction.h"
#include "DetailsPanel/MounteaDialogueDecorator_Details.h"
#include "DetailsPanel/MounteaDialogueGraph_Details.h"
#include "Graph/MounteaDialogueGraph.h"
#include "HelpButton/MDSCommands.h"
#include "Helpers/MounteaDialogueFixUtilities.h"
#include "ImportConfig/MounteaDialogueImportConfig.h"
#include "Interfaces/IMainFrameModule.h"
#include "Settings/MounteaDialogueGraphEditorSettings.h"

const FString ChangelogURL = FString("https://raw.githubusercontent.com/Mountea-Framework/MounteaDialogueSystem/master/CHANGELOG.md");

#define LOCTEXT_NAMESPACE "FMounteaDialogueSystemEditor"

static const FName MenuName("LevelEditor.LevelEditorToolBar.PlayToolBar");

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
		SendHTTPGet_Tags();
	}
	
	// Register Style
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
		FAssetToolsModule::GetModule().Get().RegisterAdvancedAssetCategory(FName("Mountea Dialogue"), FText::FromString(TEXT("\U0001F538 Mountea Dialogue")));
	}

	// Asset Actions
	{
		AssetActions.Add(MakeShared<FMounteaDialogueGraphAssetAction>());
		AssetActions.Add(MakeShared<FMounteaDialogueAdditionalDataAssetAction>());
		AssetActions.Add(MakeShared<FMounteaDialogueDecoratorAssetAction>());
		AssetActions.Add(MakeShared<FMounteaDialogueDataTableAssetAction>());
		AssetActions.Add(MakeShared<UMounteaDialogueConfigurationAssetAction>());

		for (const auto& Itr : AssetActions)
		{
			FAssetToolsModule::GetModule().Get().RegisterAssetTypeActions(Itr.ToSharedRef());
		}
	}

	// Thumbnails and Icons
	{
		DialogueTreeSet = MakeShareable(new FSlateStyleSet("DialogueTreeSet"));

		const TSharedPtr<IPlugin> PluginPtr = IPluginManager::Get().FindPlugin("MounteaDialogueSystem");

		if (PluginPtr.IsValid())
		{
			const FString ContentDir = IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir();
			
			// Dialogue specific icons
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
				//DialogueConfigIcon.png
				FSlateImageBrush* DialogueConfigSetClassThumb = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueConfigIcon"), TEXT(".png")), FVector2D(128.f, 128.f));
				FSlateImageBrush* DialogueConfigDataSetClassIcon = new FSlateImageBrush(DialogueTreeSet->RootToContentDir(TEXT("Resources/DialogueConfigIcon"), TEXT(".png")), FVector2D(16.f, 16.f));
				if (DialogueConfigSetClassThumb && DialogueConfigDataSetClassIcon)
				{
					DialogueTreeSet->Set("ClassThumbnail.MounteaDialogueConfiguration", DialogueConfigSetClassThumb);
					DialogueTreeSet->Set("ClassIcon.MounteaDialogueConfiguration", DialogueConfigDataSetClassIcon);
				}

				//Register the created style
				FSlateStyleRegistry::RegisterSlateStyle(*DialogueTreeSet.Get());
			}
		}
	}

	//Register custom Buttons for Decorators
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		{
			TArray<FOnGetDetailCustomizationInstance> CustomClassLayouts =
			{
				FOnGetDetailCustomizationInstance::CreateStatic(&FMounteaDialogueGraphNode_Details::MakeInstance),
				FOnGetDetailCustomizationInstance::CreateStatic(&FMounteaDialogueDecorator_Details::MakeInstance),
				FOnGetDetailCustomizationInstance::CreateStatic(&FMounteaDialogueGraph_Details::MakeInstance),
			};
			RegisteredCustomClassLayouts =
			{
				UMounteaDialogueGraphNode::StaticClass()->GetFName(),
				UMounteaDialogueDecoratorBase::StaticClass()->GetFName(),
				UMounteaDialogueGraph::StaticClass()->GetFName(),
			};
			for (int32 i = 0; i < RegisteredCustomClassLayouts.Num(); i++)
			{
				PropertyModule.RegisterCustomClassLayout(RegisteredCustomClassLayouts[i], CustomClassLayouts[i]);
			}
		}

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

	// Register Menu Buttons
	{
		FMDSCommands::Register();

		PluginCommands = MakeShareable(new FUICommandList);

		PluginCommands->MapAction
		(
			FMDSCommands::Get().PluginAction,
			FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::PluginButtonClicked), 
			FCanExecuteAction()
		);
		
		PluginCommands->MapAction
		(
			FMDSCommands::Get().DialoguerAction,
			FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::DialoguerButtonClicked), 
			FCanExecuteAction()
		);

		PluginCommands->MapAction(
			FMDSCommands::Get().FixMounteaNodesAction,
			FExecuteAction::CreateStatic(&FMounteaDialogueFixUtilities::ReplaceNodesInSelectedBlueprints),
			FCanExecuteAction::CreateStatic(&FMounteaDialogueFixUtilities::CanExecute)
		);
		
		IMainFrameModule& mainFrame = FModuleManager::Get().LoadModuleChecked<IMainFrameModule>("MainFrame");
		mainFrame.GetMainFrameCommandBindings()->Append(PluginCommands.ToSharedRef());

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FMounteaDialogueSystemEditor::RegisterMenus));
	}

	// Register in Window tab
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Help");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("MounteaFramework");
			Section.Label = FText::FromString(TEXT("Mountea Framework"));
						
			FToolMenuEntry Entry = Section.AddMenuEntryWithCommandList
			(
				FMDSCommands::Get().PluginAction,
				PluginCommands,
				NSLOCTEXT("MounteaSupport", "TabTitle", "Mountea Support"),
				NSLOCTEXT("MounteaSupport", "TooltipText", "Opens Mountea Framework Support channel"),
				FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Help")
			);
		}
	}

	// Load import config
	{
		const FString GameDirectory = FPaths::ProjectDir();
		const FString UpdatedConfigFile = GameDirectory + "/Config/MounteaDialogueImportConfig.ini";

		UMounteaDialogueImportConfig* ImportConfig = GetMutableDefault<UMounteaDialogueImportConfig>();

		if (FPaths::FileExists(UpdatedConfigFile))
		{
			ImportConfig->LoadConfig(nullptr, *UpdatedConfigFile);
		}
		else
		{
			ImportConfig->SaveConfig(CPF_Config, *UpdatedConfigFile);
		}
	}

	// Force GameplayTags
	{
		TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
		check(ThisPlugin.IsValid());
	
		UGameplayTagsManager::Get().AddTagIniSearchPath(ThisPlugin->GetBaseDir() / TEXT("Config") / TEXT("Tags"));
	}

	// Extend Menu
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		// Get plugin name dynamically
		const FString PluginPath = [this]() -> FString
		{
			if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem")))
			{
				return TEXT("MounteaDialogueSystem");
			}
			return FString();
		}();

		auto IsInterfaceFromPlugin = [PluginPath](const UClass* Class) -> bool
		{
			if (!Class)
			{
				return false;
			}

			// Get all implemented interfaces
			TArray<FImplementedInterface> Interfaces = Class->Interfaces;

			// Check each interface
			for (const FImplementedInterface& Interface : Interfaces)
			{
				if (Interface.Class && Interface.Class->GetPackage())
				{
					const FString InterfacePath = Interface.Class->GetPackage()->GetName();
					if (InterfacePath.Contains(PluginPath))
					{
						return true;
					}
				}
			}

			return false;
		};

		auto IsAssetFromPlugin = [PluginPath, IsInterfaceFromPlugin](const FAssetData& Asset) -> bool
		{
			// Check if asset is directly from plugin OR implements plugin interface
			const FString PackagePath = Asset.GetPackage()->GetName();
			
			// For Blueprint assets
			if (Asset.GetClass()->IsChildOf(UBlueprint::StaticClass()))
			{
				if (UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset()))
				{
					return PackagePath.Contains(PluginPath) || 
						   (Blueprint->GeneratedClass && IsInterfaceFromPlugin(Blueprint->GeneratedClass));
				}
			}
			
			// For regular assets
			return PackagePath.Contains(PluginPath) || IsInterfaceFromPlugin(Asset.GetClass());
		};

		ContentBrowserModule.GetAllAssetViewContextMenuExtenders().Add(
			FContentBrowserMenuExtender_SelectedAssets::CreateLambda([this, IsAssetFromPlugin](const TArray<FAssetData>& SelectedAssets)
			{
				bool bHasValidAsset = false;
				for (const FAssetData& Asset : SelectedAssets)
				{
					if (IsAssetFromPlugin(Asset))
					{
						bHasValidAsset = true;
						break;
					}
				}

				if (!bHasValidAsset)
				{
					return TSharedRef<FExtender>(new FExtender());
				}

				TSharedRef<FExtender> Extender = MakeShared<FExtender>();

				Extender->AddMenuExtension(
					"CommonAssetActions",
					EExtensionHook::Before,
					PluginCommands,
					FMenuExtensionDelegate::CreateLambda([SelectedAssets](FMenuBuilder& MenuBuilder)
					{
						MenuBuilder.BeginSection("MounteaActions", LOCTEXT("MounteaActionsMenuHeading", "Mountea Actions"));
						{
							MenuBuilder.AddMenuEntry(
								FMDSCommands::Get().FixMounteaNodesAction,
								NAME_None,
								LOCTEXT("MounteaAction", "Fix Mountea Nodes"),
								LOCTEXT("MounteaActionTooltip", "🔧 Replace deprecated Mountea nodes with updated substitutes.\n\n🔓 Solution is using JSON-based configuration from a public GitHub repository.\n\n💪 Supports Blueprint to C++, C++ to Blueprint, and other combinations."),
								FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust")
							);
						}
						MenuBuilder.EndSection();
					})
				);

				return Extender;
			})
		);
	}
	
	EditorLOG_WARNING(TEXT("MounteaDialogueSystemEditor module has been loaded"));
}

void FMounteaDialogueSystemEditor::ShutdownModule()
{
	// Thumbnails and Icons
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(DialogueTreeSet->GetStyleSetName());
	}

	// Style Shutdown
	{
		FMounteaDialogueGraphEditorStyle::Shutdown();
	}

	// Asset Types Cleanup
	{
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			for (const auto& Itr : AssetActions)
			{
				FAssetToolsModule::GetModule().Get().UnregisterAssetTypeActions(Itr.ToSharedRef());
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
	
	// Help Button Cleanup
	{
		UToolMenus::UnRegisterStartupCallback(this);

		UToolMenus::UnregisterOwner(this);

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

void FMounteaDialogueSystemEditor::SendHTTPGet_Tags()
{
	const UMounteaDialogueGraphEditorSettings* Settings = GetDefault<UMounteaDialogueGraphEditorSettings>();
	if (DoesHaveValidTags())
	{
		
		if (!Settings->AllowCheckTagUpdate())
		{
			return;
		}
	}
	
	const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
	
	Request->OnProcessRequestComplete().BindRaw(this, &FMounteaDialogueSystemEditor::OnGetResponse_Tags);
	Request->SetURL(Settings->GetGameplayTagsURL());

	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", "text");
	Request->ProcessRequest();
}

void FMounteaDialogueSystemEditor::LauncherButtonClicked() const
{
	const FString URL = "https://github.com/Mountea-Framework/MounteaProjectLauncher";

	if (!URL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}
}

void FMounteaDialogueSystemEditor::DialoguerButtonClicked() const
{
	const FString URL = "https://mountea-framework.github.io/MounteaDialoguer/";

	if (!URL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}
}

void FMounteaDialogueSystemEditor::WikiButtonClicked() const
{
	const FString URL = "https://github.com/Mountea-Framework/MounteaDialogueSystem/wiki/Getting-Started";

	if (!URL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}
}

void FMounteaDialogueSystemEditor::PluginButtonClicked() const
{
	const FString URL = "https://discord.gg/2vXWEEN";

	if (!URL.IsEmpty())
	{
		FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
	}
}

bool FMounteaDialogueSystemEditor::DoesHaveValidTags() const
{
	if (!GConfig) return false;
	
	const FString PluginDirectory = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"))->GetBaseDir();
	const FString ConfigFilePath = PluginDirectory + "/Config/Tags/MounteaDialogueSystemTags.ini";
	FString NormalizedConfigFilePath = FConfigCacheIni::NormalizeConfigIniPath(ConfigFilePath);

	if (FPaths::FileExists(ConfigFilePath))
	{
		FString ConfigContent;
		FConfigFile* ConfigFile = GConfig->Find(NormalizedConfigFilePath);

		return true;
	}
	
	return false;
}

void FMounteaDialogueSystemEditor::RefreshGameplayTags()
{
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"));
	check(ThisPlugin.IsValid());
	
	UGameplayTagsManager::Get().EditorRefreshGameplayTagTree();
}

void FMounteaDialogueSystemEditor::UpdateTagsConfig(const FString& NewContent)
{
	if (!GConfig) return;

	const FString PluginDirectory = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"))->GetBaseDir();
	const FString ConfigFilePath = PluginDirectory + "/Config/Tags/MounteaDialogueSystemTags.ini";
	FString NormalizedConfigFilePath = FConfigCacheIni::NormalizeConfigIniPath(ConfigFilePath);

	FConfigFile* CurrentConfig = GConfig->Find(NormalizedConfigFilePath);

	FString CurrentContent;
	CurrentConfig->WriteToString(CurrentContent);

	TArray<FString> Lines;
	NewContent.ParseIntoArray(Lines, TEXT("\n"), true);

	TArray<FString> CleanedLines;
	for (FString& Itr : Lines)
	{
		if (Itr.Equals("[/Script/GameplayTags.GameplayTagsList]")) continue;

		if (Itr.Contains("GameplayTagList="))
		{
			FString NewValue = Itr.Replace(TEXT("GameplayTagList="), TEXT(""));

			CleanedLines.Add(NewValue);
		}
	}

	if (!CurrentContent.Equals(NewContent))
	{
		TArray<FString> CurrentLines;
		FConfigFile NewConfig;
		NewConfig.SetArray(TEXT("/Script/GameplayTags.GameplayTagsList"), TEXT("GameplayTagList"), CleanedLines);
		CurrentConfig->GetArray(TEXT("/Script/GameplayTags.GameplayTagsList"), TEXT("GameplayTagList"), CurrentLines);

		for (const FString& Itr : CleanedLines)
		{
			if (CurrentLines.Contains(Itr)) continue;

			CurrentLines.AddUnique(Itr);
		}

		CurrentConfig->SetArray(TEXT("/Script/GameplayTags.GameplayTagsList"), TEXT("GameplayTagList"), CurrentLines);
		CurrentConfig->Write(ConfigFilePath);

		RefreshGameplayTags();
	}
}

void FMounteaDialogueSystemEditor::CreateTagsConfig(const FString& NewContent)
{
	if (!GConfig) return;

	const FString PluginDirectory = IPluginManager::Get().FindPlugin(TEXT("MounteaDialogueSystem"))->GetBaseDir();
	const FString ConfigFilePath = PluginDirectory + "/Config/Tags/MounteaDialogueSystemTags.ini";

	TArray<FString> Lines;
	NewContent.ParseIntoArray(Lines, TEXT("\n"), true);

	TArray<FString> CleanedLines;
	for (FString& Itr : Lines)
	{
		if (Itr.Equals("[/Script/GameplayTags.GameplayTagsList]")) continue;

		if (Itr.Contains("GameplayTagList="))
		{
			FString NewValue = Itr.Replace(TEXT("GameplayTagList="), TEXT(""));

			CleanedLines.Add(NewValue);
		}
	}
	
	FConfigFile NewConfig;
	NewConfig.SetArray(TEXT("/Script/GameplayTags.GameplayTagsList"), TEXT("GameplayTagList"), CleanedLines);
	NewConfig.Write(ConfigFilePath);
}

void FMounteaDialogueSystemEditor::OnGetResponse_Tags(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString ResponseBody;
	if (Response.Get() == nullptr) return;
	
	if (Response.IsValid() && Response->GetResponseCode() == 200)
	{
		ResponseBody = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	}
	
	if (!DoesHaveValidTags())
	{
		CreateTagsConfig(ResponseBody);
	}
	else
	{
		UpdateTagsConfig(ResponseBody);
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
				FToolMenuSection& Section = Menu->FindOrAddSection("MounteaFramework");
				
				Section.InsertPosition.Position = EToolMenuInsertType::First;
				Section.Label = FText::FromString(TEXT("Mountea Framework"));
				
				FToolMenuEntry SupportEntry = Section.AddMenuEntryWithCommandList
				(
					FMDSCommands::Get().PluginAction,
					PluginCommands,
					LOCTEXT("MounteaSystemEditor_SupportButton_Label", "Mountea Support"),
					LOCTEXT("MounteaSystemEditor_SupportButton_ToolTip", "🆘 Open Mountea Framework Support channel"),
					FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Help")
				);
				SupportEntry.Name = FName("MounteaFrameworkSupport");
			}
		}
	}

	// Register in Level Editor Toolbar
	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu(MenuName);
		{
			FToolMenuSection& Entry = ToolbarMenu->FindOrAddSection("MounteaDialogueFramework");
			FToolMenuEntry& MenuEntry = Entry.AddEntry(FToolMenuEntry::InitComboButton(
				"MounteaMenu",
				FUIAction(),
				FOnGetContent::CreateRaw(this, &FMounteaDialogueSystemEditor::MakeMounteaMenuWidget),
				LOCTEXT("MounteaMainMenu_Label", "Mountea Dialogue Menu"),
				LOCTEXT("MounteaMainMenu_Tooltip", "📂 Open Mountea Dialogue menu.\n\n❔ Provides link to Documentation, Support Discord and Dialogue tool."),
				FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.DialogueSystemIcon"),
				false,
				"MounteaMenu"
			));
				
			MenuEntry.Label = LOCTEXT("MounteaMainMenu_Label", "Mountea Dialogue Menu");
			MenuEntry.Name = TEXT("MounteaMenu");
			MenuEntry.StyleNameOverride = "CalloutToolbar";
			MenuEntry.SetCommandList(PluginCommands);
		}
	}
}

TSharedRef<SWidget> FMounteaDialogueSystemEditor::MakeMounteaMenuWidget() const
{
	FMenuBuilder MenuBuilder(true, PluginCommands);

	MenuBuilder.BeginSection("MounteaMenu_Links", LOCTEXT("MounteaMenuOptions_Options", "Mountea Links"));
	{
		// Support Entry
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MounteaSystemEditor_SupportButton_Label", "Mountea Support"),
			LOCTEXT("MounteaSystemEditor_SupportButton_ToolTip", "🆘 Open Mountea Framework Support channel"),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Help"),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::PluginButtonClicked)
			)
		);
		// Wiki Entry
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MounteaSystemEditor_WikiButton_Label", "Mountea Dialogue Wiki"),
			LOCTEXT("MounteaSystemEditor_WikiButton_ToolTip", "📖 Open Mountea Dialogue Documentation"),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Wiki"),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::WikiButtonClicked)
			)
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("MounteaMenu_Tools", LOCTEXT("MounteaMenuOptions_Tools", "Mountea Tools"));
	{
		// Dialoguer Entry
		MenuBuilder.AddMenuEntry(
			LOCTEXT("MounteaSystemEditor_DialoguerButton_Label", "Mountea Dialoguer"),
			LOCTEXT("MounteaSystemEditor_DialoguerButton_ToolTip", "⛰ Open Mountea Dialoguer Standalone Tool\n\n❔ Mountea Dialoguer is a standalone tool created for Dialogue crafting. Mountea Dialogue System supports native import for `.mnteadlg` files."),
			FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Dialoguer"),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::DialoguerButtonClicked)
			)
		);
	}
	
	// Launcher Tool Entry
	MenuBuilder.AddMenuEntry(
		LOCTEXT("MounteaSystemEditor_LauncherButton_Label", "Mountea Project Launcher"),
		LOCTEXT("MounteaSystemEditor_LauncherButton_ToolTip", "🚀 Open Mountea Project Launcher\n\n❔ Mountea Project Launcher is a standalone tool created for project launcher which can launch projects locally with multiple settings.\nUseful for testing."),
		FSlateIcon(FMounteaDialogueGraphEditorStyle::GetAppStyleSetName(), "MDSStyleSet.Launcher"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FMounteaDialogueSystemEditor::LauncherButtonClicked)
		)
	);
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMounteaDialogueSystemEditor, MounteaDialogueSystemEditor)