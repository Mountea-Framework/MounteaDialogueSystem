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
#include "Styling/SlateStyle.h"

class FMounteaDialogueGraphEditorStyle : public FAppStyle
{
public:
	
	static void Create();
	static void Initialize();
	static void Shutdown();
	static ISlateStyle& Get()
	{
		return *(StyleSet.Get());
	}

	static const FSlateBrush * GetBrush(FName PropertyName, const ANSICHAR* Specifier = NULL)
	{
		return StyleSet->GetBrush(PropertyName, Specifier);
	};

	static const FTextBlockStyle* GetTextBlockStyle( FName PropertyName, const ANSICHAR* Specifier = NULL )
	{
		return &StyleSet->GetWidgetStyle<FTextBlockStyle>(PropertyName);
	};
	
	static const FName& GetAppStyleSetName()
	{
		static FName StyleSetName(TEXT("MounteaDialogueEditorStyle"));
		return StyleSetName;
	};

	template< class T >            
	static const T& GetWidgetStyle( FName PropertyName, const ANSICHAR* Specifier = NULL  ) 
	{
		return StyleSet->GetWidgetStyle< T >( PropertyName, Specifier );
	}

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
