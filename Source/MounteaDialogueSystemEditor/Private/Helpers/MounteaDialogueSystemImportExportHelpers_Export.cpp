// Copyright (C) 2026 Dominik (Pavlicek) Morse. All rights reserved.
//
// Developed for the Mountea Framework as a free tool. This solution is provided
// for use and sharing without charge. Redistribution is allowed under the following conditions:
//
// - You may use this solution in commercial products, provided the product is not
//   this solution itself (or unless significant modifications have been made to the solution).
// - You may not resell or redistribute the original, unmodified solution.
//
// For more information, visit: https://mountea.tools

// Export half of the archive pipeline: graphs -> mountea.archive.v3 model -> .mnteadlg / .mnteadlgproj.
// The format itself (validation, record layout, zip) lives in Archive/MounteaDialogueArchive*.

#include "MounteaDialogueSystemImportExportHelpers.h"

#include "Archive/MounteaDialogueArchive.h"
#include "Archive/MounteaDialogueArchiveZip.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Conditions/MounteaDialogueConditionBase.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Decorators/MounteaDialogueDecoratorBase.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Edges/MounteaDialogueGraphEdge.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Graph/MounteaDialogueGraph.h"
#include "Helpers/MounteaDialogueGraphEditorHelpers.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "ImportConfig/MounteaDialogueImportConfig.h"
#include "Internationalization/StringTable.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Nodes/MounteaDialogueGraphNode_AnswerNode.h"
#include "Nodes/MounteaDialogueGraphNode_CompleteNode.h"
#include "Nodes/MounteaDialogueGraphNode_Delay.h"
#include "Nodes/MounteaDialogueGraphNode_DialogueNodeBase.h"
#include "Nodes/MounteaDialogueGraphNode_LeadNode.h"
#include "Nodes/MounteaDialogueGraphNode_OpenChildGraph.h"
#include "Nodes/MounteaDialogueGraphNode_ReturnToNode.h"
#include "Nodes/MounteaDialogueGraphNode_StartNode.h"
#include "Sound/SoundWave.h"
#include "UObject/UnrealType.h"

namespace
{
	using namespace MounteaArchive;

	constexpr const TCHAR* DefaultExportLocale = TEXT("en");
	constexpr const TCHAR* TagRoot = TEXT("Mountea_Dialogue.");

	/** lowercase [a-z0-9_] token, at most MaxLength characters. */
	FString Slugify(const FString& Raw, const int32 MaxLength)
	{
		FString result;
		bool bLastUnderscore = true;
		for (const TCHAR character : Raw)
		{
			const bool bAlnum = (character >= TEXT('0') && character <= TEXT('9')) || (character >= TEXT('a') && character <= TEXT('z')) || (character >= TEXT('A') && character <= TEXT('Z'));
			if (bAlnum)
			{
				result.AppendChar(FChar::ToLower(character));
				bLastUnderscore = false;
			}
			else if (!bLastUnderscore)
			{
				result.AppendChar(TEXT('_'));
				bLastUnderscore = true;
			}
			if (result.Len() >= MaxLength)
				break;
		}
		result.TrimCharInline(TEXT('_'), nullptr);
		return result;
	}

	FString ShortHash(const FGuid& Guid)
	{
		return FString::Printf(TEXT("%04x"), GetTypeHash(Guid) & 0xFFFF);
	}

	FString UniqueToken(const FString& Base, const FString& Fallback, const FGuid& Guid, TSet<FString>& Used)
	{
		FString stem = Slugify(Base, 24);
		if (stem.IsEmpty())
			stem = Fallback;

		FString token = stem + TEXT("_") + ShortHash(Guid);
		int32 counter = 2;
		while (Used.Contains(token))
			token = FString::Printf(TEXT("%s_%s_%d"), *stem, *ShortHash(Guid), counter++);
		Used.Add(token);
		return token;
	}

	/** Stable id for things that have no GUID of their own (categories, participants, native classes). */
	FString DerivedId(const FString& Seed)
	{
		return GuidToId(IdToGuid(Seed));
	}

	/** Reads <Project>/Content/Localization/<Table>/<locale>/<Table>.po into locale -> (key -> text). */
	TMap<FString, TMap<FString, FString>> ReadPoTranslations(const FString& StringTableId)
	{
		TMap<FString, TMap<FString, FString>> result;

		const FString root = FPaths::Combine(FPaths::ProjectDir(), TEXT("Content"), TEXT("Localization"), StringTableId);
		TArray<FString> localeDirs;
		IFileManager::Get().FindFiles(localeDirs, *(root / TEXT("*")), false, true);

		// Inverse of the PO escaping (PortableObjectPipeline::ConditionPOStringForArchive).
		auto unescape = [](const FString& Input) -> FString
		{
			FString output = Input;
			output.ReplaceInline(TEXT("\\t"), TEXT("\t"), ESearchCase::CaseSensitive);
			output.ReplaceInline(TEXT("\\n"), TEXT("\n"), ESearchCase::CaseSensitive);
			output.ReplaceInline(TEXT("\\r"), TEXT("\r"), ESearchCase::CaseSensitive);
			output.ReplaceInline(TEXT("\\\""), TEXT("\""), ESearchCase::CaseSensitive);
			output.ReplaceInline(TEXT("\\\\"), TEXT("\\"), ESearchCase::CaseSensitive);
			return output;
		};

		// "<Namespace>,<Key>" with escaped commas; the older "<Table>|<Key>" form is still accepted.
		auto keyOfIdentity = [&unescape](const FString& Identity) -> FString
		{
			const FString plain = unescape(Identity);
			bool bEscaped = false;
			for (int32 index = 0; index < plain.Len(); ++index)
			{
				if (bEscaped)
				{
					bEscaped = false;
					continue;
				}
				if (plain[index] == TEXT('\\'))
					bEscaped = true;
				else if (plain[index] == TEXT(','))
				{
					FString key = plain.Mid(index + 1);
					key.ReplaceInline(TEXT("\\,"), TEXT(","), ESearchCase::CaseSensitive);
					return key;
				}
			}

			int32 separator = INDEX_NONE;
			return plain.FindChar(TEXT('|'), separator) ? plain.Mid(separator + 1) : FString();
		};

		for (const FString& locale : localeDirs)
		{
			FString content;
			if (!FFileHelper::LoadFileToString(content, *(root / locale / (StringTableId + TEXT(".po")))))
				continue;

			TArray<FString> lines;
			content.ParseIntoArrayLines(lines);

			FString currentKey;
			for (const FString& line : lines)
			{
				if (line.StartsWith(TEXT("msgctxt \"")))
				{
					FString context = line.Mid(9);
					context.RemoveFromEnd(TEXT("\""));
					currentKey = keyOfIdentity(context);
				}
				else if (line.StartsWith(TEXT("msgstr \"")) && !currentKey.IsEmpty())
				{
					FString text = line.Mid(8);
					text.RemoveFromEnd(TEXT("\""));
					result.FindOrAdd(locale).Add(currentKey, unescape(text));
					currentKey.Reset();
				}
			}
		}
		return result;
	}

	/** Unreal property -> contract property type; empty when the property cannot be expressed. */
	FString ContractTypeOf(const FProperty* Property)
	{
		if (CastField<FBoolProperty>(Property))
			return Contract::PropertyBoolean;
		if (CastField<FNumericProperty>(Property) && !CastField<FEnumProperty>(Property))
			return Contract::PropertyNumber;
		if (CastField<FStrProperty>(Property) || CastField<FNameProperty>(Property) || CastField<FTextProperty>(Property))
			return Contract::PropertyString;
		return FString();
	}

	/** Editable properties a Blueprint/C++ subclass adds on top of Base. */
	TArray<const FProperty*> OwnProperties(const UClass* Class, const UClass* Base)
	{
		TArray<const FProperty*> properties;
		for (TFieldIterator<FProperty> it(Class, EFieldIteratorFlags::IncludeSuper); it; ++it)
		{
			const UClass* owner = it->GetOwnerClass();
			if (!owner || owner == Base || Base->IsChildOf(owner))
				continue;
			if (!it->HasAnyPropertyFlags(CPF_Edit) || ContractTypeOf(*it).IsEmpty())
				continue;
			properties.Add(*it);
		}
		return properties;
	}

	TSharedPtr<FJsonObject> PropertyValues(const UObject* Instance, const TArray<const FProperty*>& Properties)
	{
		const TSharedRef<FJsonObject> values = MakeShared<FJsonObject>();
		for (const FProperty* property : Properties)
		{
			FString text;
			property->ExportText_InContainer(0, text, Instance, Instance, nullptr, PPF_None);

			const FString type = ContractTypeOf(property);
			if (type == Contract::PropertyBoolean)
				values->SetBoolField(property->GetName(), text.Equals(TEXT("true"), ESearchCase::IgnoreCase));
			else if (type == Contract::PropertyNumber)
				values->SetNumberField(property->GetName(), FCString::Atod(*text));
			else
				values->SetStringField(property->GetName(), text);
		}
		return values;
	}

	FString ClassDisplayName(const UClass* Class)
	{
		FString name = Class->GetName();
		name.RemoveFromEnd(TEXT("_C"));
		return name;
	}

	bool SoundToWav(USoundWave* Sound, TArray<uint8>& OutWav)
	{
		if (!Sound)
			return false;

		TArray<uint8> pcm;
		uint32 sampleRate = 0;
		uint16 channels = 0;
		if (!Sound->GetImportedSoundWaveData(pcm, sampleRate, channels))
			return false;

		struct FWavHeader
		{
			uint8 ChunkId[4] = { 'R', 'I', 'F', 'F' };
			uint32 ChunkSize = 0;
			uint8 Format[4] = { 'W', 'A', 'V', 'E' };
			uint8 Subchunk1Id[4] = { 'f', 'm', 't', ' ' };
			uint32 Subchunk1Size = 16;
			uint16 AudioFormat = 1;
			uint16 NumChannels = 0;
			uint32 SampleRate = 0;
			uint32 ByteRate = 0;
			uint16 BlockAlign = 0;
			uint16 BitsPerSample = 16;
			uint8 Subchunk2Id[4] = { 'd', 'a', 't', 'a' };
			uint32 Subchunk2Size = 0;
		};

		FWavHeader header;
		header.NumChannels = channels;
		header.SampleRate = sampleRate;
		header.ByteRate = sampleRate * channels * 2;
		header.BlockAlign = channels * 2;
		header.Subchunk2Size = pcm.Num();
		header.ChunkSize = 36 + header.Subchunk2Size;

		OutWav.SetNum(sizeof(FWavHeader) + pcm.Num());
		FMemory::Memcpy(OutWav.GetData(), &header, sizeof(FWavHeader));
		FMemory::Memcpy(OutWav.GetData() + sizeof(FWavHeader), pcm.GetData(), pcm.Num());
		return true;
	}

	bool TextureToPng(UTexture2D* Texture, FMedia& OutMedia)
	{
		if (!Texture)
			return false;

		const FTextureSource& source = Texture->Source;
		if (!source.IsValid())
			return false;

		TArray64<uint8> sourceData;
		if (!const_cast<FTextureSource&>(source).GetMipData(sourceData, 0))
			return false;

		ERGBFormat rgbFormat = ERGBFormat::BGRA;
		int32 bitDepth = 8;
		switch (source.GetFormat())
		{
			case TSF_RGBA8_DEPRECATED: rgbFormat = ERGBFormat::RGBA; bitDepth = 8; break;
			case TSF_RGBA16: rgbFormat = ERGBFormat::RGBA; bitDepth = 16; break;
			case TSF_RGBA16F: rgbFormat = ERGBFormat::RGBA; bitDepth = 16; break;
			case TSF_G8: rgbFormat = ERGBFormat::Gray; bitDepth = 8; break;
			case TSF_G16: rgbFormat = ERGBFormat::Gray; bitDepth = 16; break;
			default: rgbFormat = ERGBFormat::BGRA; bitDepth = 8; break;
		}

		IImageWrapperModule& imageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
		const TSharedPtr<IImageWrapper> wrapper = imageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		wrapper->SetRaw(sourceData.GetData(), sourceData.Num(), source.GetSizeX(), source.GetSizeY(), rgbFormat, bitDepth);
		const TArray64<uint8>& png = wrapper->GetCompressed();
		if (png.IsEmpty())
			return false;

		OutMedia.Name = Texture->GetName() + TEXT(".png");
		OutMedia.MimeType = TEXT("image/png");
		OutMedia.Bytes.Append(png.GetData(), static_cast<int32>(png.Num()));
		OutMedia.Width = source.GetSizeX();
		OutMedia.Height = source.GetSizeY();
		return true;
	}

	/** Everything the export needs to remember while walking the graphs. */
	struct FExportBuilder
	{
		FDialogueArchive& Archive;
		TArray<FString>& Errors;

		TMap<const UMounteaDialogueGraph*, FString> DialogueIds;
		TSet<FString> UsedSlugs;
		TMap<FString, FString> CategoryIdsByPath;
		TMap<FString, FString> ParticipantIdsByTag;
		TMap<FString, FString> DecoratorIdsByClass;
		TMap<FString, FString> ConditionIdsByClass;
		TSet<FString> Locales;

		FExportBuilder(FDialogueArchive& InArchive, TArray<FString>& InErrors) : Archive(InArchive), Errors(InErrors) {}
	};

	FString NodeIdOf(const UMounteaDialogueGraphNode* Node)
	{
		if (!Node)
			return FString();
		return Node->IsA<UMounteaDialogueGraphNode_StartNode>() ? FString(Contract::StartNodeId) : GuidToId(Node->GetNodeGUID());
	}

	const TCHAR* TypeOf(const UMounteaDialogueGraphNode* Node)
	{
		if (Node->IsA<UMounteaDialogueGraphNode_StartNode>()) return Contract::NodeStart;
		if (Node->IsA<UMounteaDialogueGraphNode_LeadNode>()) return Contract::NodeLead;
		if (Node->IsA<UMounteaDialogueGraphNode_AnswerNode>()) return Contract::NodeAnswer;
		if (Node->IsA<UMounteaDialogueGraphNode_CompleteNode>()) return Contract::NodeComplete;
		if (Node->IsA<UMounteaDialogueGraphNode_ReturnToNode>()) return Contract::NodeReturn;
		if (Node->IsA<UMounteaDialogueGraphNode_OpenChildGraph>()) return Contract::NodeOpenChildGraph;
		if (Node->IsA<UMounteaDialogueGraphNode_Delay>()) return Contract::NodeDelay;
		return nullptr;
	}

	FString EnsureDecoratorDefinition(FExportBuilder& Builder, const UMounteaDialogueDecoratorBase* Instance)
	{
		const UClass* decoratorClass = Instance->GetClass();
		if (const FString* known = Builder.DecoratorIdsByClass.Find(decoratorClass->GetPathName()))
			return *known;

		const UMounteaDialogueDecoratorBase* cdo = decoratorClass->GetDefaultObject<UMounteaDialogueDecoratorBase>();
		const FGuid definitionGuid = cdo && cdo->GetDecoratorGUID().IsValid() ? cdo->GetDecoratorGUID() : IdToGuid(TEXT("decorator:") + decoratorClass->GetPathName());

		FDefinition definition;
		definition.Id = GuidToId(definitionGuid);
		definition.Name = cdo && !cdo->GetDecoratorName().IsEmpty() ? cdo->GetDecoratorName().ToString() : ClassDisplayName(decoratorClass);
		definition.Type = TEXT("Decorator");
		for (const FProperty* property : OwnProperties(decoratorClass, UMounteaDialogueDecoratorBase::StaticClass()))
		{
			FPropertyDef def;
			def.Name = property->GetName();
			def.Type = ContractTypeOf(property);
			FString text;
			property->ExportText_InContainer(0, text, cdo, cdo, nullptr, PPF_None);
			def.DefaultValue = text;
			definition.Properties.Add(def);
		}

		Builder.DecoratorIdsByClass.Add(decoratorClass->GetPathName(), definition.Id);
		Builder.Archive.Decorators.Add(MoveTemp(definition));
		return Builder.DecoratorIdsByClass[decoratorClass->GetPathName()];
	}

	FString EnsureConditionDefinition(FExportBuilder& Builder, const UMounteaDialogueConditionBase* Instance)
	{
		const UClass* conditionClass = Instance->GetClass();
		if (const FString* known = Builder.ConditionIdsByClass.Find(conditionClass->GetPathName()))
			return *known;

		// An imported instance keeps its definition GUID; otherwise the class CDO or a path-derived id is used.
		const UMounteaDialogueConditionBase* cdo = conditionClass->GetDefaultObject<UMounteaDialogueConditionBase>();
		FGuid definitionGuid = Instance->GetConditionGUID();
		if (!definitionGuid.IsValid() && cdo)
			definitionGuid = cdo->GetConditionGUID();
		if (!definitionGuid.IsValid())
			definitionGuid = IdToGuid(TEXT("condition:") + conditionClass->GetPathName());

		FDefinition definition;
		definition.Id = GuidToId(definitionGuid);
		definition.Name = !Instance->ConditionName.IsNone() ? Instance->ConditionName.ToString() : ClassDisplayName(conditionClass);
		for (const FProperty* property : OwnProperties(conditionClass, UMounteaDialogueConditionBase::StaticClass()))
		{
			FPropertyDef def;
			def.Name = property->GetName();
			def.Type = ContractTypeOf(property);
			FString text;
			property->ExportText_InContainer(0, text, cdo, cdo, nullptr, PPF_None);
			def.DefaultValue = text;
			definition.Properties.Add(def);
		}

		// Two classes can share a definition GUID (re-created Blueprints): keep the first definition only.
		const bool bAlreadyDefined = Builder.Archive.FindCondition(definition.Id) != nullptr;
		Builder.ConditionIdsByClass.Add(conditionClass->GetPathName(), definition.Id);
		if (!bAlreadyDefined)
			Builder.Archive.Conditions.Add(MoveTemp(definition));
		return Builder.ConditionIdsByClass[conditionClass->GetPathName()];
	}

	/** Participant (and category chain) for a name/tag pair; created on first use. */
	FString EnsureParticipant(FExportBuilder& Builder, const FString& ParticipantName, const FGameplayTag& Tag, const FDialogueParticipant* TableRow)
	{
		const FString tagText = Tag.IsValid() ? Tag.ToString() : FString();
		const FString participantKey = tagText.IsEmpty() ? (TEXT("name:") + ParticipantName) : tagText;
		if (const FString* known = Builder.ParticipantIdsByTag.Find(participantKey))
			return *known;

		TArray<FString> chain;
		FString path = tagText;
		path.RemoveFromStart(TagRoot);
		path.ParseIntoArray(chain, TEXT("."), true);
		if (!chain.IsEmpty())
			chain.Pop(); // the last segment is the participant itself
		if (chain.IsEmpty())
			chain.Add(TEXT("General"));

		FString parentId;
		FString chainPath;
		for (const FString& segment : chain)
		{
			chainPath += (chainPath.IsEmpty() ? TEXT("") : TEXT(".")) + segment;
			const FString* existing = Builder.CategoryIdsByPath.Find(chainPath);
			if (!existing)
			{
				FCategory category;
				category.Id = DerivedId(TEXT("category:") + chainPath);
				category.Name = segment;
				category.ParentId = parentId;
				Builder.CategoryIdsByPath.Add(chainPath, category.Id);
				Builder.Archive.Categories.Add(category);
				existing = &Builder.CategoryIdsByPath[chainPath];
			}
			parentId = *existing;
		}

		FParticipant participant;
		participant.Id = DerivedId(TEXT("participant:") + participantKey);
		participant.Name = ParticipantName;
		participant.CategoryId = parentId;
		if (TableRow && !TableRow->ParticipantIcon.IsNull())
		{
			if (TextureToPng(TableRow->ParticipantIcon.LoadSynchronous(), participant.Thumbnail))
				participant.bHasThumbnail = true;
		}

		Builder.ParticipantIdsByTag.Add(participantKey, participant.Id);
		Builder.Archive.Participants.Add(MoveTemp(participant));
		return Builder.ParticipantIdsByTag[participantKey];
	}

	/** Text of an FText plus its translations (from the .po files of its string table, when it came from one). */
	void FillLocalizedValues(FLocalizedString& Entry, const FText& Text, const TMap<FString, TMap<FString, TMap<FString, FString>>>& PoByTable, FExportBuilder& Builder)
	{
		Entry.Values.Add(DefaultExportLocale, Text.ToString());
		Builder.Locales.Add(DefaultExportLocale);

		FName tableId;
		FString sourceKey;
		if (!FTextInspector::GetTableIdAndKey(Text, tableId, sourceKey))
			return;

		// The inspector returns the string table asset path; the .po files are named after the asset.
		const TMap<FString, TMap<FString, FString>>* perLocale = PoByTable.Find(FPackageName::ObjectPathToObjectName(tableId.ToString()));
		if (!perLocale)
			return;

		for (const TPair<FString, TMap<FString, FString>>& localePair : *perLocale)
		{
			const FString* translated = localePair.Value.Find(sourceKey);
			if (translated && !translated->IsEmpty())
			{
				Entry.Values.Add(localePair.Key, *translated);
				Builder.Locales.Add(localePair.Key);
			}
		}
	}

	UMounteaDialogueGraph* ResolveChildGraph(const UMounteaDialogueGraphNode_OpenChildGraph* Node)
	{
		if (!Node)
			return nullptr;
		return Node->TargetDialogue.IsNull() ? nullptr : Node->TargetDialogue.LoadSynchronous();
	}

	void CollectClosure(const UMounteaDialogueGraph* Graph, TArray<const UMounteaDialogueGraph*>& InOutGraphs)
	{
		if (!Graph || InOutGraphs.Contains(Graph))
			return;
		InOutGraphs.Add(Graph);

		for (const UMounteaDialogueGraphNode* node : Graph->GetAllNodes())
		{
			if (const UMounteaDialogueGraphNode_OpenChildGraph* openNode = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(node))
				CollectClosure(ResolveChildGraph(openNode), InOutGraphs);
		}
	}
}

#define LOCTEXT_NAMESPACE "MounteaDialogueSystemImportExportHelpers"

bool UMounteaDialogueSystemImportExportHelpers::BuildArchiveFromGraphs(const TArray<const UMounteaDialogueGraph*>& Graphs, const UMounteaDialogueGraph* Root, const bool bProject,
	const FString& ProjectName, MounteaArchive::FDialogueArchive& OutArchive, TArray<FString>& OutErrors)
{
	if (Graphs.IsEmpty() || (!bProject && !Root))
	{
		OutErrors.Add(TEXT("NOTHING_TO_EXPORT: No dialogue graph was given."));
		return false;
	}

	// Child dialogues are part of the archive, otherwise Open Child Graph nodes would dangle.
	TArray<const UMounteaDialogueGraph*> graphs;
	for (const UMounteaDialogueGraph* graph : Graphs)
		CollectClosure(graph, graphs);

	OutArchive = FDialogueArchive();
	OutArchive.Kind = bProject ? EKind::Project : EKind::Dialogue;
	OutArchive.Project.Name = ProjectName;
	OutArchive.Project.Localization.DefaultLocale = DefaultExportLocale;

	// Reuse the project identity recorded at import time; Dialoguer then recognises it on a replace-import.
	OutArchive.Project.Id = GuidToId(FGuid::NewGuid());
	if (const UMounteaDialogueImportConfig* importConfig = GetDefault<UMounteaDialogueImportConfig>())
	{
		for (const TPair<FGuid, FProjectImportRecord>& pair : importConfig->ProjectHistory)
		{
			if (pair.Value.DialogueGuids.Contains(graphs[0]->GetGraphGUID()))
			{
				OutArchive.Project.Id = GuidToId(pair.Key);
				break;
			}
		}
	}

	FExportBuilder builder(OutArchive, OutErrors);

	FAssetRegistryModule& assetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Translations live in .po files next to the Unreal string tables; read each table once.
	TMap<FString, TMap<FString, TMap<FString, FString>>> poByTable;

	for (const UMounteaDialogueGraph* graph : graphs)
	{
		FDialogue dialogue;
		dialogue.Id = GuidToId(graph->GetGraphGUID());
		dialogue.Name = graph->GetName();

		FString slug = Slugify(dialogue.Name, 40);
		if (slug.IsEmpty())
			slug = TEXT("dialogue");
		for (int32 suffix = 2; builder.UsedSlugs.Contains(slug); ++suffix)
			slug = FString::Printf(TEXT("%s_%d"), *Slugify(dialogue.Name, 36), suffix);
		builder.UsedSlugs.Add(slug);
		dialogue.Slug = slug;

		builder.DialogueIds.Add(graph, dialogue.Id);
		OutArchive.Dialogues.Add(dialogue);
		if (!bProject && graph == Root)
			OutArchive.RootDialogueId = dialogue.Id;

		for (const FString& suffix : { FString(TEXT("DialogueRows")), FString(TEXT("Nodes")) })
		{
			const FString tableId = FString::Printf(TEXT("ST_%s_%s"), *graph->GetName(), *suffix);
			poByTable.Add(tableId, ReadPoTranslations(tableId));
		}

		// Participants table of this graph (tag + icon), by participant name.
		TMap<FName, const FDialogueParticipant*> participantRows;
		{
			const FString packagePath = FPackageName::GetLongPackagePath(graph->GetPathName());
			const FString tableName = FString::Printf(TEXT("DT_%s_Participants"), *graph->GetName());
			if (const UDataTable* participantsTable = Cast<UDataTable>(assetRegistry.Get().GetAssetByObjectPath(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), *packagePath, *tableName, *tableName))).GetAsset()))
			{
				for (const TPair<FName, uint8*>& rowPair : participantsTable->GetRowMap())
					participantRows.Add(rowPair.Key, reinterpret_cast<const FDialogueParticipant*>(rowPair.Value));
			}
		}

		TSet<FString> usedNodeTokens;
		TMap<const UMounteaDialogueGraphNode*, int32> edgeCounters;
		int32 edgeIndex = 0;

		TArray<const UMounteaDialogueGraphNode*> nodes;
		for (const UMounteaDialogueGraphNode* node : graph->GetAllNodes())
		{
			if (node)
				nodes.Add(node);
		}
		if (graph->GetStartNode() && !nodes.Contains(graph->GetStartNode()))
			nodes.Insert(graph->GetStartNode(), 0);

		for (const UMounteaDialogueGraphNode* node : nodes)
		{
			const TCHAR* type = TypeOf(node);
			if (!type)
			{
				OutErrors.Add(FString::Printf(TEXT("UNSUPPORTED_NODE_TYPE: Node '%s' (%s) cannot be exported."), *node->GetName(), *node->GetClass()->GetName()));
				continue;
			}

			FNode record;
			record.Id = NodeIdOf(node);
			record.DialogueId = dialogue.Id;
			record.Type = type;
			record.PosX = node->NodePosition.X;
			record.PosY = node->NodePosition.Y;
			record.Label = node->NodeTitle.ToString();
			record.NodeToken = UniqueToken(record.Label.IsEmpty() ? FString(type) : record.Label, TEXT("node"), node->GetNodeGUID(), usedNodeTokens);

			// Decorators
			for (const FMounteaDialogueDecorator& decorator : node->NodeDecorators)
			{
				if (!decorator.DecoratorType)
					continue;
				FDecoratorInstance instance;
				instance.DefinitionId = EnsureDecoratorDefinition(builder, decorator.DecoratorType);
				instance.Name = ClassDisplayName(decorator.DecoratorType->GetClass());
				if (const FDefinition* definition = OutArchive.FindDecorator(instance.DefinitionId))
					instance.Name = definition->Name;
				instance.Values = PropertyValues(decorator.DecoratorType, OwnProperties(decorator.DecoratorType->GetClass(), UMounteaDialogueDecoratorBase::StaticClass()));
				record.Decorators.Add(MoveTemp(instance));
			}

			if (const UMounteaDialogueGraphNode_Delay* delayNode = Cast<UMounteaDialogueGraphNode_Delay>(node))
			{
				record.bHasDuration = true;
				record.Duration = delayNode->GetDelayDuration();
			}
			else if (const UMounteaDialogueGraphNode_ReturnToNode* returnNode = Cast<UMounteaDialogueGraphNode_ReturnToNode>(node))
			{
				record.TargetNode = NodeIdOf(returnNode->SelectedNode);
			}
			else if (const UMounteaDialogueGraphNode_OpenChildGraph* openNode = Cast<UMounteaDialogueGraphNode_OpenChildGraph>(node))
			{
				if (const UMounteaDialogueGraph* childGraph = ResolveChildGraph(openNode))
					record.TargetDialogue = GuidToId(childGraph->GetGraphGUID());
				else if (openNode->PendingTargetDialogueGUID.IsValid())
					record.TargetDialogue = GuidToId(openNode->PendingTargetDialogueGUID);
			}

			// Dialogue rows and the texts behind them
			const UMounteaDialogueGraphNode_DialogueNodeBase* dialogueNode = Cast<UMounteaDialogueGraphNode_DialogueNodeBase>(node);
			const FDialogueRow* dialogueRow = (dialogueNode && dialogueNode->GetDataTable())
				? dialogueNode->GetDataTable()->FindRow<FDialogueRow>(dialogueNode->GetRowName(), TEXT("")) : nullptr;

			FText displayName = node->NodeTitle;
			FText selectionTitle;
			if (dialogueRow)
			{
				selectionTitle = dialogueRow->RowTitle;
				if (displayName.IsEmpty())
					displayName = dialogueRow->RowTitle;

				const FString participantName = dialogueRow->DialogueParticipantName.ToString();
				const FDialogueParticipant* const* tableRow = participantRows.Find(dialogueRow->DialogueParticipantName);
				FGameplayTag tag = tableRow && *tableRow ? (*tableRow)->ParticipantCategoryTag : FGameplayTag();
				if (!tag.IsValid() && !dialogueRow->CompatibleTags.IsEmpty())
					tag = dialogueRow->CompatibleTags.First();

				record.ParticipantName = participantName;
				record.ParticipantId = EnsureParticipant(builder, participantName, tag, tableRow ? *tableRow : nullptr);
			}

			auto addString = [&](const FString& Field, const FText& Text, const FString& RowToken, const FString& RowId) -> FString
			{
				FLocalizedString entry;
				entry.DialogueId = dialogue.Id;
				entry.NodeId = record.Id;
				entry.RowId = RowId;
				entry.Field = Field;
				entry.DialogueSlug = dialogue.Slug;
				entry.NodeToken = record.NodeToken;
				entry.RowToken = RowToken;
				const TCHAR* suffix = Field == Contract::FieldDisplayName ? TEXT("display_name") : (Field == Contract::FieldSelectionTitle ? TEXT("selection_title") : TEXT("text"));
				entry.Key = RowToken.IsEmpty()
					? FString::Printf(TEXT("dlg.%s.n_%s.%s"), *dialogue.Slug, *record.NodeToken, suffix)
					: FString::Printf(TEXT("dlg.%s.n_%s.r_%s.%s"), *dialogue.Slug, *record.NodeToken, *RowToken, suffix);
				FillLocalizedValues(entry, Text, poByTable, builder);
				OutArchive.Strings.Add(entry);
				return entry.Key;
			};

			if (!displayName.IsEmpty())
				record.DisplayNameKey = addString(Contract::FieldDisplayName, displayName, FString(), FString());
			if (!selectionTitle.IsEmpty())
				record.SelectionTitleKey = addString(Contract::FieldSelectionTitle, selectionTitle, FString(), FString());

			if (dialogueRow)
			{
				TSet<FString> usedRowTokens;
				for (const FDialogueRowData& rowData : dialogueRow->RowData)
				{
					FRow row;
					row.Id = GuidToId(rowData.RowGUID);
					row.Duration = rowData.RowDuration;
					row.DurationMode = rowData.RowDurationMode == ERowDurationMode::Default ? ERowDurationMode::ERDM_Duration : rowData.RowDurationMode;
					row.DurationOverride = rowData.RowDurationOverride;
					row.ExecutionBehaviour = rowData.RowExecutionBehaviour == ERowExecutionMode::Default ? ERowExecutionMode::EREM_Automatic : rowData.RowExecutionBehaviour;
					// Manual is hidden in Unreal and not part of the contract.
					if (row.DurationMode == ERowDurationMode::ERDM_Manual)
						row.DurationMode = ERowDurationMode::ERDM_Duration;

					row.RowToken = UniqueToken(rowData.RowText.ToString(), TEXT("row"), rowData.RowGUID, usedRowTokens);
					row.TextKey = addString(Contract::FieldRowText, rowData.RowText, row.RowToken, row.Id);

					if (rowData.RowSound)
					{
						FMedia audio;
						if (SoundToWav(Cast<USoundWave>(rowData.RowSound), audio.Bytes))
						{
							audio.Name = rowData.RowSound->GetName() + TEXT(".wav");
							audio.MimeType = TEXT("audio/wav");
							row.Audio = MoveTemp(audio);
							row.bHasAudio = true;
						}
						else
							OutErrors.Add(FString::Printf(TEXT("AUDIO_EXPORT_FAILED: Could not read the source audio of '%s'."), *rowData.RowSound->GetPathName()));
					}
					record.Rows.Add(MoveTemp(row));
				}
			}

			OutArchive.Nodes.Add(MoveTemp(record));
		}

		// Edges: sibling order is the edge order, so children are written sorted by ExecutionOrder.
		for (const UMounteaDialogueGraphNode* node : nodes)
		{
			TArray<UMounteaDialogueGraphNode*> children = node->ChildrenNodes;
			children.RemoveAll([](const UMounteaDialogueGraphNode* Child) { return Child == nullptr; });
			children.StableSort([](const UMounteaDialogueGraphNode& A, const UMounteaDialogueGraphNode& B) { return A.ExecutionOrder < B.ExecutionOrder; });

			for (const UMounteaDialogueGraphNode* child : children)
			{
				FArchiveEdge edge;
				edge.Id = FString::Printf(TEXT("e%d"), ++edgeIndex);
				edge.DialogueId = dialogue.Id;
				edge.SourceId = NodeIdOf(node);
				edge.TargetId = NodeIdOf(child);

				const UMounteaDialogueGraphEdge* graphEdge = node->Edges.FindRef(child);
				if (graphEdge && !graphEdge->EdgeConditions.Rules.IsEmpty())
				{
					edge.ConditionMode = graphEdge->EdgeConditions.Mode == EConditionEvaluationMode::Any ? TEXT("any") : TEXT("all");
					for (const FMounteaDialogueCondition& condition : graphEdge->EdgeConditions.Rules)
					{
						if (!condition.ConditionClass)
							continue;
						FConditionRule rule;
						rule.DefinitionId = EnsureConditionDefinition(builder, condition.ConditionClass);
						rule.Name = OutArchive.FindCondition(rule.DefinitionId) ? OutArchive.FindCondition(rule.DefinitionId)->Name : ClassDisplayName(condition.ConditionClass->GetClass());
						rule.bNegate = condition.bNegate;
						rule.Values = PropertyValues(condition.ConditionClass, OwnProperties(condition.ConditionClass->GetClass(), UMounteaDialogueConditionBase::StaticClass()));
						edge.Rules.Add(MoveTemp(rule));
					}
				}
				OutArchive.Edges.Add(MoveTemp(edge));
			}
		}
	}

	for (const FString& locale : builder.Locales)
		OutArchive.Project.Localization.SupportedLocales.AddUnique(locale);
	OutArchive.Project.Localization.SupportedLocales.AddUnique(OutArchive.Project.Localization.DefaultLocale);

	return OutErrors.IsEmpty();
}

bool UMounteaDialogueSystemImportExportHelpers::SaveArchiveToFile(const MounteaArchive::FDialogueArchive& Archive, const FString& FilePath, FString& OutMessage)
{
	TArray<FString> errors;
	TArray<uint8> zipped;
	if (!MounteaArchive::WriteArchiveToBytes(Archive, zipped, errors))
	{
		OutMessage = FString::Join(errors, TEXT("\n"));
		EditorLOG_ERROR(TEXT("[SaveArchiveToFile] %s"), *OutMessage);
		return false;
	}

	if (!FFileHelper::SaveArrayToFile(zipped, *FilePath))
	{
		OutMessage = FString::Printf(TEXT("Failed to write file: %s"), *FilePath);
		EditorLOG_ERROR(TEXT("[SaveArchiveToFile] %s"), *OutMessage);
		return false;
	}
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::ExportDialogueGraph(const UMounteaDialogueGraph* Graph, const FString& FilePath)
{
	if (!Graph)
	{
		EditorLOG_ERROR(TEXT("Invalid Graph object provided for export"));
		return false;
	}

	MounteaArchive::FDialogueArchive archive;
	TArray<FString> errors;
	if (!BuildArchiveFromGraphs({ Graph }, Graph, false, Graph->GetName(), archive, errors))
	{
		const FString message = FString::Join(errors, TEXT("\n"));
		EditorLOG_ERROR(TEXT("[ExportDialogueGraph] %s"), *message);
		ShowNotification(FText::Format(LOCTEXT("ExportFailed", "Export failed:\n{0}"), FText::FromString(message)), 10.f, TEXT("MDSStyleSet.Info.Error"));
		return false;
	}

	FString message;
	if (!SaveArchiveToFile(archive, FilePath, message))
	{
		ShowNotification(FText::Format(LOCTEXT("ExportWriteFailed", "Export failed:\n{0}"), FText::FromString(message)), 10.f, TEXT("MDSStyleSet.Info.Error"));
		return false;
	}
	return true;
}

bool UMounteaDialogueSystemImportExportHelpers::ExportProject(const TArray<UMounteaDialogueGraph*>& Graphs, const FString& FilePath)
{
	TArray<const UMounteaDialogueGraph*> graphs;
	for (const UMounteaDialogueGraph* graph : Graphs)
	{
		if (graph)
			graphs.Add(graph);
	}

	MounteaArchive::FDialogueArchive archive;
	TArray<FString> errors;
	if (!BuildArchiveFromGraphs(graphs, nullptr, true, FPaths::GetBaseFilename(FilePath), archive, errors))
	{
		const FString message = FString::Join(errors, TEXT("\n"));
		EditorLOG_ERROR(TEXT("[ExportProject] %s"), *message);
		ShowNotification(FText::Format(LOCTEXT("ProjectExportFailed", "Project export failed:\n{0}"), FText::FromString(message)), 10.f, TEXT("MDSStyleSet.Info.Error"));
		return false;
	}

	FString message;
	if (!SaveArchiveToFile(archive, FilePath, message))
	{
		ShowNotification(FText::Format(LOCTEXT("ProjectExportWriteFailed", "Project export failed:\n{0}"), FText::FromString(message)), 10.f, TEXT("MDSStyleSet.Info.Error"));
		return false;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
