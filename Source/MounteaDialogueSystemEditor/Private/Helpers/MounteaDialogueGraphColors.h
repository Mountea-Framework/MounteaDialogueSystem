#pragma once

#include "CoreMinimal.h"

namespace MounteaDialogueGraphColors
{
	namespace NodeBody
	{
		constexpr FLinearColor Default(0.1f, 0.1f, 0.1f);
		constexpr FLinearColor Root(0.5f, 0.5f, 0.5f, 0.1f);
		constexpr FLinearColor Error(1.0f, 0.0f, 0.0f);
	}

	namespace NodeBorder
	{
		constexpr FLinearColor Inactive(0.08f, 0.08f, 0.08f);
		constexpr FLinearColor Root(0.2f, 0.2f, 0.2f, 0.2f);
		constexpr FLinearColor Selected(1.00f, 0.08f, 0.08f);
		constexpr FLinearColor ActiveDebugging(1.0f, 1.0f, 0.0f);
		constexpr FLinearColor InactiveDebugging(0.4f, 0.4f, 0.0f);
		constexpr FLinearColor HighlightAbortRange0(0.0f, 0.22f, 0.4f);
		constexpr FLinearColor HighlightAbortRange1(0.0f, 0.4f, 0.22f);
		constexpr FLinearColor Disconnected(0.f, 0.f, 0.f);
		constexpr FLinearColor BrokenWithParent(1.f, 0.f, 1.f);
		constexpr FLinearColor QuickFind(0.f, 0.8f, 0.f);
	}

	namespace Pin
	{
		constexpr FLinearColor Diff(0.9f, 0.2f, 0.15f);
		constexpr FLinearColor Hover(1.0f, 0.7f, 0.0f);
		constexpr FLinearColor Default(0.02f, 0.02f, 0.02f);
		constexpr FLinearColor SingleNode(0.02f, 0.02f, 0.02f);
	}
	
	namespace Connection
	{
		constexpr FLinearColor Default(1.0f, 1.0f, 1.0f);
	}

	namespace Action
	{
		constexpr FLinearColor DragMarker(1.0f, 1.0f, 0.2f);
	}

	namespace  Overlay
	{
		constexpr FLinearColor LightTheme(0.f, 0.f, 0.f, 0.5f);
		constexpr FLinearColor DarkTheme(0.f, 0.f, 0.f, 0.9f);
	}

	namespace PinsDock
	{
		constexpr FLinearColor LightTheme(0.5f, 0.5f, 0.5f, 0.5f);
		constexpr FLinearColor DarkTheme(0.f, 0.f, 0.f, 0.75f);
	}
}