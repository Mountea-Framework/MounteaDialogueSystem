// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueImportConfig.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace
{
	FString GuidToKey(const FGuid& G) { return G.ToString(EGuidFormats::Digits); }

	bool KeyToGuid(const FString& Key, FGuid& OutGuid) { return FGuid::Parse(Key, OutGuid); }

	FString DateTimeToString(const FDateTime& DT)
	{
		return FString::Printf(TEXT("%lld"), DT.GetTicks());
	}

	FDateTime StringToDateTime(const FString& Str)
	{
		int64 ticks = FCString::Atoi64(*Str);
		return ticks > 0 ? FDateTime(ticks) : FDateTime::MinValue();
	}
}

// ── Constructor ───────────────────────────────────────────────────────────────

UMounteaDialogueImportConfig::UMounteaDialogueImportConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CategoryName = TEXT("Mountea Framework");
	SectionName  = TEXT("Mountea Dialogue System (Import)");

	LoadFromFile();
}

// ── Dialogue helpers ──────────────────────────────────────────────────────────

bool UMounteaDialogueImportConfig::IsDialogueImported(const FGuid& Guid) const
{
	return DialogueHistory.Contains(Guid);
}

bool UMounteaDialogueImportConfig::LookupDialogueAssetPath(const FGuid& Guid, FString& OutAssetPath) const
{
	if (const FDialogueImportRecord* rec = DialogueHistory.Find(Guid))
	{
		OutAssetPath = rec->AssetPath;
		return !OutAssetPath.IsEmpty();
	}
	return false;
}

void UMounteaDialogueImportConfig::RecordDialogueImport(const FGuid& Guid, const FString& AssetPath,
	const FString& SourcePath, bool bIsProjectImport)
{
	FDialogueImportRecord& rec = DialogueHistory.FindOrAdd(Guid);
	rec.AssetPath       = AssetPath;
	rec.SourcePath      = SourcePath;
	rec.ImportedAt      = FDateTime::Now();
	rec.bIsProjectImport = bIsProjectImport;
}

// ── Project helpers ───────────────────────────────────────────────────────────

bool UMounteaDialogueImportConfig::IsProjectImported(const FGuid& ProjectGuid) const
{
	return ProjectHistory.Contains(ProjectGuid);
}

bool UMounteaDialogueImportConfig::LookupProjectFolder(const FGuid& ProjectGuid, FString& OutProjectFolder) const
{
	const FProjectImportRecord* rec = ProjectHistory.Find(ProjectGuid);
	if (!rec || rec->ProjectFolder.IsEmpty())
		return false;

	// Validate that at least one member dialogue asset still exists in the registry.
	FAssetRegistryModule& reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	for (const FGuid& dlgGuid : rec->DialogueGuids)
	{
		const FDialogueImportRecord* dlg = DialogueHistory.Find(dlgGuid);
		if (!dlg || dlg->AssetPath.IsEmpty())
			continue;

		const FString objectPath = FString::Printf(TEXT("%s.%s"),
			*dlg->AssetPath, *FPaths::GetBaseFilename(dlg->AssetPath));
		if (reg.Get().GetAssetByObjectPath(FSoftObjectPath(objectPath)).IsValid())
		{
			OutProjectFolder = rec->ProjectFolder;
			return true;
		}
	}
	return false;
}

void UMounteaDialogueImportConfig::RecordProjectImport(const FGuid& ProjectGuid,
	const FString& ProjectName, const FString& SourcePath, const FString& ProjectFolder,
	const TArray<FGuid>& DialogueGuids)
{
	FProjectImportRecord& rec = ProjectHistory.FindOrAdd(ProjectGuid);
	rec.ProjectName   = ProjectName;
	rec.SourcePath    = SourcePath;
	rec.ProjectFolder = ProjectFolder;
	rec.ImportedAt    = FDateTime::Now();
	rec.DialogueGuids = DialogueGuids;
}

// ── Persistence ───────────────────────────────────────────────────────────────

void UMounteaDialogueImportConfig::SaveToFile()
{
	const FString filePath = GetImportConfigFilePath();
	IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*FPaths::GetPath(filePath));

	TSharedRef<FJsonObject> root = MakeShared<FJsonObject>();

	// ── DialogueHistory ───────────────────────────────────────────────────────
	{
		TSharedRef<FJsonObject> histObj = MakeShared<FJsonObject>();
		for (const auto& pair : DialogueHistory)
		{
			TSharedRef<FJsonObject> entry = MakeShared<FJsonObject>();
			entry->SetStringField(TEXT("assetPath"),       pair.Value.AssetPath);
			entry->SetStringField(TEXT("sourcePath"),      pair.Value.SourcePath);
			entry->SetStringField(TEXT("importedAt"),      DateTimeToString(pair.Value.ImportedAt));
			entry->SetBoolField  (TEXT("isProjectImport"), pair.Value.bIsProjectImport);
			histObj->SetObjectField(GuidToKey(pair.Key), entry);
		}
		root->SetObjectField(TEXT("dialogueHistory"), histObj);
	}

	// ── ProjectHistory ────────────────────────────────────────────────────────
	{
		TSharedRef<FJsonObject> projObj = MakeShared<FJsonObject>();
		for (const auto& pair : ProjectHistory)
		{
			TSharedRef<FJsonObject> entry = MakeShared<FJsonObject>();
			entry->SetStringField(TEXT("projectName"),   pair.Value.ProjectName);
			entry->SetStringField(TEXT("sourcePath"),    pair.Value.SourcePath);
			entry->SetStringField(TEXT("projectFolder"), pair.Value.ProjectFolder);
			entry->SetStringField(TEXT("importedAt"),    DateTimeToString(pair.Value.ImportedAt));

			TArray<TSharedPtr<FJsonValue>> guidsArray;
			for (const FGuid& g : pair.Value.DialogueGuids)
				guidsArray.Add(MakeShared<FJsonValueString>(GuidToKey(g)));
			entry->SetArrayField(TEXT("dialogueGuids"), guidsArray);

			projObj->SetObjectField(GuidToKey(pair.Key), entry);
		}
		root->SetObjectField(TEXT("projectHistory"), projObj);
	}

	FString output;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&output);
	FJsonSerializer::Serialize(root, writer);
	FFileHelper::SaveStringToFile(output, *filePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

void UMounteaDialogueImportConfig::LoadFromFile()
{
	const FString filePath = GetImportConfigFilePath();
	if (!FPaths::FileExists(filePath))
		return;

	FString content;
	if (!FFileHelper::LoadFileToString(content, *filePath))
		return;

	TSharedPtr<FJsonObject> root;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(content);
	if (!FJsonSerializer::Deserialize(reader, root) || !root.IsValid())
		return;

	DialogueHistory.Reset();
	ProjectHistory.Reset();

	// ── DialogueHistory ───────────────────────────────────────────────────────
	const TSharedPtr<FJsonObject>* dlgHistField = nullptr;
	if (root->TryGetObjectField(TEXT("dialogueHistory"), dlgHistField))
	{
		for (const auto& field : (*dlgHistField)->Values)
		{
			FGuid guid;
			if (!KeyToGuid(field.Key, guid)) continue;
			const TSharedPtr<FJsonObject> entry = field.Value->AsObject();
			if (!entry.IsValid()) continue;

			FDialogueImportRecord rec;
			entry->TryGetStringField(TEXT("assetPath"),  rec.AssetPath);
			entry->TryGetStringField(TEXT("sourcePath"), rec.SourcePath);
			FString dtStr; entry->TryGetStringField(TEXT("importedAt"), dtStr);
			rec.ImportedAt = StringToDateTime(dtStr);
			entry->TryGetBoolField(TEXT("isProjectImport"), rec.bIsProjectImport);
			DialogueHistory.Add(guid, MoveTemp(rec));
		}
	}

	// ── ProjectHistory ────────────────────────────────────────────────────────
	const TSharedPtr<FJsonObject>* projHistField = nullptr;
	if (root->TryGetObjectField(TEXT("projectHistory"), projHistField))
	{
		for (const auto& field : (*projHistField)->Values)
		{
			FGuid projectGuid;
			if (!KeyToGuid(field.Key, projectGuid)) continue;
			const TSharedPtr<FJsonObject> entry = field.Value->AsObject();
			if (!entry.IsValid()) continue;

			FProjectImportRecord rec;
			entry->TryGetStringField(TEXT("projectName"),   rec.ProjectName);
			entry->TryGetStringField(TEXT("sourcePath"),    rec.SourcePath);
			entry->TryGetStringField(TEXT("projectFolder"), rec.ProjectFolder);
			FString dtStr; entry->TryGetStringField(TEXT("importedAt"), dtStr);
			rec.ImportedAt = StringToDateTime(dtStr);

			const TArray<TSharedPtr<FJsonValue>>* guidsArray = nullptr;
			if (entry->TryGetArrayField(TEXT("dialogueGuids"), guidsArray))
			{
				for (const TSharedPtr<FJsonValue>& val : *guidsArray)
				{
					FGuid dlgGuid;
					FString gStr;
					if (val->TryGetString(gStr) && KeyToGuid(gStr, dlgGuid))
						rec.DialogueGuids.Add(dlgGuid);
				}
			}
			ProjectHistory.Add(projectGuid, MoveTemp(rec));
		}
	}
}
