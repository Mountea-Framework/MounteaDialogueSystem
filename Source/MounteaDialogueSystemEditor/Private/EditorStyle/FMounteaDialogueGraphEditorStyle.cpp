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
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);
	const FVector2D Icon128x128(128.f, 128.f);

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

	StyleSet->Set("MDSStyleSet.ValidateGraph.small", new IMAGE_BRUSH(TEXT("ValidateGraph"), Icon16x16));
	StyleSet->Set("MDSStyleSet.ValidateGraph", new IMAGE_BRUSH(TEXT("ValidateGraph"), Icon40x40));
	StyleSet->Set("MDSStyleSet.ValidateGraph.large", new IMAGE_BRUSH(TEXT("ValidateGraph"), Icon64x64));
	
	StyleSet->Set("MDSStyleSet.Graph.NodeOverlay", new BOX_BRUSH( TEXT("NodeOverlay"), FMargin(8.0f/64.0f, 3.0f/32.0f, 0, 0) ));
	StyleSet->Set("MDSStyleSet.Graph.PinDocksOverlay", new BOX_BRUSH( TEXT("PinDocksOverlay"), FMargin(8.0f/64.0f, 3.0f/32.0f, 0, 0) ));

	StyleSet->Set("MDSStyleSet.Graph.SimpleArrow", new IMAGE_BRUSH(TEXT("SimpleArrow"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Graph.HollowArrow", new IMAGE_BRUSH(TEXT("HollowArrow"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Graph.FancyArrow", new IMAGE_BRUSH(TEXT("FancyArrow"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Graph.Bubble", new IMAGE_BRUSH(TEXT("Bubble"), Icon16x16));

	StyleSet->Set("MDSStyleSet.Node.SoftEdges", new BOX_BRUSH( TEXT("NodeSoftCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));
	StyleSet->Set("MDSStyleSet.Node.HardEdges", new BOX_BRUSH( TEXT("NodeHardCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));

	StyleSet->Set("MDSStyleSet.Node.TextSoftEdges", new BOX_BRUSH( TEXT("TextSoftCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));
	StyleSet->Set("MDSStyleSet.Node.TextHardEdges", new BOX_BRUSH( TEXT("TextHardCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));

	StyleSet->Set("MDSStyleSet.Node.IndexCircle", new IMAGE_BRUSH(TEXT("CircleBox"), Icon16x16));

	StyleSet->Set("MDSStyleSet.Icon.OK", new IMAGE_BRUSH(TEXT("OKIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Icon.Error", new IMAGE_BRUSH(TEXT("ErroIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Icon.BulletPoint", new IMAGE_BRUSH(TEXT("CircleBox"), Icon16x16));
	
	StyleSet->Set("MDSStyleSet.Graph.CornerImage", new IMAGE_BRUSH(TEXT("Icon128"), Icon128x128));

	StyleSet->Set("MDSStyleSet.Icon.Browse", new IMAGE_BRUSH(TEXT("BrowseIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.Edit", new IMAGE_BRUSH(TEXT("EditIcon"), Icon12x12));
	
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

const FSlateBrush* FMounteaDialogueGraphEditorStyle::GetBrush(FName PropertyName, const ANSICHAR* Specifier)
{
	return StyleSet->GetBrush(PropertyName, Specifier);
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