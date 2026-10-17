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

#include "Archive/MounteaDialogueArchive.h"

#include "Dom/JsonValue.h"
#include "Misc/Base64.h"
#include "Misc/DateTime.h"
#include "Misc/SecureHash.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace MounteaArchive
{
	namespace Contract
	{
		const TCHAR* const Format = TEXT("mountea.archive.v3");
		const TCHAR* const StartNodeId = TEXT("00000000-0000-0000-0000-000000000001");

		const TCHAR* const KindProject = TEXT("project");
		const TCHAR* const KindDialogue = TEXT("dialogue");

		const TCHAR* const NodeStart = TEXT("startNode");
		const TCHAR* const NodeLead = TEXT("leadNode");
		const TCHAR* const NodeAnswer = TEXT("answerNode");
		const TCHAR* const NodeComplete = TEXT("completeNode");
		const TCHAR* const NodeReturn = TEXT("returnNode");
		const TCHAR* const NodeOpenChildGraph = TEXT("openChildGraphNode");
		const TCHAR* const NodeDelay = TEXT("delayNode");

		const TCHAR* const FieldDisplayName = TEXT("displayName");
		const TCHAR* const FieldSelectionTitle = TEXT("selectionTitle");
		const TCHAR* const FieldRowText = TEXT("rowText");

		const TCHAR* const PropertyString = TEXT("string");
		const TCHAR* const PropertyNumber = TEXT("number");
		const TCHAR* const PropertyBoolean = TEXT("boolean");

		bool IsValidName(const FString& Name)
		{
			if (Name.IsEmpty() || Name.Len() > NameMaxLength)
				return false;

			for (const TCHAR character : Name)
			{
				const bool bIsDigit = character >= TEXT('0') && character <= TEXT('9');
				const bool bIsUpper = character >= TEXT('A') && character <= TEXT('Z');
				const bool bIsLower = character >= TEXT('a') && character <= TEXT('z');
				if (!bIsDigit && !bIsUpper && !bIsLower)
					return false;
			}
			return true;
		}

		FString NameRuleDescription()
		{
			return FString::Printf(TEXT("1-%d letters or digits (A-Z, a-z, 0-9), no spaces or other characters"), NameMaxLength);
		}

		bool IsKnownNodeType(const FString& Type)
		{
			return Type == NodeStart || Type == NodeLead || Type == NodeAnswer || Type == NodeComplete
				|| Type == NodeReturn || Type == NodeOpenChildGraph || Type == NodeDelay;
		}

		bool ParseExecutionBehaviour(const FString& Text, ERowExecutionMode& Out)
		{
			if (Text == TEXT("automatic")) { Out = ERowExecutionMode::EREM_Automatic; return true; }
			if (Text == TEXT("awaitInput")) { Out = ERowExecutionMode::EREM_AwaitInput; return true; }
			if (Text == TEXT("stopping")) { Out = ERowExecutionMode::EREM_Stopping; return true; }
			return false;
		}

		FString ExecutionBehaviourToString(const ERowExecutionMode Mode)
		{
			switch (Mode)
			{
				case ERowExecutionMode::EREM_AwaitInput: return TEXT("awaitInput");
				case ERowExecutionMode::EREM_Stopping: return TEXT("stopping");
				default: return TEXT("automatic");
			}
		}

		bool ParseDurationMode(const FString& Text, ERowDurationMode& Out)
		{
			if (Text == TEXT("duration")) { Out = ERowDurationMode::ERDM_Duration; return true; }
			if (Text == TEXT("override")) { Out = ERowDurationMode::EDRM_Override; return true; }
			if (Text == TEXT("add")) { Out = ERowDurationMode::EDRM_Add; return true; }
			if (Text == TEXT("calculate")) { Out = ERowDurationMode::ERDM_AutoCalculate; return true; }
			return false;
		}

		FString DurationModeToString(const ERowDurationMode Mode)
		{
			switch (Mode)
			{
				case ERowDurationMode::EDRM_Override: return TEXT("override");
				case ERowDurationMode::EDRM_Add: return TEXT("add");
				case ERowDurationMode::ERDM_AutoCalculate: return TEXT("calculate");
				default: return TEXT("duration");
			}
		}
	}

	// ---------------------------------------------------------------------------------------------
	// Lookups
	// ---------------------------------------------------------------------------------------------

	const FNode* FDialogueArchive::FindNode(const FString& DialogueId, const FString& NodeId) const
	{
		return this->Nodes.FindByPredicate([&](const FNode& Candidate) { return Candidate.DialogueId == DialogueId && Candidate.Id == NodeId; });
	}

	const FParticipant* FDialogueArchive::FindParticipant(const FString& Id) const
	{
		return this->Participants.FindByPredicate([&](const FParticipant& Candidate) { return Candidate.Id == Id; });
	}

	const FCategory* FDialogueArchive::FindCategory(const FString& Id) const
	{
		return this->Categories.FindByPredicate([&](const FCategory& Candidate) { return Candidate.Id == Id; });
	}

	const FDialogue* FDialogueArchive::FindDialogue(const FString& Id) const
	{
		return this->Dialogues.FindByPredicate([&](const FDialogue& Candidate) { return Candidate.Id == Id; });
	}

	const FDefinition* FDialogueArchive::FindDecorator(const FString& Id) const
	{
		return this->Decorators.FindByPredicate([&](const FDefinition& Candidate) { return Candidate.Id == Id; });
	}

	const FDefinition* FDialogueArchive::FindCondition(const FString& Id) const
	{
		return this->Conditions.FindByPredicate([&](const FDefinition& Candidate) { return Candidate.Id == Id; });
	}

	const FLocalizedString* FDialogueArchive::FindString(const FString& Key) const
	{
		return this->Strings.FindByPredicate([&](const FLocalizedString& Candidate) { return Candidate.Key == Key; });
	}

	// ---------------------------------------------------------------------------------------------
	// Ids
	// ---------------------------------------------------------------------------------------------

	FGuid IdToGuid(const FString& Id)
	{
		FGuid parsed;
		if (FGuid::Parse(Id, parsed))
			return parsed;

		const FTCHARToUTF8 Utf8(*Id);
		FMD5 hash;
		hash.Update(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
		uint8 digest[16];
		hash.Final(digest);

		uint32 parts[4];
		FMemory::Memcpy(parts, digest, sizeof(parts));
		return FGuid(parts[0], parts[1], parts[2], parts[3]);
	}

	FString GuidToId(const FGuid& Guid)
	{
		return Guid.ToString(EGuidFormats::DigitsWithHyphensLower);
	}

	// ---------------------------------------------------------------------------------------------
	// JSON helpers
	// ---------------------------------------------------------------------------------------------

	namespace
	{
		/** JSON object keys are FString up to UE 5.7 and a shared-string type from 5.8. */
		template <typename KeyType>
		FString JsonKeyToString(const KeyType& Key)
		{
			if constexpr (std::is_convertible_v<const KeyType&, FString>)
				return FString(Key);
			else
				return FString(Key.ToView());
		}

		FString BytesToText(const TArray<uint8>& Bytes)
		{
			int32 offset = 0;
			if (Bytes.Num() >= 3 && Bytes[0] == 0xEF && Bytes[1] == 0xBB && Bytes[2] == 0xBF)
				offset = 3;

			const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes.GetData() + offset), Bytes.Num() - offset);
			return FString(Converted.Length(), Converted.Get());
		}

		TArray<uint8> TextToBytes(const FString& Text)
		{
			const FTCHARToUTF8 Converted(*Text);
			TArray<uint8> bytes;
			bytes.Append(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
			return bytes;
		}

		FString GetString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
		{
			FString value;
			if (Object.IsValid())
				Object->TryGetStringField(Field, value);
			return value;
		}

		TSharedPtr<FJsonObject> GetObject(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
		{
			if (!Object.IsValid())
				return nullptr;
			const TSharedPtr<FJsonObject>* found = nullptr;
			if (Object->TryGetObjectField(Field, found) && found)
				return *found;
			return nullptr;
		}

		/** Number, boolean or string as text. Returns false for missing/other values. */
		bool JsonScalarToText(const TSharedPtr<FJsonValue>& Value, FString& Out)
		{
			if (!Value.IsValid())
				return false;
			switch (Value->Type)
			{
				case EJson::String: Out = Value->AsString(); return true;
				case EJson::Number: Out = FString::SanitizeFloat(Value->AsNumber()); return true;
				case EJson::Boolean: Out = Value->AsBool() ? TEXT("true") : TEXT("false"); return true;
				default: return false;
			}
		}

		bool ParseJsonObject(const TArray<uint8>& Bytes, TSharedPtr<FJsonObject>& OutObject)
		{
			const TSharedRef<TJsonReader<>> jsonReader = TJsonReaderFactory<>::Create(BytesToText(Bytes));
			return FJsonSerializer::Deserialize(jsonReader, OutObject) && OutObject.IsValid();
		}

		TArray<uint8> SerializeJsonObject(const TSharedRef<FJsonObject>& Object)
		{
			FString text;
			const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> jsonWriter =
				TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&text);
			FJsonSerializer::Serialize(Object, jsonWriter);
			return TextToBytes(text);
		}

		bool DecodeDataUrl(const FString& DataUrl, TArray<uint8>& OutBytes)
		{
			int32 comma = INDEX_NONE;
			if (!DataUrl.StartsWith(TEXT("data:")) || !DataUrl.FindChar(TEXT(','), comma))
				return false;
			return FBase64::Decode(DataUrl.Mid(comma + 1), OutBytes);
		}

		FString Describe(const FString& Where, const FString& Message)
		{
			return Where.IsEmpty() ? Message : FString::Printf(TEXT("%s (%s)"), *Message, *Where);
		}

		FString SanitizeFileName(const FString& Raw)
		{
			FString result;
			for (const TCHAR character : Raw)
			{
				const bool bSafe = (character >= TEXT('0') && character <= TEXT('9')) || (character >= TEXT('A') && character <= TEXT('Z'))
					|| (character >= TEXT('a') && character <= TEXT('z')) || character == TEXT('_') || character == TEXT('-');
				result.AppendChar(bSafe ? character : TEXT('_'));
				if (result.Len() >= 70)
					break;
			}
			return result.IsEmpty() ? FString(TEXT("record")) : result;
		}

		// -----------------------------------------------------------------------------------------
		// Reading
		// -----------------------------------------------------------------------------------------

		struct FReadContext
		{
			const FEntries& Entries;
			FDialogueArchive& Archive;
			TArray<FString>& Errors;
			TSet<FString> UsedPaths;

			FReadContext(const FEntries& InEntries, FDialogueArchive& InArchive, TArray<FString>& InErrors)
				: Entries(InEntries), Archive(InArchive), Errors(InErrors) {}

			void Fail(const FString& Code, const FString& Message, const FString& Where = FString())
			{
				this->Errors.Add(FString::Printf(TEXT("%s: %s"), *Code, *Describe(Where, Message)));
			}

			bool LoadJson(const FString& Path, TSharedPtr<FJsonObject>& OutObject)
			{
				const TArray<uint8>* bytes = this->Entries.Find(Path);
				if (!bytes)
				{
					this->Fail(TEXT("MISSING_ARCHIVE_ENTRY"), TEXT("Required archive entry is missing."), Path);
					return false;
				}
				if (bytes->Num() > Contract::MaxJsonEntryBytes)
				{
					this->Fail(TEXT("ARCHIVE_JSON_LIMIT"), TEXT("JSON entry exceeds its size limit."), Path);
					return false;
				}
				if (!ParseJsonObject(*bytes, OutObject))
				{
					this->Fail(TEXT("INVALID_ARCHIVE_JSON"), TEXT("Archive contains invalid JSON."), Path);
					return false;
				}
				return true;
			}
		};

		FMedia ReadMediaMeta(const TSharedPtr<FJsonObject>& Object)
		{
			FMedia media;
			media.Name = GetString(Object, TEXT("name"));
			media.MimeType = GetString(Object, TEXT("mimeType"));
			double size = 0.0;
			if (Object->TryGetNumberField(TEXT("size"), size))
				media.DeclaredSize = static_cast<int64>(size);
			double dimension = 0.0;
			if (Object->TryGetNumberField(TEXT("width"), dimension))
				media.Width = static_cast<int32>(dimension);
			if (Object->TryGetNumberField(TEXT("height"), dimension))
				media.Height = static_cast<int32>(dimension);

			// Inline bytes are accepted as an alternative to a manifest media entry.
			FString inlineBytes = GetString(Object, TEXT("base64"));
			if (inlineBytes.IsEmpty())
				inlineBytes = GetString(Object, TEXT("dataUrl"));
			if (!inlineBytes.IsEmpty())
				DecodeDataUrl(inlineBytes, media.Bytes);
			return media;
		}

		void ReadDefinition(FReadContext& Context, const TSharedPtr<FJsonObject>& Object, TArray<FDefinition>& Target, const FString& Path)
		{
			FDefinition definition;
			definition.Id = GetString(Object, TEXT("id"));
			definition.Name = GetString(Object, TEXT("name"));
			definition.Type = GetString(Object, TEXT("type"));
			if (definition.Id.IsEmpty())
				Context.Fail(TEXT("INVALID_ID"), TEXT("Definition has no id."), Path);

			const TArray<TSharedPtr<FJsonValue>>* properties = nullptr;
			if (Object->TryGetArrayField(TEXT("properties"), properties) && properties)
			{
				for (const TSharedPtr<FJsonValue>& propertyValue : *properties)
				{
					const TSharedPtr<FJsonObject> propertyObject = propertyValue.IsValid() ? propertyValue->AsObject() : nullptr;
					if (!propertyObject.IsValid())
					{
						Context.Fail(TEXT("INVALID_RECORD"), TEXT("Property is not an object."), Path);
						continue;
					}
					FPropertyDef property;
					property.Name = GetString(propertyObject, TEXT("name"));
					property.Type = GetString(propertyObject, TEXT("type"));
					JsonScalarToText(propertyObject->TryGetField(TEXT("defaultValue")), property.DefaultValue);
					definition.Properties.Add(MoveTemp(property));
				}
			}
			Target.Add(MoveTemp(definition));
		}

		void ReadRow(FReadContext& Context, const TSharedPtr<FJsonObject>& RowObject, FNode& Node)
		{
			const FString where = FString::Printf(TEXT("node %s"), *Node.Id);
			FRow row;
			row.Id = GetString(RowObject, TEXT("id"));
			row.TextKey = GetString(RowObject, TEXT("textKey"));
			row.RowToken = GetString(RowObject, TEXT("localizationRowToken"));
			row.ParticipantId = GetString(RowObject, TEXT("participantId"));

			double number = 0.0;
			if (RowObject->TryGetNumberField(TEXT("duration"), number))
				row.Duration = static_cast<float>(number);

			FString text;
			if (RowObject->TryGetStringField(TEXT("executionBehaviour"), text) && !Contract::ParseExecutionBehaviour(text, row.ExecutionBehaviour))
				Context.Fail(TEXT("INVALID_ROW_TIMING"), FString::Printf(TEXT("Unknown executionBehaviour '%s'."), *text), where);
			if (RowObject->TryGetStringField(TEXT("durationMode"), text) && !Contract::ParseDurationMode(text, row.DurationMode))
				Context.Fail(TEXT("INVALID_ROW_TIMING"), FString::Printf(TEXT("Unknown durationMode '%s'."), *text), where);
			if (RowObject->HasField(TEXT("durationOverride")))
			{
				if (!RowObject->TryGetNumberField(TEXT("durationOverride"), number) || number < 0.0)
					Context.Fail(TEXT("INVALID_ROW_TIMING"), TEXT("durationOverride must be a number >= 0."), where);
				else
					row.DurationOverride = static_cast<float>(number);
			}

			if (const TSharedPtr<FJsonObject> audioObject = GetObject(RowObject, TEXT("audioFile")))
			{
				row.bHasAudio = true;
				row.Audio = ReadMediaMeta(audioObject);
			}

			Node.Rows.Add(MoveTemp(row));
		}

		void ReadNode(FReadContext& Context, const TSharedPtr<FJsonObject>& Object, const FString& Path)
		{
			FNode node;
			node.Id = GetString(Object, TEXT("id"));
			node.DialogueId = GetString(Object, TEXT("dialogueId"));
			node.Type = GetString(Object, TEXT("type"));
			if (node.Id.IsEmpty() || node.DialogueId.IsEmpty())
				Context.Fail(TEXT("INVALID_ID"), TEXT("Node needs id and dialogueId."), Path);

			if (const TSharedPtr<FJsonObject> position = GetObject(Object, TEXT("position")))
			{
				position->TryGetNumberField(TEXT("x"), node.PosX);
				position->TryGetNumberField(TEXT("y"), node.PosY);
			}

			if (const TSharedPtr<FJsonObject> data = GetObject(Object, TEXT("data")))
			{
				node.Label = GetString(data, TEXT("label"));
				node.NodeToken = GetString(data, TEXT("localizationNodeToken"));
				node.DisplayNameKey = GetString(data, TEXT("displayNameKey"));
				node.SelectionTitleKey = GetString(data, TEXT("selectionTitleKey"));
				node.ParticipantId = GetString(data, TEXT("participantId"));
				node.ParticipantName = GetString(data, TEXT("participant"));
				node.TargetNode = GetString(data, TEXT("targetNode"));
				node.TargetDialogue = GetString(data, TEXT("targetDialogue"));

				double duration = 0.0;
				if (data->TryGetNumberField(TEXT("duration"), duration))
				{
					node.bHasDuration = true;
					node.Duration = static_cast<float>(duration);
				}

				const TArray<TSharedPtr<FJsonValue>>* decorators = nullptr;
				if (data->TryGetArrayField(TEXT("decorators"), decorators) && decorators)
				{
					for (const TSharedPtr<FJsonValue>& decoratorValue : *decorators)
					{
						const TSharedPtr<FJsonObject> decoratorObject = decoratorValue.IsValid() ? decoratorValue->AsObject() : nullptr;
						if (!decoratorObject.IsValid())
							continue;
						FDecoratorInstance instance;
						instance.DefinitionId = GetString(decoratorObject, TEXT("id"));
						instance.Name = GetString(decoratorObject, TEXT("name"));
						instance.Values = GetObject(decoratorObject, TEXT("values"));
						node.Decorators.Add(MoveTemp(instance));
					}
				}

				const TArray<TSharedPtr<FJsonValue>>* rows = nullptr;
				if (data->TryGetArrayField(TEXT("dialogueRows"), rows) && rows)
				{
					for (const TSharedPtr<FJsonValue>& rowValue : *rows)
					{
						const TSharedPtr<FJsonObject> rowObject = rowValue.IsValid() ? rowValue->AsObject() : nullptr;
						if (rowObject.IsValid())
							ReadRow(Context, rowObject, node);
					}
				}
			}

			Context.Archive.Nodes.Add(MoveTemp(node));
		}

		void ReadEdge(FReadContext& Context, const TSharedPtr<FJsonObject>& Object, const FString& Path)
		{
			FArchiveEdge edge;
			edge.Id = GetString(Object, TEXT("id"));
			edge.DialogueId = GetString(Object, TEXT("dialogueId"));
			edge.SourceId = GetString(Object, TEXT("source"));
			edge.TargetId = GetString(Object, TEXT("target"));
			if (edge.Id.IsEmpty() || edge.DialogueId.IsEmpty())
				Context.Fail(TEXT("INVALID_ID"), TEXT("Edge needs id and dialogueId."), Path);

			if (const TSharedPtr<FJsonObject> data = GetObject(Object, TEXT("data")))
			{
				if (const TSharedPtr<FJsonObject> conditions = GetObject(data, TEXT("conditions")))
				{
					edge.ConditionMode = GetString(conditions, TEXT("mode"));
					const TArray<TSharedPtr<FJsonValue>>* rules = nullptr;
					if (conditions->TryGetArrayField(TEXT("rules"), rules) && rules)
					{
						for (const TSharedPtr<FJsonValue>& ruleValue : *rules)
						{
							const TSharedPtr<FJsonObject> ruleObject = ruleValue.IsValid() ? ruleValue->AsObject() : nullptr;
							if (!ruleObject.IsValid())
								continue;
							FConditionRule rule;
							rule.DefinitionId = GetString(ruleObject, TEXT("id"));
							rule.Name = GetString(ruleObject, TEXT("name"));
							ruleObject->TryGetBoolField(TEXT("negate"), rule.bNegate);
							rule.Values = GetObject(ruleObject, TEXT("values"));
							edge.Rules.Add(MoveTemp(rule));
						}
					}
				}
			}
			Context.Archive.Edges.Add(MoveTemp(edge));
		}

		void ReadLocalizedString(FReadContext& Context, const TSharedPtr<FJsonObject>& Object, const FString& Path)
		{
			FLocalizedString entry;
			entry.Key = GetString(Object, TEXT("key"));
			entry.DialogueId = GetString(Object, TEXT("dialogueId"));
			entry.NodeId = GetString(Object, TEXT("nodeId"));
			entry.RowId = GetString(Object, TEXT("rowId"));
			entry.Field = GetString(Object, TEXT("field"));
			entry.DialogueSlug = GetString(Object, TEXT("dialogueSlug"));
			entry.NodeToken = GetString(Object, TEXT("nodeToken"));
			entry.RowToken = GetString(Object, TEXT("rowToken"));
			if (entry.Key.IsEmpty())
				Context.Fail(TEXT("INVALID_ID"), TEXT("Localized string has no key."), Path);

			if (const TSharedPtr<FJsonObject> values = GetObject(Object, TEXT("values")))
			{
				for (const auto& pair : values->Values)
				{
					FString text;
					if (pair.Value.IsValid() && pair.Value->TryGetString(text))
						entry.Values.Add(JsonKeyToString(pair.Key), text);
				}
			}
			Context.Archive.Strings.Add(MoveTemp(entry));
		}

		void ReadRecord(FReadContext& Context, const FString& Table, const TSharedPtr<FJsonObject>& Object, const FString& Path)
		{
			FDialogueArchive& archive = Context.Archive;

			if (Table == TEXT("dialogues"))
			{
				FDialogue dialogue;
				dialogue.Id = GetString(Object, TEXT("id"));
				dialogue.Name = GetString(Object, TEXT("name"));
				dialogue.Description = GetString(Object, TEXT("description"));
				dialogue.Slug = GetString(Object, TEXT("localizationSlug"));
				archive.Dialogues.Add(MoveTemp(dialogue));
			}
			else if (Table == TEXT("categories"))
			{
				FCategory category;
				category.Id = GetString(Object, TEXT("id"));
				category.Name = GetString(Object, TEXT("name"));
				category.ParentId = GetString(Object, TEXT("parentCategoryId"));
				archive.Categories.Add(MoveTemp(category));
			}
			else if (Table == TEXT("participants"))
			{
				FParticipant participant;
				participant.Id = GetString(Object, TEXT("id"));
				participant.Name = GetString(Object, TEXT("name"));
				participant.CategoryId = GetString(Object, TEXT("categoryId"));
				if (const TSharedPtr<FJsonObject> thumbnail = GetObject(Object, TEXT("thumbnail")))
				{
					participant.bHasThumbnail = true;
					participant.Thumbnail = ReadMediaMeta(thumbnail);
				}
				archive.Participants.Add(MoveTemp(participant));
			}
			else if (Table == TEXT("decorators"))
				ReadDefinition(Context, Object, archive.Decorators, Path);
			else if (Table == TEXT("conditions"))
				ReadDefinition(Context, Object, archive.Conditions, Path);
			else if (Table == TEXT("nodes"))
				ReadNode(Context, Object, Path);
			else if (Table == TEXT("edges"))
				ReadEdge(Context, Object, Path);
			else if (Table == TEXT("localizedStrings"))
				ReadLocalizedString(Context, Object, Path);
		}

		void ApplyMedia(FReadContext& Context, const TArray<TSharedPtr<FJsonValue>>& MediaItems)
		{
			TSet<FString> bindings;
			for (const TSharedPtr<FJsonValue>& itemValue : MediaItems)
			{
				const TSharedPtr<FJsonObject> item = itemValue.IsValid() ? itemValue->AsObject() : nullptr;
				if (!item.IsValid())
				{
					Context.Fail(TEXT("INVALID_ARCHIVE_MANIFEST"), TEXT("Media item is not an object."));
					continue;
				}

				const FString path = GetString(item, TEXT("path"));
				const FString participantId = GetString(item, TEXT("participantId"));
				const FString binding = participantId.IsEmpty()
					? FString::Printf(TEXT("audio|%s|%s|%s"), *GetString(item, TEXT("dialogueId")), *GetString(item, TEXT("nodeId")), *GetString(item, TEXT("rowId")))
					: FString::Printf(TEXT("thumbnail|%s"), *participantId);

				if (bindings.Contains(binding))
				{
					Context.Fail(TEXT("INVALID_ARCHIVE_MEDIA"), TEXT("Multiple media entries claim the same record."), path);
					continue;
				}
				bindings.Add(binding);

				const TArray<uint8>* bytes = Context.Entries.Find(path);
				if (!bytes || Context.UsedPaths.Contains(path))
				{
					Context.Fail(TEXT("INVALID_ARCHIVE_MEDIA"), TEXT("Media manifest reference is missing or duplicated."), path);
					continue;
				}
				Context.UsedPaths.Add(path);

				if (!participantId.IsEmpty())
				{
					FParticipant* participant = Context.Archive.Participants.FindByPredicate([&](const FParticipant& Candidate) { return Candidate.Id == participantId; });
					if (!participant || !participant->bHasThumbnail)
					{
						Context.Fail(TEXT("INVALID_ARCHIVE_MEDIA"), TEXT("Thumbnail manifest reference has no matching participant thumbnail."), path);
						continue;
					}
					participant->Thumbnail.Bytes = *bytes;
					continue;
				}

				const FString dialogueId = GetString(item, TEXT("dialogueId"));
				const FString nodeId = GetString(item, TEXT("nodeId"));
				const FString rowId = GetString(item, TEXT("rowId"));
				FNode* node = Context.Archive.Nodes.FindByPredicate([&](const FNode& Candidate) { return Candidate.DialogueId == dialogueId && Candidate.Id == nodeId; });
				FRow* row = node ? node->Rows.FindByPredicate([&](const FRow& Candidate) { return Candidate.Id == rowId; }) : nullptr;
				if (!row || !row->bHasAudio)
				{
					Context.Fail(TEXT("INVALID_ARCHIVE_MEDIA"), TEXT("Audio manifest reference has no matching row audioFile."), path);
					continue;
				}
				row->Audio.Bytes = *bytes;
			}
		}
	}

	// ---------------------------------------------------------------------------------------------
	// Validation (shared by read and write)
	// ---------------------------------------------------------------------------------------------

	void ValidateArchive(const FDialogueArchive& Archive, TArray<FString>& OutErrors)
	{
		auto fail = [&OutErrors](const FString& Code, const FString& Message, const FString& where = FString())
		{
			OutErrors.Add(FString::Printf(TEXT("%s: %s"), *Code, *Describe(where, Message)));
		};

		if (Archive.Project.Id.IsEmpty())
			fail(TEXT("INVALID_PROJECT"), TEXT("Project has no id."));

		const FLocalizationConfig& localization = Archive.Project.Localization;
		if (localization.DefaultLocale.IsEmpty())
			fail(TEXT("INVALID_LOCALIZATION"), TEXT("Project has no default locale."));

		if (Archive.Dialogues.Num() > Contract::MaxDialogues)
			fail(TEXT("ARCHIVE_DIALOGUE_LIMIT"), TEXT("Too many dialogues for one archive."));

		// Identity uniqueness.
		TSet<FString> dialogueIds;
		for (const FDialogue& dialogue : Archive.Dialogues)
		{
			if (dialogue.Id.IsEmpty() || dialogueIds.Contains(dialogue.Id))
				fail(TEXT("INVALID_ID"), TEXT("Dialogue id is empty or duplicated."), dialogue.Id);
			dialogueIds.Add(dialogue.Id);
		}

		if (Archive.Kind == EKind::Dialogue && !dialogueIds.Contains(Archive.RootDialogueId))
			fail(TEXT("INVALID_ARCHIVE_MANIFEST"), TEXT("The archive root dialogue is missing."), Archive.RootDialogueId);

		// Categories: names, parents, cycles, depth.
		TSet<FString> categoryIds;
		for (const FCategory& category : Archive.Categories)
		{
			if (category.Id.IsEmpty() || categoryIds.Contains(category.Id))
				fail(TEXT("INVALID_ID"), TEXT("Category id is empty or duplicated."), category.Id);
			categoryIds.Add(category.Id);
			if (!Contract::IsValidName(category.Name))
				fail(TEXT("INVALID_CATEGORY_NAME"), FString::Printf(TEXT("Category name '%s' must be %s."), *category.Name, *Contract::NameRuleDescription()), category.Id);
		}
		for (const FCategory& category : Archive.Categories)
		{
			int32 depth = 1;
			TSet<FString> seen;
			const FCategory* cursor = &category;
			while (cursor && !cursor->ParentId.IsEmpty())
			{
				if (seen.Contains(cursor->Id))
				{
					fail(TEXT("INVALID_CATEGORY"), TEXT("Category parents form a cycle."), category.Id);
					break;
				}
				seen.Add(cursor->Id);
				cursor = Archive.FindCategory(cursor->ParentId);
				if (!cursor)
				{
					fail(TEXT("MISSING_CATEGORY"), TEXT("Category parent does not exist."), category.Id);
					break;
				}
				++depth;
			}
			if (depth > 5)
				fail(TEXT("INVALID_CATEGORY"), TEXT("Category depth exceeds 5."), category.Id);
		}

		// Participants.
		TSet<FString> participantIds;
		for (const FParticipant& participant : Archive.Participants)
		{
			if (participant.Id.IsEmpty() || participantIds.Contains(participant.Id))
				fail(TEXT("INVALID_ID"), TEXT("Participant id is empty or duplicated."), participant.Id);
			participantIds.Add(participant.Id);
			if (!Contract::IsValidName(participant.Name))
				fail(TEXT("INVALID_PARTICIPANT_NAME"), FString::Printf(TEXT("Participant name '%s' must be %s."), *participant.Name, *Contract::NameRuleDescription()), participant.Id);
			if (!categoryIds.Contains(participant.CategoryId))
				fail(TEXT("MISSING_CATEGORY"), FString::Printf(TEXT("Participant '%s' has an unknown category."), *participant.Name), participant.Id);
			if (participant.bHasThumbnail)
			{
				if (!participant.Thumbnail.IsSet())
					fail(TEXT("MISSING_MEDIA"), FString::Printf(TEXT("Thumbnail bytes of participant '%s' are missing."), *participant.Name), participant.Id);
				else if (participant.Thumbnail.DeclaredSize >= 0 && participant.Thumbnail.DeclaredSize != participant.Thumbnail.Bytes.Num())
					fail(TEXT("MEDIA_SIZE_MISMATCH"), FString::Printf(TEXT("Thumbnail size of participant '%s' differs from its metadata."), *participant.Name), participant.Id);
			}
		}

		// Definitions.
		auto validateDefinitions = [&](const TArray<FDefinition>& Definitions, const TCHAR* Label)
		{
			TSet<FString> ids;
			for (const FDefinition& definition : Definitions)
			{
				if (definition.Id.IsEmpty() || ids.Contains(definition.Id))
					fail(TEXT("INVALID_ID"), FString::Printf(TEXT("%s definition id is empty or duplicated."), Label), definition.Id);
				ids.Add(definition.Id);
				if (definition.Name.IsEmpty())
					fail(TEXT("INVALID_DEFINITION"), FString::Printf(TEXT("%s definition has no name."), Label), definition.Id);

				TSet<FString> propertyNames;
				for (const FPropertyDef& property : definition.Properties)
				{
					if (property.Name.IsEmpty() || propertyNames.Contains(property.Name))
						fail(TEXT("INVALID_DEFINITION"), TEXT("Property name is empty or duplicated."), definition.Name);
					propertyNames.Add(property.Name);
					const bool bKnownType = property.Type == Contract::PropertyString || property.Type == Contract::PropertyNumber || property.Type == Contract::PropertyBoolean;
					if (!bKnownType)
						fail(TEXT("INVALID_DEFINITION"), FString::Printf(TEXT("Property '%s' has unsupported type '%s'."), *property.Name, *property.Type), definition.Name);
				}
			}
		};
		validateDefinitions(Archive.Decorators, TEXT("Decorator"));
		validateDefinitions(Archive.Conditions, TEXT("Condition"));

		// Nodes.
		TSet<FString> nodeKeys;
		TMap<FString, int32> startNodesPerDialogue;
		for (const FNode& node : Archive.Nodes)
		{
			const FString where = FString::Printf(TEXT("node %s"), *node.Id);
			const FString nodeKey = node.DialogueId + TEXT("|") + node.Id;
			if (node.Id.IsEmpty() || nodeKeys.Contains(nodeKey))
				fail(TEXT("INVALID_ID"), TEXT("Node id is empty or duplicated within its dialogue."), where);
			nodeKeys.Add(nodeKey);

			if (!dialogueIds.Contains(node.DialogueId))
				fail(TEXT("MISSING_DIALOGUE"), TEXT("Node refers to an unknown dialogue."), where);
			if (!Contract::IsKnownNodeType(node.Type))
				fail(TEXT("UNSUPPORTED_NODE_TYPE"), FString::Printf(TEXT("Unsupported node type '%s'."), *node.Type), where);

			if (node.Type == Contract::NodeStart)
			{
				startNodesPerDialogue.FindOrAdd(node.DialogueId)++;
				if (node.Id != Contract::StartNodeId)
					fail(TEXT("INVALID_START"), FString::Printf(TEXT("Start node id must be %s."), Contract::StartNodeId), where);
			}

			if (!node.ParticipantId.IsEmpty() && !participantIds.Contains(node.ParticipantId))
				fail(TEXT("MISSING_PARTICIPANT"), TEXT("Node refers to an unknown participant."), where);
			if (!node.TargetNode.IsEmpty() && !Archive.FindNode(node.DialogueId, node.TargetNode))
				fail(TEXT("MISSING_TARGET"), TEXT("Return target node does not exist in the same dialogue."), where);
			if (!node.TargetDialogue.IsEmpty() && !dialogueIds.Contains(node.TargetDialogue))
				fail(TEXT("MISSING_TARGET"), TEXT("Child dialogue is not part of the archive."), where);

			for (const FDecoratorInstance& instance : node.Decorators)
			{
				if (!Archive.FindDecorator(instance.DefinitionId))
					fail(TEXT("MISSING_DEFINITION"), FString::Printf(TEXT("Decorator '%s' has no definition."), *instance.Name), where);
			}

			auto requireKey = [&](const FString& Key, const FString& What)
			{
				if (Key.IsEmpty())
					return;
				const FLocalizedString* entry = Archive.FindString(Key);
				if (!entry)
					fail(TEXT("MISSING_STRING"), FString::Printf(TEXT("%s key '%s' has no localized string."), *What, *Key), where);
				else if (!entry->Values.Contains(localization.DefaultLocale))
					fail(TEXT("MISSING_STRING"), FString::Printf(TEXT("%s key '%s' has no value for default locale '%s'."), *What, *Key, *localization.DefaultLocale), where);
			};
			requireKey(node.DisplayNameKey, TEXT("displayName"));
			requireKey(node.SelectionTitleKey, TEXT("selectionTitle"));

			TSet<FString> rowIds;
			for (const FRow& row : node.Rows)
			{
				if (row.Id.IsEmpty() || rowIds.Contains(row.Id))
					fail(TEXT("INVALID_ID"), TEXT("Row id is empty or duplicated within its node."), where);
				rowIds.Add(row.Id);

				if (row.TextKey.IsEmpty())
					fail(TEXT("MISSING_STRING"), TEXT("Row has no textKey."), where);
				requireKey(row.TextKey, TEXT("row text"));

				if (row.DurationOverride < 0.f)
					fail(TEXT("INVALID_ROW_TIMING"), TEXT("durationOverride must be >= 0."), where);
				if (!row.ParticipantId.IsEmpty() && !participantIds.Contains(row.ParticipantId))
					fail(TEXT("MISSING_PARTICIPANT"), TEXT("Row refers to an unknown participant."), where);

				if (row.bHasAudio)
				{
					if (!row.Audio.IsSet())
						fail(TEXT("MISSING_MEDIA"), TEXT("Row audio bytes are missing."), where);
					else if (row.Audio.DeclaredSize >= 0 && row.Audio.DeclaredSize != row.Audio.Bytes.Num())
						fail(TEXT("MEDIA_SIZE_MISMATCH"), TEXT("Row audio size differs from its metadata."), where);
				}
			}
		}

		for (const FDialogue& dialogue : Archive.Dialogues)
		{
			const int32* startCount = startNodesPerDialogue.Find(dialogue.Id);
			if (!startCount || *startCount != 1)
				fail(TEXT("INVALID_START"), TEXT("A dialogue needs exactly one start node."), dialogue.Id);
		}

		// Edges.
		TSet<FString> edgeKeys;
		for (const FArchiveEdge& edge : Archive.Edges)
		{
			const FString where = FString::Printf(TEXT("edge %s"), *edge.Id);
			const FString edgeKey = edge.DialogueId + TEXT("|") + edge.Id;
			if (edge.Id.IsEmpty() || edgeKeys.Contains(edgeKey))
				fail(TEXT("INVALID_ID"), TEXT("Edge id is empty or duplicated within its dialogue."), where);
			edgeKeys.Add(edgeKey);

			if (!Archive.FindNode(edge.DialogueId, edge.SourceId) || !Archive.FindNode(edge.DialogueId, edge.TargetId))
				fail(TEXT("MISSING_ENDPOINT"), TEXT("Edge endpoint does not exist in its dialogue."), where);

			if (!edge.Rules.IsEmpty() && edge.ConditionMode != TEXT("all") && edge.ConditionMode != TEXT("any"))
				fail(TEXT("INVALID_CONDITION"), TEXT("Condition mode must be 'all' or 'any'."), where);
			for (const FConditionRule& rule : edge.Rules)
			{
				if (!Archive.FindCondition(rule.DefinitionId))
					fail(TEXT("MISSING_DEFINITION"), FString::Printf(TEXT("Condition '%s' has no definition."), *rule.Name), where);
			}
		}

		TSet<FString> stringKeys;
		for (const FLocalizedString& entry : Archive.Strings)
		{
			if (entry.Key.IsEmpty() || stringKeys.Contains(entry.Key))
				fail(TEXT("INVALID_ID"), TEXT("Localized string key is empty or duplicated."), entry.Key);
			stringKeys.Add(entry.Key);
		}
	}

	// ---------------------------------------------------------------------------------------------
	// ReadArchive
	// ---------------------------------------------------------------------------------------------

	bool ReadArchive(const FEntries& Entries, FDialogueArchive& OutArchive, TArray<FString>& OutErrors)
	{
		OutArchive = FDialogueArchive();
		const int32 errorsBefore = OutErrors.Num();

		if (!Entries.Contains(TEXT("manifest.json")))
		{
			const bool bLooksFlat = Entries.Contains(TEXT("dialogueData.json")) || Entries.Contains(TEXT("projectData.json")) || Entries.Contains(TEXT("nodes.json"));
			OutErrors.Add(bLooksFlat
				? TEXT("LEGACY_FLAT_ARCHIVE: This file uses the old flat layout (no manifest.json), which is no longer supported. Re-export it from the current Mountea Dialoguer.")
				: TEXT("INVALID_ARCHIVE: manifest.json is missing, this is not a Mountea Dialoguer archive."));
			return false;
		}

		FReadContext context(Entries, OutArchive, OutErrors);

		if (Entries.Num() > Contract::MaxEntries)
			context.Fail(TEXT("ARCHIVE_ENTRY_LIMIT"), TEXT("Archive exceeds the entry count limit."));

		TSharedPtr<FJsonObject> manifest;
		if (!context.LoadJson(TEXT("manifest.json"), manifest))
			return false;
		context.UsedPaths.Add(TEXT("manifest.json"));

		if (GetString(manifest, TEXT("format")) != Contract::Format)
		{
			context.Fail(TEXT("UNSUPPORTED_ARCHIVE"), FString::Printf(TEXT("Unsupported archive format '%s', expected '%s'."), *GetString(manifest, TEXT("format")), Contract::Format));
			return false;
		}

		const FString kind = GetString(manifest, TEXT("kind"));
		if (kind == Contract::KindProject)
			OutArchive.Kind = EKind::Project;
		else if (kind == Contract::KindDialogue)
			OutArchive.Kind = EKind::Dialogue;
		else
		{
			context.Fail(TEXT("INVALID_ARCHIVE_MANIFEST"), TEXT("Archive kind must be project or dialogue."));
			return false;
		}
		OutArchive.RootDialogueId = GetString(manifest, TEXT("dialogueId"));

		const TArray<TSharedPtr<FJsonValue>>* records = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* media = nullptr;
		if (!manifest->TryGetArrayField(TEXT("records"), records) || !manifest->TryGetArrayField(TEXT("media"), media) || !records || !media)
		{
			context.Fail(TEXT("INVALID_ARCHIVE_MANIFEST"), TEXT("Archive manifest lists are invalid."));
			return false;
		}

		const FString projectPath = GetString(manifest, TEXT("project"));
		TSharedPtr<FJsonObject> projectObject;
		if (context.LoadJson(projectPath, projectObject))
		{
			context.UsedPaths.Add(projectPath);
			OutArchive.Project.Id = GetString(projectObject, TEXT("id"));
			OutArchive.Project.Name = GetString(projectObject, TEXT("name"));
			OutArchive.Project.Description = GetString(projectObject, TEXT("description"));
			if (const TSharedPtr<FJsonObject> localization = GetObject(projectObject, TEXT("localization")))
			{
				const FString defaultLocale = GetString(localization, TEXT("defaultLocale"));
				if (!defaultLocale.IsEmpty())
					OutArchive.Project.Localization.DefaultLocale = defaultLocale;
				const TArray<TSharedPtr<FJsonValue>>* supported = nullptr;
				if (localization->TryGetArrayField(TEXT("supportedLocales"), supported) && supported)
				{
					for (const TSharedPtr<FJsonValue>& localeValue : *supported)
					{
						FString locale;
						if (localeValue.IsValid() && localeValue->TryGetString(locale))
							OutArchive.Project.Localization.SupportedLocales.AddUnique(locale);
					}
				}
			}
			OutArchive.Project.Localization.SupportedLocales.AddUnique(OutArchive.Project.Localization.DefaultLocale);
		}

		for (const TSharedPtr<FJsonValue>& recordValue : *records)
		{
			const TSharedPtr<FJsonObject> item = recordValue.IsValid() ? recordValue->AsObject() : nullptr;
			const FString table = GetString(item, TEXT("table"));
			const FString path = GetString(item, TEXT("path"));
			static const TCHAR* const tables[] = { TEXT("dialogues"), TEXT("participants"), TEXT("categories"), TEXT("decorators"), TEXT("conditions"), TEXT("nodes"), TEXT("edges"), TEXT("localizedStrings") };
			bool bKnownTable = false;
			for (const TCHAR* knownTable : tables)
				bKnownTable |= (table == knownTable);

			if (!bKnownTable || context.UsedPaths.Contains(path))
			{
				context.Fail(TEXT("INVALID_ARCHIVE_MANIFEST"), TEXT("Duplicate or unsupported manifest record."), path);
				continue;
			}
			context.UsedPaths.Add(path);

			TSharedPtr<FJsonObject> recordObject;
			if (context.LoadJson(path, recordObject))
				ReadRecord(context, table, recordObject, path);
		}

		ApplyMedia(context, *media);

		for (const TPair<FString, TArray<uint8>>& entry : Entries)
		{
			if (!context.UsedPaths.Contains(entry.Key))
				context.Fail(TEXT("INVALID_ARCHIVE_MANIFEST"), TEXT("Archive has an entry not declared by its manifest."), entry.Key);
		}

		ValidateArchive(OutArchive, OutErrors);
		return OutErrors.Num() == errorsBefore;
	}

	// ---------------------------------------------------------------------------------------------
	// WriteArchive
	// ---------------------------------------------------------------------------------------------

	namespace
	{
		TSharedPtr<FJsonValue> StringValue(const FString& Text) { return MakeShared<FJsonValueString>(Text); }
		TSharedPtr<FJsonValue> NumberValue(const double Number) { return MakeShared<FJsonValueNumber>(Number); }
		TSharedPtr<FJsonValue> ObjectValue(const TSharedRef<FJsonObject>& Object) { return MakeShared<FJsonValueObject>(Object); }

		void SetOptionalString(const TSharedRef<FJsonObject>& Object, const TCHAR* Field, const FString& Value)
		{
			if (!Value.IsEmpty())
				Object->SetStringField(Field, Value);
		}

		TSharedRef<FJsonObject> MediaToJson(const FMedia& Media)
		{
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			SetOptionalString(json, TEXT("name"), Media.Name);
			json->SetStringField(TEXT("mimeType"), Media.MimeType.IsEmpty() ? FString(TEXT("application/octet-stream")) : Media.MimeType);
			json->SetNumberField(TEXT("size"), Media.Bytes.Num());
			if (Media.Width > 0)
				json->SetNumberField(TEXT("width"), Media.Width);
			if (Media.Height > 0)
				json->SetNumberField(TEXT("height"), Media.Height);
			return json;
		}

		TSharedPtr<FJsonValue> TypedDefault(const FPropertyDef& Property)
		{
			if (Property.Type == Contract::PropertyNumber)
				return NumberValue(FCString::Atod(*Property.DefaultValue));
			if (Property.Type == Contract::PropertyBoolean)
				return MakeShared<FJsonValueBoolean>(Property.DefaultValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Property.DefaultValue == TEXT("1"));
			return StringValue(Property.DefaultValue);
		}

		TSharedRef<FJsonObject> DefinitionToJson(const FDefinition& Definition, const FString& ProjectId)
		{
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("id"), Definition.Id);
			json->SetStringField(TEXT("projectId"), ProjectId);
			json->SetStringField(TEXT("name"), Definition.Name);
			SetOptionalString(json, TEXT("type"), Definition.Type);
			TArray<TSharedPtr<FJsonValue>> properties;
			for (const FPropertyDef& property : Definition.Properties)
			{
				const TSharedRef<FJsonObject> propertyJson = MakeShared<FJsonObject>();
				propertyJson->SetStringField(TEXT("name"), property.Name);
				propertyJson->SetStringField(TEXT("type"), property.Type);
				propertyJson->SetField(TEXT("defaultValue"), TypedDefault(property));
				properties.Add(ObjectValue(propertyJson));
			}
			json->SetArrayField(TEXT("properties"), properties);
			return json;
		}

		TSharedRef<FJsonObject> NodeToJson(const FNode& Node)
		{
			const TSharedRef<FJsonObject> data = MakeShared<FJsonObject>();
			SetOptionalString(data, TEXT("label"), Node.Label);
			SetOptionalString(data, TEXT("localizationNodeToken"), Node.NodeToken);
			SetOptionalString(data, TEXT("displayNameKey"), Node.DisplayNameKey);
			SetOptionalString(data, TEXT("selectionTitleKey"), Node.SelectionTitleKey);
			SetOptionalString(data, TEXT("participant"), Node.ParticipantName);
			SetOptionalString(data, TEXT("participantId"), Node.ParticipantId);
			SetOptionalString(data, TEXT("targetNode"), Node.TargetNode);
			SetOptionalString(data, TEXT("targetDialogue"), Node.TargetDialogue);
			if (Node.bHasDuration)
				data->SetNumberField(TEXT("duration"), Node.Duration);

			if (!Node.Decorators.IsEmpty())
			{
				TArray<TSharedPtr<FJsonValue>> decorators;
				for (const FDecoratorInstance& instance : Node.Decorators)
				{
					const TSharedRef<FJsonObject> instanceJson = MakeShared<FJsonObject>();
					instanceJson->SetStringField(TEXT("id"), instance.DefinitionId);
					instanceJson->SetStringField(TEXT("name"), instance.Name);
					instanceJson->SetObjectField(TEXT("values"), instance.Values.IsValid() ? instance.Values : MakeShared<FJsonObject>());
					decorators.Add(ObjectValue(instanceJson));
				}
				data->SetArrayField(TEXT("decorators"), decorators);
			}

			if (!Node.Rows.IsEmpty())
			{
				TArray<TSharedPtr<FJsonValue>> rows;
				for (const FRow& row : Node.Rows)
				{
					const TSharedRef<FJsonObject> rowJson = MakeShared<FJsonObject>();
					rowJson->SetStringField(TEXT("id"), row.Id);
					rowJson->SetStringField(TEXT("textKey"), row.TextKey);
					SetOptionalString(rowJson, TEXT("localizationRowToken"), row.RowToken);
					rowJson->SetNumberField(TEXT("duration"), row.Duration);
					SetOptionalString(rowJson, TEXT("participantId"), row.ParticipantId);
					if (row.ExecutionBehaviour != ERowExecutionMode::EREM_Automatic)
						rowJson->SetStringField(TEXT("executionBehaviour"), Contract::ExecutionBehaviourToString(row.ExecutionBehaviour));
					if (row.DurationMode != ERowDurationMode::ERDM_Duration)
						rowJson->SetStringField(TEXT("durationMode"), Contract::DurationModeToString(row.DurationMode));
					if (!FMath::IsNearlyZero(row.DurationOverride))
						rowJson->SetNumberField(TEXT("durationOverride"), row.DurationOverride);
					if (row.bHasAudio)
						rowJson->SetObjectField(TEXT("audioFile"), MediaToJson(row.Audio));
					rows.Add(ObjectValue(rowJson));
				}
				data->SetArrayField(TEXT("dialogueRows"), rows);
			}

			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("id"), Node.Id);
			json->SetStringField(TEXT("dialogueId"), Node.DialogueId);
			json->SetStringField(TEXT("type"), Node.Type);
			const TSharedRef<FJsonObject> position = MakeShared<FJsonObject>();
			position->SetNumberField(TEXT("x"), Node.PosX);
			position->SetNumberField(TEXT("y"), Node.PosY);
			json->SetObjectField(TEXT("position"), position);
			json->SetObjectField(TEXT("data"), data);
			return json;
		}

		TSharedRef<FJsonObject> EdgeToJson(const FArchiveEdge& Edge)
		{
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("id"), Edge.Id);
			json->SetStringField(TEXT("dialogueId"), Edge.DialogueId);
			json->SetStringField(TEXT("source"), Edge.SourceId);
			json->SetStringField(TEXT("target"), Edge.TargetId);
			json->SetStringField(TEXT("type"), TEXT("conditionEdge"));

			const TSharedRef<FJsonObject> data = MakeShared<FJsonObject>();
			if (!Edge.Rules.IsEmpty())
			{
				const TSharedRef<FJsonObject> conditions = MakeShared<FJsonObject>();
				conditions->SetStringField(TEXT("mode"), Edge.ConditionMode);
				TArray<TSharedPtr<FJsonValue>> rules;
				for (const FConditionRule& rule : Edge.Rules)
				{
					const TSharedRef<FJsonObject> ruleJson = MakeShared<FJsonObject>();
					ruleJson->SetStringField(TEXT("id"), rule.DefinitionId);
					ruleJson->SetStringField(TEXT("name"), rule.Name);
					ruleJson->SetBoolField(TEXT("negate"), rule.bNegate);
					ruleJson->SetObjectField(TEXT("values"), rule.Values.IsValid() ? rule.Values : MakeShared<FJsonObject>());
					rules.Add(ObjectValue(ruleJson));
				}
				conditions->SetArrayField(TEXT("rules"), rules);
				data->SetObjectField(TEXT("conditions"), conditions);
			}
			json->SetObjectField(TEXT("data"), data);
			return json;
		}

		TSharedRef<FJsonObject> StringToJson(const FLocalizedString& Entry, const FString& ProjectId, const FString& Timestamp)
		{
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("projectId"), ProjectId);
			json->SetStringField(TEXT("key"), Entry.Key);
			json->SetStringField(TEXT("keyType"), TEXT("readable"));
			json->SetStringField(TEXT("dialogueId"), Entry.DialogueId);
			json->SetStringField(TEXT("nodeId"), Entry.NodeId);
			json->SetStringField(TEXT("rowId"), Entry.RowId);
			json->SetStringField(TEXT("field"), Entry.Field);
			SetOptionalString(json, TEXT("dialogueSlug"), Entry.DialogueSlug);
			SetOptionalString(json, TEXT("nodeToken"), Entry.NodeToken);
			SetOptionalString(json, TEXT("rowToken"), Entry.RowToken);
			const TSharedRef<FJsonObject> values = MakeShared<FJsonObject>();
			for (const TPair<FString, FString>& pair : Entry.Values)
				values->SetStringField(pair.Key, pair.Value);
			json->SetObjectField(TEXT("values"), values);
			json->SetStringField(TEXT("createdAt"), Timestamp);
			json->SetStringField(TEXT("modifiedAt"), Timestamp);
			return json;
		}
	}

	bool WriteArchive(const FDialogueArchive& Archive, FEntries& OutEntries, TArray<FString>& OutErrors)
	{
		const int32 errorsBefore = OutErrors.Num();
		ValidateArchive(Archive, OutErrors);
		if (OutErrors.Num() != errorsBefore)
			return false;

		OutEntries.Reset();
		const FString timestamp = FDateTime::UtcNow().ToIso8601();
		const FString projectId = Archive.Project.Id;

		TArray<TSharedPtr<FJsonValue>> manifestRecords;
		TArray<TSharedPtr<FJsonValue>> manifestMedia;
		int64 expandedBytes = 0;

		auto addEntry = [&](const FString& path, TArray<uint8>&& Bytes)
		{
			if (path.EndsWith(TEXT(".json")) && Bytes.Num() > Contract::MaxJsonEntryBytes)
				OutErrors.Add(FString::Printf(TEXT("ARCHIVE_JSON_LIMIT: A record exceeds the 5 MiB JSON entry limit (%s)."), *path));
			expandedBytes += Bytes.Num();
			OutEntries.Add(path, MoveTemp(Bytes));
		};

		auto addRecord = [&](const TCHAR* Table, const FString& Name, const TSharedRef<FJsonObject>& json)
		{
			const FString path = FString::Printf(TEXT("%s/%s--%d.json"), Table, *SanitizeFileName(Name), manifestRecords.Num());
			addEntry(path, SerializeJsonObject(json));
			const TSharedRef<FJsonObject> item = MakeShared<FJsonObject>();
			item->SetStringField(TEXT("table"), Table);
			item->SetStringField(TEXT("path"), path);
			manifestRecords.Add(ObjectValue(item));
		};

		auto addMedia = [&](const FString& path, const FMedia& Media, const TSharedRef<FJsonObject>& binding)
		{
			TArray<uint8> copy = Media.Bytes;
			addEntry(path, MoveTemp(copy));
			binding->SetStringField(TEXT("path"), path);
			manifestMedia.Add(ObjectValue(binding));
		};

		const TSharedRef<FJsonObject> projectJson = MakeShared<FJsonObject>();
		projectJson->SetStringField(TEXT("id"), projectId);
		projectJson->SetStringField(TEXT("name"), Archive.Project.Name);
		projectJson->SetStringField(TEXT("description"), Archive.Project.Description);
		projectJson->SetStringField(TEXT("createdAt"), timestamp);
		projectJson->SetStringField(TEXT("modifiedAt"), timestamp);
		{
			const TSharedRef<FJsonObject> localizationJson = MakeShared<FJsonObject>();
			localizationJson->SetStringField(TEXT("defaultLocale"), Archive.Project.Localization.DefaultLocale);
			TArray<TSharedPtr<FJsonValue>> supported;
			for (const FString& locale : Archive.Project.Localization.SupportedLocales)
				supported.Add(StringValue(locale));
			localizationJson->SetArrayField(TEXT("supportedLocales"), supported);
			projectJson->SetObjectField(TEXT("localization"), localizationJson);
		}
		addEntry(TEXT("project.json"), SerializeJsonObject(projectJson));

		for (const FDialogue& dialogue : Archive.Dialogues)
		{
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("id"), dialogue.Id);
			json->SetStringField(TEXT("projectId"), projectId);
			json->SetStringField(TEXT("name"), dialogue.Name);
			json->SetStringField(TEXT("description"), dialogue.Description);
			SetOptionalString(json, TEXT("localizationSlug"), dialogue.Slug);
			json->SetNumberField(TEXT("localizationVersion"), 2);
			json->SetStringField(TEXT("createdAt"), timestamp);
			json->SetStringField(TEXT("modifiedAt"), timestamp);
			addRecord(TEXT("dialogues"), dialogue.Name, json);
		}

		for (const FParticipant& participant : Archive.Participants)
		{
			const FCategory* category = Archive.FindCategory(participant.CategoryId);
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("id"), participant.Id);
			json->SetStringField(TEXT("projectId"), projectId);
			json->SetStringField(TEXT("name"), participant.Name);
			json->SetStringField(TEXT("category"), category ? category->Name : FString());
			json->SetStringField(TEXT("categoryId"), participant.CategoryId);
			if (participant.bHasThumbnail)
				json->SetObjectField(TEXT("thumbnail"), MediaToJson(participant.Thumbnail));
			addRecord(TEXT("participants"), participant.Name, json);

			if (participant.bHasThumbnail)
			{
				const TSharedRef<FJsonObject> binding = MakeShared<FJsonObject>();
				binding->SetStringField(TEXT("participantId"), participant.Id);
				addMedia(FString::Printf(TEXT("media/thumbnails/%s"), *SanitizeFileName(participant.Id)), participant.Thumbnail, binding);
			}
		}

		for (const FCategory& category : Archive.Categories)
		{
			const TSharedRef<FJsonObject> json = MakeShared<FJsonObject>();
			json->SetStringField(TEXT("id"), category.Id);
			json->SetStringField(TEXT("projectId"), projectId);
			json->SetStringField(TEXT("name"), category.Name);
			if (category.ParentId.IsEmpty())
				json->SetField(TEXT("parentCategoryId"), MakeShared<FJsonValueNull>());
			else
				json->SetStringField(TEXT("parentCategoryId"), category.ParentId);
			addRecord(TEXT("categories"), category.Name, json);
		}

		for (const FDefinition& definition : Archive.Decorators)
			addRecord(TEXT("decorators"), definition.Name, DefinitionToJson(definition, projectId));
		for (const FDefinition& definition : Archive.Conditions)
			addRecord(TEXT("conditions"), definition.Name, DefinitionToJson(definition, projectId));

		for (const FNode& node : Archive.Nodes)
		{
			addRecord(TEXT("nodes"), node.Type, NodeToJson(node));
			for (const FRow& row : node.Rows)
			{
				if (!row.bHasAudio)
					continue;
				const TSharedRef<FJsonObject> binding = MakeShared<FJsonObject>();
				binding->SetStringField(TEXT("dialogueId"), node.DialogueId);
				binding->SetStringField(TEXT("nodeId"), node.Id);
				binding->SetStringField(TEXT("rowId"), row.Id);
				addMedia(FString::Printf(TEXT("media/%d--%s"), manifestMedia.Num(), *SanitizeFileName(row.Audio.Name)), row.Audio, binding);
			}
		}

		for (const FArchiveEdge& edge : Archive.Edges)
			addRecord(TEXT("edges"), TEXT("edge"), EdgeToJson(edge));
		for (const FLocalizedString& entry : Archive.Strings)
			addRecord(TEXT("localizedStrings"), entry.Key, StringToJson(entry, projectId, timestamp));

		const TSharedRef<FJsonObject> manifest = MakeShared<FJsonObject>();
		manifest->SetStringField(TEXT("format"), Contract::Format);
		manifest->SetStringField(TEXT("kind"), Archive.Kind == EKind::Project ? Contract::KindProject : Contract::KindDialogue);
		if (Archive.Kind == EKind::Dialogue)
			manifest->SetStringField(TEXT("dialogueId"), Archive.RootDialogueId);
		manifest->SetStringField(TEXT("project"), TEXT("project.json"));
		manifest->SetArrayField(TEXT("records"), manifestRecords);
		manifest->SetArrayField(TEXT("media"), manifestMedia);
		addEntry(TEXT("manifest.json"), SerializeJsonObject(manifest));

		if (OutEntries.Num() > Contract::MaxEntries)
			OutErrors.Add(FString::Printf(TEXT("ARCHIVE_ENTRY_LIMIT: Archive would contain %d entries, the limit is %d."), OutEntries.Num(), Contract::MaxEntries));
		if (expandedBytes > Contract::MaxExpandedBytes)
			OutErrors.Add(TEXT("ARCHIVE_EXPANSION_LIMIT: Archive content exceeds the supported expanded size."));

		if (OutErrors.Num() != errorsBefore)
		{
			OutEntries.Reset();
			return false;
		}
		return true;
	}
}
