// All rights reserved Dominik Morse 2024

#pragma once

#include "Widgets/SCompoundWidget.h"

class SDialogueSystemTutorialPage : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDialogueSystemTutorialPage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};
