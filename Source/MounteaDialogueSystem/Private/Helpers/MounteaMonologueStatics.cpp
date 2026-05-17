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


#include "Helpers/MounteaMonologueStatics.h"

#include "Graph/MounteaDialogueGraph.h"
#include "Settings/MounteaDialogueSystemSettings.h"


bool UMounteaMonologueStatics::IsGraphMonologue(UMounteaDialogueGraph* Graph)
{
	if (!IsValid(Graph))
		return false;
	const auto dlgSettings = GetMutableDefault<UMounteaDialogueSystemSettings>();
	if (!IsValid(dlgSettings))
		return false;
	const auto dlgConfig = dlgSettings->GetDialogueConfiguration().LoadSynchronous();
	if (!IsValid(dlgConfig))
		return false;
	const auto mlgTag = dlgConfig->GraphTypeDefinitions.FindByPredicate([&](const FMounteaDialogueGraphTypeDefinition& Definition)
	{
		return Definition.TypeId.IsEqual(FName("Monologue"));
	});
	if (!mlgTag)
		return false;
	return Graph->GraphTags.HasTagExact(mlgTag->RootTag);
}
