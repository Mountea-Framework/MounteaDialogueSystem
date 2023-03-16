// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FMounteaDialogueGraphEditorStyle
{
public:
	static void Initialize();
	static void Shutdown();

	static const FSlateBrush * GetBrush(FName PropertyName, const ANSICHAR* Specifier = NULL);
	static const FName& GetStyleSetName();

	template< class T >            
	static const T& GetWidgetStyle( FName PropertyName, const ANSICHAR* Specifier = NULL  ) 
	{
		return StyleSet->GetWidgetStyle< T >( PropertyName, Specifier );
	}

	static ISlateStyle& Get()
	{
		return *(StyleSet.Get());
	}

private:
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
