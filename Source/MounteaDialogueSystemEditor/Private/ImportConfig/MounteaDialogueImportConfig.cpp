// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MounteaDialogueImportConfig.h"

#include "Dom/JsonObject.h"
#include "HAL/PlatformFileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UMounteaDialogueImportConfig::UMounteaDialogueImportConfig(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CategoryName = TEXT("Mountea Framework");
	SectionName = TEXT("Mountea Dialogue System (Import)");

	LoadFromFile();
}

bool UMounteaDialogueImportConfig::IsReimport(const FGuid& GraphGuid) const
{
	return ImportHistory.Contains(GraphGuid);
}

FString UMounteaDialogueImportConfig::WriteToConfig(const FGuid& DialogueGuid, const FDialogueImportSourceData& NewSourceData)
{
	FString outputResult;
	if (ImportHistory.Contains(DialogueGuid)) outputResult = TEXT("UPDATE");
	else outputResult = TEXT("CREATE");

	ImportHistory.Add(DialogueGuid, NewSourceData);

	return outputResult;
}

void UMounteaDialogueImportConfig::SaveToFile()
{
	const FString filePath = GetImportConfigFilePath();
	IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*FPaths::GetPath(filePath));

	TSharedRef<FJsonObject> rootObj  = MakeShared<FJsonObject>();
	TSharedRef<FJsonObject> historyObj = MakeShared<FJsonObject>();

	for (const auto& pair : ImportHistory)
	{
		TSharedRef<FJsonObject> entryObj = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(FDialogueImportSourceData::StaticStruct(), &pair.Value, entryObj, 0, 0))
			historyObj->SetObjectField(pair.Key.ToString(EGuidFormats::Digits), entryObj);
	}

	rootObj->SetObjectField(TEXT("ImportHistory"), historyObj);

	FString output;
	TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&output);
	FJsonSerializer::Serialize(rootObj, writer);

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

	TSharedPtr<FJsonObject> rootObj;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(content);
	if (!FJsonSerializer::Deserialize(reader, rootObj) || !rootObj.IsValid())
		return;

	ImportHistory.Reset();

	const TSharedPtr<FJsonObject>* historyField = nullptr;
	if (!rootObj->TryGetObjectField(TEXT("ImportHistory"), historyField))
		return;

	for (const auto& field : (*historyField)->Values)
	{
		FGuid guid;
		if (!FGuid::Parse(field.Key, guid))
			continue;

		const TSharedPtr<FJsonObject> entryObj = field.Value->AsObject();
		if (!entryObj.IsValid())
			continue;

		FDialogueImportSourceData sourceData;
		FJsonObjectConverter::JsonObjectToUStruct(entryObj.ToSharedRef(), &sourceData, 0, 0);
		ImportHistory.Add(guid, sourceData);
	}
}
