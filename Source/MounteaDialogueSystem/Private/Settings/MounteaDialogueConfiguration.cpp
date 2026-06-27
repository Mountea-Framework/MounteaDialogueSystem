// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Settings/MounteaDialogueConfiguration.h"

#include "Engine/Font.h"
#include "NativeGameplayTags.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNode.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

namespace MounteaDialogueConfigurationTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_MDS_GraphTypeNamespace, "Mountea_Dialogue.Graph.Type");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_MDS_GraphTypeDialogue, "Mountea_Dialogue.Graph.Type.Dialogue");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_MDS_GraphTypeMonologue, "Mountea_Dialogue.Graph.Type.Monologue");
}

UMounteaDialogueConfiguration::UMounteaDialogueConfiguration() :
	InputMode(EMounteaInputMode::EIM_UIAndGame),
	bAllowSubtitles(true),
	bSkipRowWithAudioSkip(false)
{
	GraphTypeNamespaceRootTag = MounteaDialogueConfigurationTags::TAG_MDS_GraphTypeNamespace;
	DefaultGraphTypeId = FName(TEXT("Dialogue"));
	GraphTypeDefinitions.Empty();
	{
		FMounteaDialogueGraphTypeDefinition dialogueType;
		dialogueType.TypeId = FName(TEXT("Dialogue"));
		dialogueType.RootTag = MounteaDialogueConfigurationTags::TAG_MDS_GraphTypeDialogue;
		GraphTypeDefinitions.Add(dialogueType);
	}
	{
		FMounteaDialogueGraphTypeDefinition monologueType;
		monologueType.TypeId = FName(TEXT("Monologue"));
		monologueType.RootTag = MounteaDialogueConfigurationTags::TAG_MDS_GraphTypeMonologue;
		GraphTypeDefinitions.Add(monologueType);
	}

#if WITH_EDITOR
	if (SubtitlesSettings.SettingsGUID.IsValid() == false)
		SubtitlesSettings.SettingsGUID = FGuid::NewGuid();
	if (SubtitlesSettings.SubtitlesFont.FontObject == nullptr)
		SubtitlesSettings.SubtitlesFont = SetupDefaultFontSettings();
#endif

	{
		FMounteaDialogueGraphNodeConfig leadNodeConfig;
		leadNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());
		leadNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
		leadNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());
		NodesConfiguration.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass(), leadNodeConfig);
	}
	{
		FMounteaDialogueGraphNodeConfig answerNodeConfig;
		answerNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass());
		answerNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_StartNode::StaticClass());
		answerNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());
		NodesConfiguration.Add(UMounteaDialogueGraphNode_LeadNode::StaticClass(), answerNodeConfig);
	}
	{
		FMounteaDialogueGraphNodeConfig completeNodeConfig;
		completeNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_DialogueNodeBase::StaticClass());
		NodesConfiguration.Add(UMounteaDialogueGraphNode_CompleteNode::StaticClass(), completeNodeConfig);
	}
	{
		FMounteaDialogueGraphNodeConfig delayNodeConfig;
		delayNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());
		NodesConfiguration.Add(UMounteaDialogueGraphNode_Delay::StaticClass(), delayNodeConfig);
	}
	{
		FMounteaDialogueGraphNodeConfig dialogueBaseNodeConfig;
		dialogueBaseNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode_Delay::StaticClass());
		NodesConfiguration.Add(UMounteaDialogueGraphNode_DialogueNodeBase::StaticClass(), dialogueBaseNodeConfig);
	}
	{
		FMounteaDialogueGraphNodeConfig openChildGraphNodeConfig;
		openChildGraphNodeConfig.AllowedInputClasses.Add(UMounteaDialogueGraphNode::StaticClass());
		NodesConfiguration.Add(UMounteaDialogueGraphNode_OpenChildGraph::StaticClass(), openChildGraphNodeConfig);
	}
	
	MonologueWhitelistedNodes.Add(UMounteaDialogueGraphNode_AnswerNode::StaticClass());
	MonologueWhitelistedNodes.Add(UMounteaDialogueGraphNode_CompleteNode::StaticClass());
	MonologueWhitelistedNodes.Add(UMounteaDialogueGraphNode_Delay::StaticClass());
	MonologueWhitelistedNodes.Add(UMounteaDialogueGraphNode_OpenChildGraph::StaticClass());
	MonologueWhitelistedNodes.Add(UMounteaDialogueGraphNode_DialogueNode::StaticClass());
}

bool UMounteaDialogueConfiguration::ResolveGraphTypeFromTags(
	const FGameplayTagContainer& InGraphTags,
	FName& OutResolvedTypeId,
	FString& OutFailureReason) const
{
	OutResolvedTypeId = NAME_None;
	OutFailureReason.Empty();

	if (!GraphTypeNamespaceRootTag.IsValid())
	{
		OutFailureReason = TEXT("GraphTypeNamespaceRootTag is invalid.");
		return false;
	}

	if (GraphTypeDefinitions.Num() == 0)
	{
		OutFailureReason = TEXT("GraphTypeDefinitions are empty.");
		return false;
	}

	if (DefaultGraphTypeId.IsNone())
	{
		OutFailureReason = TEXT("DefaultGraphTypeId is invalid.");
		return false;
	}

	TSet<FName> seenTypeIds;
	TMap<FName, FGameplayTag> resolvedDefinitions;
	for (const FMounteaDialogueGraphTypeDefinition& definition : GraphTypeDefinitions)
	{
		if (definition.TypeId.IsNone() || !definition.RootTag.IsValid())
		{
			OutFailureReason = TEXT("GraphTypeDefinitions contain invalid entries (missing TypeId or RootTag).");
			return false;
		}

		if (seenTypeIds.Contains(definition.TypeId))
		{
			OutFailureReason = FString::Printf(TEXT("GraphTypeDefinitions contain duplicate TypeId '%s'."), *definition.TypeId.ToString());
			return false;
		}

		seenTypeIds.Add(definition.TypeId);
		resolvedDefinitions.Add(definition.TypeId, definition.RootTag);
	}

	if (!resolvedDefinitions.Contains(DefaultGraphTypeId))
	{
		OutFailureReason = FString::Printf(
			TEXT("DefaultGraphTypeId '%s' is not present in GraphTypeDefinitions."),
			*DefaultGraphTypeId.ToString());
		return false;
	}

	TArray<FGameplayTag> graphTagsArray;
	InGraphTags.GetGameplayTagArray(graphTagsArray);

	TArray<FGameplayTag> graphTypeTags;
	for (const FGameplayTag& graphTag : graphTagsArray)
	{
		if (graphTag.IsValid() && graphTag.MatchesTag(GraphTypeNamespaceRootTag))
			graphTypeTags.AddUnique(graphTag);
	}

	if (graphTypeTags.Num() == 0)
	{
		OutResolvedTypeId = DefaultGraphTypeId;
		return true;
	}

	TArray<FString> unknownTypeTags;
	for (const FGameplayTag& graphTypeTag : graphTypeTags)
	{
		bool bKnownTypeTag = false;
		for (const auto& definitionKvp : resolvedDefinitions)
		{
			if (graphTypeTag.MatchesTag(definitionKvp.Value))
			{
				bKnownTypeTag = true;
				break;
			}
		}

		if (!bKnownTypeTag)
			unknownTypeTags.AddUnique(graphTypeTag.ToString());
	}

	if (unknownTypeTags.Num() > 0)
	{
		const FString unknownTags = FString::Join(unknownTypeTags, TEXT(", "));
		OutFailureReason = FString::Printf(
			TEXT("Unknown graph type tags detected: %s"),
			*unknownTags);
		return false;
	}

	TArray<FName> matchedTypeIds;
	for (const auto& definitionKvp : resolvedDefinitions)
	{
		for (const FGameplayTag& graphTypeTag : graphTypeTags)
		{
			if (graphTypeTag.MatchesTag(definitionKvp.Value))
			{
				matchedTypeIds.AddUnique(definitionKvp.Key);
				break;
			}
		}
	}

	if (matchedTypeIds.Num() == 0)
	{
		OutFailureReason = TEXT("Graph type tags were provided but none matched configured graph type definitions.");
		return false;
	}

	if (matchedTypeIds.Num() > 1)
	{
		TArray<FString> matchedTypeNames;
		for (const FName& typeId : matchedTypeIds)
			matchedTypeNames.Add(typeId.ToString());

		const FString ambiguousTypes = FString::Join(matchedTypeNames, TEXT(", "));
		OutFailureReason = FString::Printf(
			TEXT("Ambiguous graph type tags detected. Matched multiple graph types: %s"),
			*ambiguousTypes);
		return false;
	}

	OutResolvedTypeId = matchedTypeIds[0];
	return true;
}

#if WITH_EDITOR
FSlateFontInfo UMounteaDialogueConfiguration::SetupDefaultFontSettings()
{
	FSlateFontInfo ReturnFontInfo;
	ReturnFontInfo.FontObject = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto"));

	ReturnFontInfo.TypefaceFontName = FName("Regular");
	ReturnFontInfo.Size = 16;
	ReturnFontInfo.OutlineSettings = FFontOutlineSettings(1);

	return ReturnFontInfo;
}

void UMounteaDialogueConfiguration::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	if (SubtitlesSettings.SettingsGUID.IsValid() == false)
		SubtitlesSettings.SettingsGUID = FGuid::NewGuid();

	if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueConfiguration, SubtitlesSettings))
	{
		if (SubtitlesSettings.SubtitlesFont.FontObject == nullptr)
			SubtitlesSettings.SubtitlesFont = SetupDefaultFontSettings();

	}

	if (PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName() == GET_MEMBER_NAME_CHECKED(UMounteaDialogueConfiguration, SubtitlesSettingsOverrides))
	{
		for (auto& Itr : SubtitlesSettingsOverrides)
		{
			if (Itr.Value.SettingsGUID.IsValid() == false)
				Itr.Value.SettingsGUID = FGuid::NewGuid();
			
			if (Itr.Value.SubtitlesFont.FontObject == nullptr)
				Itr.Value.SubtitlesFont = SetupDefaultFontSettings();
		}
	}
}
#endif
