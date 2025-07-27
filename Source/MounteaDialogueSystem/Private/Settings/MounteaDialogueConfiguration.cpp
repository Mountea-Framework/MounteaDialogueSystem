// All rights reserved Dominik Morse (Pavlicek) 2024


#include "Settings/MounteaDialogueConfiguration.h"

#include "Engine/Font.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"

UMounteaDialogueConfiguration::UMounteaDialogueConfiguration() :
	InputMode(EMounteaInputMode::EIM_UIAndGame),
	bAllowSubtitles(true),
	bSkipRowWithAudioSkip(false)
{
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
