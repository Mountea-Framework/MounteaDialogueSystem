// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

class MOUNTEADIALOGUESYSTEMEDITOR_API FMounteaDialogueSearchHelpers
{
public:
	static TSharedPtr<SDockTab> InvokeTab(TSharedPtr<FTabManager> TabManager, const FTabId& TabID);
};