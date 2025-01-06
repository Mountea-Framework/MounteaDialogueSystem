// All rights reserved Dominik Morse (Pavlicek) 2024

#include "MonuteaDialogueSystemDeveloperStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

TSharedPtr<FSlateStyleSet> FMonuteaDialogueSystemDeveloperStyle::StyleSet = nullptr;

void FMonuteaDialogueSystemDeveloperStyle::Create()
{
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);
	const FVector2D Icon128x128(128.f, 128.f);
	const FVector2D Icon200x70(200.f, 70.f);
	
	StyleSet = MakeShareable(new FSlateStyleSet(GetAppStyleSetName()));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir() / TEXT("Resources"));

	StyleSet->Set("MDSStyleSet.MounteaLogo.Small", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon16x16));
	StyleSet->Set("MDSStyleSet.MounteaLogo", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon40x40));

	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_ValidateIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon", new IMAGE_BRUSH(TEXT("K2Node_ValidateIcon"), Icon40x40));

	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_SetterIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon", new IMAGE_BRUSH(TEXT("K2Node_SetterIcon"), Icon40x40));
	
	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_GetterIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon", new IMAGE_BRUSH(TEXT("K2Node_GetterIcon"), Icon40x40));
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
