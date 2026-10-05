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
#include "Internationalization/StringTableCore.h"

#include <type_traits>

// Small shims for engine API differences between the UE versions this plugin is built for.
// They detect the API by shape rather than by engine version number, so a version that changes
// the API in between keeps compiling.
namespace MounteaDialogueCompat
{
	/**
	 * FStringTable::SetSourceString gained a required InDevNotes argument in UE 5.8 (UE 5.6 has two arguments).
	 */
	template<typename TStringTable>
	void SetStringTableSourceString(TStringTable& Table, const FString& Key, const FString& SourceString)
	{
		if constexpr (requires { Table.SetSourceString(FTextKey(Key), SourceString); })
		{
			Table.SetSourceString(FTextKey(Key), SourceString);
		}
		else
		{
			Table.SetSourceString(FTextKey(Key), SourceString, FString());
		}
	}

	/**
	 * Keys of FJsonObject::Values are FString up to UE 5.6 and UE::FSharedString from UE 5.8 on.
	 */
	template<typename TKey>
	FString JsonKeyToString(const TKey& Key)
	{
		if constexpr (std::is_convertible_v<const TKey&, FString>)
		{
			return Key;
		}
		else
		{
			return FString(Key.ToView());
		}
	}
}
