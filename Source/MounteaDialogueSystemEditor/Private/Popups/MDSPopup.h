// All rights reserved Dominik Morse (Pavlicek) 2024

#pragma once

struct FPluginVersion
{
	FString PluginVersion;
	FString PluginVersionName;

	FPluginVersion(const FString& A, const FString& B) : PluginVersion(A), PluginVersionName(B) {};
};

class MDSPopup
{
public:
	static void Register(const FString& ChangelogMarkdown, const FString& ChangelogHtml, const FString& HtmlSourcePath);
	static void Open(const FString& ChangelogHtml, const FString& HtmlSourcePath);

	static FPluginVersion GetPluginVersion();
	static bool IsVersionGreater(const FString& NewVersion, const FString& OldVersion);

private:
	static bool ExtractVersionFromMarkdown(const FString& ChangelogMarkdown, FString& OutVersion);
};
