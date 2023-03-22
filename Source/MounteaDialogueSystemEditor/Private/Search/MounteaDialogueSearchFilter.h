// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

struct FMounteaDialogueSearchFilter
{
public:
	bool IsEmptyFilter() const
	{
		return SearchString.IsEmpty()
			&& bIncludeNodeTitle == false
			&& bIncludeNodeType == false
			&& bIncludeNodeDecoratorsTypes == false
			&& bIncludeNodeData == true
			&& bIncludeNodeGUID == false;
	}

public:
	// Search term that the search items must match
	FString SearchString;

	bool bIncludeNodeTitle = true;
	bool bIncludeNodeType = true;
	bool bIncludeNodeDecoratorsTypes = true;
	bool bIncludeNodeData = true;
	bool bIncludeNodeGUID = false;
};
