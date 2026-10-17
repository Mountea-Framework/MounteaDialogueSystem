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

#include "Archive/MounteaDialogueArchiveZip.h"

#include "Misc/Compression.h"
#include "Misc/Crc.h"

#include "zip.h"

namespace MounteaArchive
{
	namespace
	{
		/** Mirrors Dialoguer's safePath(): no traversal, absolute, drive, backslash or control characters. */
		bool IsSafeEntryPath(const FString& Path)
		{
			if (Path.IsEmpty() || Path.Contains(TEXT("\\")) || Path.StartsWith(TEXT("/")) || Path.Contains(TEXT(":")) || Path.Contains(TEXT("//")))
				return false;

			for (const TCHAR character : Path)
			{
				if (character < 32)
					return false;
			}

			TArray<FString> segments;
			Path.ParseIntoArray(segments, TEXT("/"), false);
			for (const FString& segment : segments)
			{
				if (segment == TEXT("..") || segment == TEXT("."))
					return false;
			}
			return true;
		}

		void AppendU16(TArray<uint8>& Buffer, const uint16 Value) { Buffer.Append(reinterpret_cast<const uint8*>(&Value), 2); }
		void AppendU32(TArray<uint8>& Buffer, const uint32 Value) { Buffer.Append(reinterpret_cast<const uint8*>(&Value), 4); }

		struct FCentralRecord
		{
			TArray<uint8> NameBytes;
			uint32 Crc;
			uint32 CompressedSize;
			uint32 UncompressedSize;
			uint16 Method;
			uint32 Offset;

			FCentralRecord(const FString& InName, const uint32 InCrc, const uint32 InCompressed, const uint32 InUncompressed, const uint16 InMethod, const uint32 InOffset)
				: Crc(InCrc), CompressedSize(InCompressed), UncompressedSize(InUncompressed), Method(InMethod), Offset(InOffset)
			{
				const FTCHARToUTF8 converted(*InName);
				this->NameBytes.Append(reinterpret_cast<const uint8*>(converted.Get()), converted.Length());
			}
		};

		/** Raw deflate (no zlib wrapper). Returns false when compression does not help or fails. */
		bool DeflateRaw(const TArray<uint8>& Source, TArray<uint8>& OutDeflated)
		{
			if (Source.Num() < 64)
				return false;

			int32 bound = FCompression::CompressMemoryBound(NAME_Zlib, Source.Num());
			TArray<uint8> zlib;
			zlib.SetNumUninitialized(bound);
			if (!FCompression::CompressMemory(NAME_Zlib, zlib.GetData(), bound, Source.GetData(), Source.Num()))
				return false;

			// zlib stream = 2 byte header + raw deflate + 4 byte adler32. The zip format wants the raw part only.
			if (bound <= 6)
				return false;
			OutDeflated.Append(zlib.GetData() + 2, bound - 6);
			return OutDeflated.Num() < Source.Num();
		}
	}

	bool UnzipEntries(const TArray<uint8>& ZipData, FEntries& OutEntries, TArray<FString>& OutErrors)
	{
		OutEntries.Reset();

		if (ZipData.Num() > Contract::MaxCompressedBytes)
		{
			OutErrors.Add(TEXT("ARCHIVE_COMPRESSED_LIMIT: Archive exceeds the compressed size limit."));
			return false;
		}
		if (ZipData.Num() < 4)
		{
			OutErrors.Add(TEXT("INVALID_ARCHIVE: File is too small to be a zip archive."));
			return false;
		}

		struct zip_t* Zip = zip_stream_open(reinterpret_cast<const char*>(ZipData.GetData()), static_cast<size_t>(ZipData.Num()), 0, 'r');
		if (!Zip)
		{
			OutErrors.Add(TEXT("INVALID_ARCHIVE: File is not a readable zip archive."));
			return false;
		}

		bool bOk = true;
		int64 expandedBytes = 0;
		const ssize_t Total = zip_entries_total(Zip);
		if (Total > Contract::MaxEntries)
		{
			OutErrors.Add(TEXT("ARCHIVE_ENTRY_LIMIT: Archive exceeds the entry count limit."));
			bOk = false;
		}

		for (ssize_t Index = 0; bOk && Index < Total; ++Index)
		{
			if (zip_entry_openbyindex(Zip, static_cast<size_t>(Index)) != 0)
			{
				OutErrors.Add(FString::Printf(TEXT("INVALID_ARCHIVE: Entry %lld cannot be opened."), static_cast<int64>(Index)));
				bOk = false;
				break;
			}

			if (zip_entry_isdir(Zip) == 0)
			{
				const FString name = UTF8_TO_TCHAR(zip_entry_name(Zip));
				const uint64 Size = zip_entry_size(Zip);

				if (!IsSafeEntryPath(name))
				{
					OutErrors.Add(FString::Printf(TEXT("UNSAFE_ARCHIVE_PATH: Archive contains an unsafe entry path (%s)."), *name));
					bOk = false;
				}
				else if (OutEntries.Contains(name))
				{
					OutErrors.Add(FString::Printf(TEXT("DUPLICATE_ARCHIVE_PATH: Archive contains duplicate entry paths (%s)."), *name));
					bOk = false;
				}
				else if (name.EndsWith(TEXT(".json")) && Size > static_cast<uint64>(Contract::MaxJsonEntryBytes))
				{
					OutErrors.Add(FString::Printf(TEXT("ARCHIVE_JSON_LIMIT: JSON entry exceeds its size limit (%s)."), *name));
					bOk = false;
				}
				else
				{
					expandedBytes += static_cast<int64>(Size);
					if (expandedBytes > Contract::MaxExpandedBytes)
					{
						OutErrors.Add(TEXT("ARCHIVE_EXPANSION_LIMIT: Archive expanded content exceeds its allowed size."));
						bOk = false;
					}
					else
					{
						TArray<uint8> bytes;
						bytes.SetNumUninitialized(static_cast<int32>(Size));
						if (Size > 0 && zip_entry_noallocread(Zip, bytes.GetData(), static_cast<size_t>(Size)) < 0)
						{
							OutErrors.Add(FString::Printf(TEXT("INVALID_ARCHIVE: Entry '%s' cannot be read or fails its checksum."), *name));
							bOk = false;
						}
						else
							OutEntries.Add(name, MoveTemp(bytes));
					}
				}
			}
			zip_entry_close(Zip);
		}

		zip_close(Zip);
		if (!bOk)
			OutEntries.Reset();
		return bOk;
	}

	bool ZipEntries(const FEntries& Entries, TArray<uint8>& OutZipData, TArray<FString>& OutErrors)
	{
		OutZipData.Reset();

		TArray<FString> names;
		Entries.GetKeys(names);
		names.Sort();

		// DOS timestamp fixed to 1980-01-01 so identical content gives identical archives.
		constexpr uint16 dosTime = 0x0000;
		constexpr uint16 dosDate = 0x0021;

		TArray<FCentralRecord> central;

		for (const FString& name : names)
		{
			if (!IsSafeEntryPath(name))
			{
				OutErrors.Add(FString::Printf(TEXT("UNSAFE_ARCHIVE_PATH: Refusing to write unsafe entry path (%s)."), *name));
				OutZipData.Reset();
				return false;
			}

			const TArray<uint8>& data = Entries[name];
			TArray<uint8> deflated;
			const bool bDeflate = DeflateRaw(data, deflated);
			const TArray<uint8>& payload = bDeflate ? deflated : data;
			const uint16 method = bDeflate ? 8 : 0;
			const uint32 crc = FCrc::MemCrc32(data.GetData(), data.Num());
			const uint32 offset = static_cast<uint32>(OutZipData.Num());

			central.Emplace(name, crc, static_cast<uint32>(payload.Num()), static_cast<uint32>(data.Num()), method, offset);
			const FCentralRecord& record = central.Last();

			const uint8 localSignature[] = { 0x50, 0x4b, 0x03, 0x04 };
			OutZipData.Append(localSignature, 4);
			AppendU16(OutZipData, 0x0014);
			AppendU16(OutZipData, 0x0800);
			AppendU16(OutZipData, method);
			AppendU16(OutZipData, dosTime);
			AppendU16(OutZipData, dosDate);
			AppendU32(OutZipData, crc);
			AppendU32(OutZipData, record.CompressedSize);
			AppendU32(OutZipData, record.UncompressedSize);
			AppendU16(OutZipData, static_cast<uint16>(record.NameBytes.Num()));
			AppendU16(OutZipData, 0x0000);
			OutZipData.Append(record.NameBytes);
			OutZipData.Append(payload);
		}

		const uint32 centralStart = static_cast<uint32>(OutZipData.Num());
		for (const FCentralRecord& record : central)
		{
			const uint8 centralSignature[] = { 0x50, 0x4b, 0x01, 0x02 };
			OutZipData.Append(centralSignature, 4);
			AppendU16(OutZipData, 0x0314);
			AppendU16(OutZipData, 0x0014);
			AppendU16(OutZipData, 0x0800);
			AppendU16(OutZipData, record.Method);
			AppendU16(OutZipData, dosTime);
			AppendU16(OutZipData, dosDate);
			AppendU32(OutZipData, record.Crc);
			AppendU32(OutZipData, record.CompressedSize);
			AppendU32(OutZipData, record.UncompressedSize);
			AppendU16(OutZipData, static_cast<uint16>(record.NameBytes.Num()));
			AppendU16(OutZipData, 0x0000);
			AppendU16(OutZipData, 0x0000);
			AppendU16(OutZipData, 0x0000);
			AppendU16(OutZipData, 0x0000);
			AppendU32(OutZipData, 0x00000000);
			AppendU32(OutZipData, record.Offset);
			OutZipData.Append(record.NameBytes);
		}
		const uint32 centralSize = static_cast<uint32>(OutZipData.Num()) - centralStart;

		const uint8 endSignature[] = { 0x50, 0x4b, 0x05, 0x06 };
		OutZipData.Append(endSignature, 4);
		AppendU16(OutZipData, 0x0000);
		AppendU16(OutZipData, 0x0000);
		AppendU16(OutZipData, static_cast<uint16>(central.Num()));
		AppendU16(OutZipData, static_cast<uint16>(central.Num()));
		AppendU32(OutZipData, centralSize);
		AppendU32(OutZipData, centralStart);
		AppendU16(OutZipData, 0x0000);

		if (OutZipData.Num() > Contract::MaxCompressedBytes)
		{
			OutErrors.Add(FString::Printf(TEXT("ARCHIVE_COMPRESSED_LIMIT: The archive is %.1f MiB, Dialoguer accepts at most %lld MiB. Remove or shorten audio and export again."),
				OutZipData.Num() / (1024.0 * 1024.0), Contract::MaxCompressedBytes / (1024 * 1024)));
			OutZipData.Reset();
			return false;
		}
		return true;
	}

	bool ReadArchiveFromBytes(const TArray<uint8>& ZipData, FDialogueArchive& OutArchive, TArray<FString>& OutErrors)
	{
		FEntries entries;
		if (!UnzipEntries(ZipData, entries, OutErrors))
			return false;
		return ReadArchive(entries, OutArchive, OutErrors);
	}

	bool WriteArchiveToBytes(const FDialogueArchive& Archive, TArray<uint8>& OutZipData, TArray<FString>& OutErrors)
	{
		FEntries entries;
		if (!WriteArchive(Archive, entries, OutErrors))
			return false;
		return ZipEntries(entries, OutZipData, OutErrors);
	}
}
