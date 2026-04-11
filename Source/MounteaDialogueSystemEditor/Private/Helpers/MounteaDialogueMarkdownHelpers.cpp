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

#include "Helpers/MounteaDialogueMarkdownHelpers.h"

#include "Containers/StringConv.h"

extern "C"
{
#include "md4c.h"
#include "md4c-html.h"
}

namespace
{
	void AppendMarkdownHtml(const MD_CHAR* HtmlText, MD_SIZE HtmlSize, void* UserData)
	{
		if(!UserData)
			return;

		FString* htmlOutput = static_cast<FString*>(UserData);
		if(!htmlOutput)
			return;

		FUTF8ToTCHAR utf8Converter(reinterpret_cast<const ANSICHAR*>(HtmlText), static_cast<int32>(HtmlSize));
		htmlOutput->AppendChars(utf8Converter.Get(), utf8Converter.Length());
	}
}

bool FMounteaDialogueMarkdownHelpers::ConvertMarkdownToHtml(const FString& MarkdownText, FString& OutputHtml)
{
	OutputHtml.Empty();
	if(MarkdownText.IsEmpty())
		return false;

	FTCHARToUTF8 markdownUtf8(*MarkdownText);

	const unsigned parserFlags = MD_FLAG_TABLES
		| MD_FLAG_STRIKETHROUGH
		| MD_FLAG_PERMISSIVEAUTOLINKS
		| MD_FLAG_TASKLISTS;

	const int32 parseResult = md_html(
		reinterpret_cast<const MD_CHAR*>(markdownUtf8.Get()),
		static_cast<MD_SIZE>(markdownUtf8.Length()),
		&AppendMarkdownHtml,
		&OutputHtml,
		parserFlags,
		0
	);

	return parseResult == 0 && !OutputHtml.IsEmpty();
}
