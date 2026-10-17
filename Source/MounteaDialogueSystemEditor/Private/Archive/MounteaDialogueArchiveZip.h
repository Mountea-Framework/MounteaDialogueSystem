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
#include "Archive/MounteaDialogueArchive.h"

namespace MounteaArchive
{
	/**
	 * Reads a zip from memory into raw entries (directories are skipped).
	 * Enforces the contract limits and rejects unsafe entry paths, like Dialoguer's own reader does.
	 */
	bool UnzipEntries(const TArray<uint8>& ZipData, FEntries& OutEntries, TArray<FString>& OutErrors);

	/**
	 * Writes entries as a standard deflate zip that Dialoguer (JSZip) reads: UTF-8 names, no ZIP64,
	 * no data descriptors, fixed timestamps. Entries are written in sorted order for reproducible output.
	 */
	bool ZipEntries(const FEntries& Entries, TArray<uint8>& OutZipData, TArray<FString>& OutErrors);

	/** Convenience: file bytes -> entries -> model. */
	bool ReadArchiveFromBytes(const TArray<uint8>& ZipData, FDialogueArchive& OutArchive, TArray<FString>& OutErrors);

	/** Convenience: model -> entries -> zip bytes. */
	bool WriteArchiveToBytes(const FDialogueArchive& Archive, TArray<uint8>& OutZipData, TArray<FString>& OutErrors);
}
