// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FMounteaDialogueGraphEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/Paths.h"

TSharedPtr<FSlateStyleSet> FMounteaDialogueGraphEditorStyle::StyleSet = nullptr;

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( StyleSet->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

void FMounteaDialogueGraphEditorStyle::Initialize()
{
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);

	if (StyleSet.IsValid() )
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet("MDSStyleSet"));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir() / TEXT("Resources"));

	StyleSet->Set("MDSStyleSet.AutoArrange.small", new IMAGE_BRUSH(TEXT("AutoArrangeIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.AutoArrange", new IMAGE_BRUSH(TEXT("AutoArrangeIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.AutoArrange.large", new IMAGE_BRUSH(TEXT("AutoArrangeIcon"), Icon64x64));
	

	StyleSet->Set("MDSStyleSet.GraphSettings.small", new IMAGE_BRUSH(TEXT("GraphSettings"), Icon16x16));
	StyleSet->Set("MDSStyleSet.GraphSettings", new IMAGE_BRUSH(TEXT("GraphSettings"), Icon40x40));
	StyleSet->Set("MDSStyleSet.GraphSettings.large", new IMAGE_BRUSH(TEXT("GraphSettings"), Icon64x64));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FMounteaDialogueGraphEditorStyle::Shutdown()
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

const FName& FMounteaDialogueGraphEditorStyle::GetStyleSetName()
{
	return StyleSet->GetStyleSetName();
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT