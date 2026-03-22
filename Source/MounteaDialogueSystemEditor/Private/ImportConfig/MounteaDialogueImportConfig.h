// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MounteaDialogueImportConfig.generated.h"

/** Lightweight record tracking a single imported dialogue graph. */
USTRUCT()
struct FDialogueImportRecord
{
	GENERATED_BODY()

	/** Content-browser package path: "/Game/.../MerchantBranchingExample" */
	UPROPERTY(VisibleAnywhere, Category=Import)
	FString AssetPath;

	/** Source file that produced this dialogue (.mnteadlg or .mnteadlgproj) */
	UPROPERTY(VisibleAnywhere, Category=Import)
	FString SourcePath;

	UPROPERTY(VisibleAnywhere, Category=Import)
	FDateTime ImportedAt;

	UPROPERTY(VisibleAnywhere, Category=Import)
	bool bIsProjectImport = false;
};

/** Record for an entire project (.mnteadlgproj) import, keyed by projectGuid. */
USTRUCT()
struct FProjectImportRecord
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category=Import)
	FString ProjectName;

	/** Full path to the source .mnteadlgproj file */
	UPROPERTY(VisibleAnywhere, Category=Import)
	FString SourcePath;

	/**
	 * Content-browser root folder chosen for this project at first import.
	 * e.g. "/Game/TEST/Project/OnboardingExample"
	 * Used on reimport to place new dialogues and thumbnails at the same location.
	 */
	UPROPERTY(VisibleAnywhere, Category=Import)
	FString ProjectFolder;

	UPROPERTY(VisibleAnywhere, Category=Import)
	FDateTime ImportedAt;

	/** GUIDs of all dialogue graphs belonging to this project. */
	UPROPERTY(VisibleAnywhere, Category=Import)
	TArray<FGuid> DialogueGuids;
};

/**
 * Mountea Dialogue System (Import)
 *
 * Tracks import history so the system can locate existing assets on reimport.
 * Two separate histories:
 *   - DialogueHistory  keyed by dialogue GUID
 *   - ProjectHistory   keyed by projectGuid from projectData.json
 */
UCLASS(NotBlueprintable, NotBlueprintType, config=MounteaDialogueImportConfig,
	meta=(DisplayName = "Mountea Dialogue System (Import)"))
class MOUNTEADIALOGUESYSTEMEDITOR_API UMounteaDialogueImportConfig final : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UMounteaDialogueImportConfig(const FObjectInitializer& ObjectInitializer);

	/** Canonical path: <ProjectDir>/Config/Import/MounteaDialogueImportConfig.json */
	static FString GetImportConfigFilePath()
	{
		return FPaths::ProjectDir() / TEXT("Config/Import/MounteaDialogueImportConfig.json");
	}

	// ── Persistence ──────────────────────────────────────────────────────────────

	/** Serialize both histories to JSON and write to GetImportConfigFilePath(). */
	void SaveToFile();

	/** Read GetImportConfigFilePath() and populate both histories. */
	void LoadFromFile();

	// ── Dialogue helpers ─────────────────────────────────────────────────────────

	bool IsDialogueImported(const FGuid& Guid) const;

	/** Returns the stored content-browser asset path, or false if not found. */
	bool LookupDialogueAssetPath(const FGuid& Guid, FString& OutAssetPath) const;

	void RecordDialogueImport(const FGuid& Guid, const FString& AssetPath,
		const FString& SourcePath, bool bIsProjectImport = false);

	// ── Project helpers ──────────────────────────────────────────────────────────

	bool IsProjectImported(const FGuid& ProjectGuid) const;

	/**
	 * Returns the stored project folder if at least one member dialogue asset
	 * still exists in the asset registry (validates the stored path is live).
	 */
	bool LookupProjectFolder(const FGuid& ProjectGuid, FString& OutProjectFolder) const;

	void RecordProjectImport(const FGuid& ProjectGuid, const FString& ProjectName,
		const FString& SourcePath, const FString& ProjectFolder,
		const TArray<FGuid>& DialogueGuids);

public:

	UPROPERTY(VisibleDefaultsOnly, Category=Import)
	TMap<FGuid, FDialogueImportRecord> DialogueHistory;

	UPROPERTY(VisibleDefaultsOnly, Category=Import)
	TMap<FGuid, FProjectImportRecord> ProjectHistory;
};
