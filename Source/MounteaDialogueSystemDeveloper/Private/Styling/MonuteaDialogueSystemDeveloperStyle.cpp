// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MonuteaDialogueSystemDeveloperStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define SVG_BRUSH( RelativePath, ... ) FSlateVectorImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".svg") ), __VA_ARGS__ )
#define SVG_BRUSH_TINT( RelativePath, Size, Tint ) FSlateVectorImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".svg") ), Size, Tint )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

TSharedPtr<FSlateStyleSet> FMonuteaDialogueSystemDeveloperStyle::StyleSet = nullptr;

void FMonuteaDialogueSystemDeveloperStyle::Create()
{
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	
	StyleSet = MakeShareable(new FSlateStyleSet(GetAppStyleSetName()));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir() / TEXT("Resources"));

	StyleSet->Set("MDSStyleSet.MounteaLogo.Small", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon16x16));
	StyleSet->Set("MDSStyleSet.MounteaLogo", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon40x40));

	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon.Small", new SVG_BRUSH_TINT(TEXT("WebIcons/network"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/network"), Icon40x40, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon.Small", new SVG_BRUSH_TINT(TEXT("WebIcons/sunrise"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/sunrise"), Icon40x40, FLinearColor::White));
	
	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon.Small", new SVG_BRUSH_TINT(TEXT("WebIcons/sunset"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/sunset"), Icon40x40, FLinearColor::White));
}

void FMonuteaDialogueSystemDeveloperStyle::Initialize()
{
	if (!StyleSet.IsValid() )
	{
		Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

		StyleSet->SetParentStyleName(FAppStyle::GetAppStyleSetName());
		FAppStyle::SetAppStyleSet(*StyleSet);
	}
}

void FMonuteaDialogueSystemDeveloperStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	ensure(StyleSet.IsUnique());
	StyleSet.Reset();
}
