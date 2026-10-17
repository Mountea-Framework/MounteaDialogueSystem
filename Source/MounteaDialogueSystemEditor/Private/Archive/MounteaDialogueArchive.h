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

#pragma once

#include "CoreMinimal.h"
#include "Data/MounteaDialogueGraphDataTypes.h"
#include "Dom/JsonObject.h"

/**
 * Pure (no UObject) model of the `mountea.archive.v3` interchange format shared with Mountea Dialoguer.
 *
 * Normative definition lives in Dialoguer (src/lib/persistence/projectArchive.js, canonicalProject.js,
 * localization/stringTable.js, rowTiming.js). Docs/ArchiveContract-v3.md summarises it for this plugin.
 * Everything that touches the format (import, export, tests) goes through this header, so the contract
 * has exactly one definition on the Unreal side.
 */
namespace MounteaArchive
{
	namespace Contract
	{
		extern const TCHAR* const Format;
		extern const TCHAR* const StartNodeId;

		extern const TCHAR* const KindProject;
		extern const TCHAR* const KindDialogue;

		/** Participant and category names: letters and digits only, 1..NameMaxLength characters. Single source of the rule. */
		inline constexpr int32 NameMaxLength = 16;
		bool IsValidName(const FString& Name);
		FString NameRuleDescription();

		/** Same limits Dialoguer enforces on both reading and writing. */
		inline constexpr int64 MaxCompressedBytes = 25ll * 1024 * 1024;
		inline constexpr int64 MaxJsonEntryBytes = 5ll * 1024 * 1024;
		inline constexpr int64 MaxExpandedBytes = 128ll * 1024 * 1024;
		inline constexpr int32 MaxEntries = 1000;
		inline constexpr int32 MaxDialogues = 250;

		// Node type ids.
		extern const TCHAR* const NodeStart;
		extern const TCHAR* const NodeLead;
		extern const TCHAR* const NodeAnswer;
		extern const TCHAR* const NodeComplete;
		extern const TCHAR* const NodeReturn;
		extern const TCHAR* const NodeOpenChildGraph;
		extern const TCHAR* const NodeDelay;
		bool IsKnownNodeType(const FString& Type);

		// Localized string fields.
		extern const TCHAR* const FieldDisplayName;
		extern const TCHAR* const FieldSelectionTitle;
		extern const TCHAR* const FieldRowText;

		// Property types of decorator/condition definitions.
		extern const TCHAR* const PropertyString;
		extern const TCHAR* const PropertyNumber;
		extern const TCHAR* const PropertyBoolean;

		// Row timing. Absent value means default; defaults are never written.
		bool ParseExecutionBehaviour(const FString& Text, ERowExecutionMode& Out);
		FString ExecutionBehaviourToString(ERowExecutionMode Mode);
		bool ParseDurationMode(const FString& Text, ERowDurationMode& Out);
		FString DurationModeToString(ERowDurationMode Mode);
	}

	struct FMedia
	{
		FString Name;
		FString MimeType;
		TArray<uint8> Bytes;
		/** Size stated by the record metadata, -1 when absent. Checked against Bytes on read. */
		int64 DeclaredSize = -1;
		/** Image dimensions (thumbnails only), 0 when unknown. */
		int32 Width = 0;
		int32 Height = 0;

		bool IsSet() const { return Bytes.Num() > 0; }
	};

	struct FLocalizationConfig
	{
		FString DefaultLocale = TEXT("en");
		TArray<FString> SupportedLocales;
	};

	struct FProject
	{
		FString Id;
		FString Name;
		FString Description;
		FLocalizationConfig Localization;
	};

	struct FDialogue
	{
		FString Id;
		FString Name;
		FString Description;
		FString Slug;
	};

	struct FCategory
	{
		FString Id;
		FString Name;
		FString ParentId;
	};

	struct FParticipant
	{
		FString Id;
		FString Name;
		FString CategoryId;
		FMedia Thumbnail;
		bool bHasThumbnail = false;
	};

	struct FPropertyDef
	{
		FString Name;
		FString Type;
		/** Default value as text (number/boolean converted); empty when absent. */
		FString DefaultValue;
	};

	/** Decorator or condition definition. */
	struct FDefinition
	{
		FString Id;
		FString Name;
		FString Type;
		TArray<FPropertyDef> Properties;
	};

	struct FDecoratorInstance
	{
		FString DefinitionId;
		FString Name;
		/** Typed values (string/number/boolean), keyed by property name. */
		TSharedPtr<FJsonObject> Values;
	};

	struct FRow
	{
		FString Id;
		FString TextKey;
		FString RowToken;
		float Duration = 3.f;
		ERowExecutionMode ExecutionBehaviour = ERowExecutionMode::EREM_Automatic;
		ERowDurationMode DurationMode = ERowDurationMode::ERDM_Duration;
		float DurationOverride = 0.f;
		FString ParticipantId;
		FMedia Audio;
		bool bHasAudio = false;
	};

	struct FNode
	{
		FString Id;
		FString DialogueId;
		FString Type;
		double PosX = 0.0;
		double PosY = 0.0;
		FString Label;
		FString NodeToken;
		FString DisplayNameKey;
		FString SelectionTitleKey;
		FString ParticipantId;
		FString ParticipantName;
		TArray<FDecoratorInstance> Decorators;
		TArray<FRow> Rows;
		bool bHasDuration = false;
		float Duration = 0.f;
		FString TargetNode;
		FString TargetDialogue;
	};

	struct FConditionRule
	{
		FString DefinitionId;
		FString Name;
		bool bNegate = false;
		TSharedPtr<FJsonObject> Values;
	};

	struct FArchiveEdge
	{
		FString Id;
		FString DialogueId;
		FString SourceId;
		FString TargetId;
		/** "all" or "any"; empty when the edge has no conditions. */
		FString ConditionMode;
		TArray<FConditionRule> Rules;
	};

	struct FLocalizedString
	{
		FString Key;
		FString DialogueId;
		FString NodeId;
		FString RowId;
		FString Field;
		FString DialogueSlug;
		FString NodeToken;
		FString RowToken;
		/** locale -> text */
		TMap<FString, FString> Values;
	};

	enum class EKind : uint8
	{
		Project,
		Dialogue
	};

	struct FDialogueArchive
	{
		EKind Kind = EKind::Dialogue;
		/** Required for EKind::Dialogue. */
		FString RootDialogueId;

		FProject Project;
		TArray<FDialogue> Dialogues;
		TArray<FCategory> Categories;
		TArray<FParticipant> Participants;
		TArray<FDefinition> Decorators;
		TArray<FDefinition> Conditions;
		/** Array order is meaningful only for edges; see Edges. */
		TArray<FNode> Nodes;
		/** Edge array order defines sibling execution order (same as Dialoguer). */
		TArray<FArchiveEdge> Edges;
		TArray<FLocalizedString> Strings;

		const FNode* FindNode(const FString& DialogueId, const FString& NodeId) const;
		const FParticipant* FindParticipant(const FString& Id) const;
		const FCategory* FindCategory(const FString& Id) const;
		const FDialogue* FindDialogue(const FString& Id) const;
		const FDefinition* FindDecorator(const FString& Id) const;
		const FDefinition* FindCondition(const FString& Id) const;
		const FLocalizedString* FindString(const FString& Key) const;
	};

	/** Entry name -> raw bytes, exactly as stored in the zip. */
	using FEntries = TMap<FString, TArray<uint8>>;

	/**
	 * Parses and validates. Collects every problem it can find into OutErrors (never partial success).
	 * A zip that looks like the pre-v3 flat layout produces one dedicated error.
	 */
	bool ReadArchive(const FEntries& Entries, FDialogueArchive& OutArchive, TArray<FString>& OutErrors);

	/** Validates the model with the same rules as ReadArchive, then serialises it into zip entries. */
	bool WriteArchive(const FDialogueArchive& Archive, FEntries& OutEntries, TArray<FString>& OutErrors);

	/** Structural and referential validation shared by read and write. */
	void ValidateArchive(const FDialogueArchive& Archive, TArray<FString>& OutErrors);

	/** Stable GUID for a v3 id: the id itself when it parses as a GUID, otherwise a deterministic MD5-derived GUID. */
	FGuid IdToGuid(const FString& Id);
	/** Export-side id text for a GUID (matches the lowercase hyphenated form Dialoguer uses). */
	FString GuidToId(const FGuid& Guid);
}
