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
