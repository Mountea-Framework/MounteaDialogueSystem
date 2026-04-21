// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FMounteaDialogueGraphEditorStyle.h"

#include "EditorStyle/MounteaDialogueGraphVisualTokens.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Misc/Paths.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define SVG_BRUSH( RelativePath, ... ) FSlateVectorImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".svg") ), __VA_ARGS__ )
#define SVG_BRUSH_TINT( RelativePath, Size, Tint ) FSlateVectorImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".svg") ), Size, Tint )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

TSharedPtr<FSlateStyleSet> FMounteaDialogueGraphEditorStyle::StyleSet = nullptr;

void FMounteaDialogueGraphEditorStyle::Initialize()
{
	if (!StyleSet.IsValid() )
	{
		Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

		StyleSet->SetParentStyleName(FAppStyle::GetAppStyleSetName());
	}
}

void FMounteaDialogueGraphEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	ensure(StyleSet.IsUnique());
	StyleSet.Reset();
}

void FMounteaDialogueGraphEditorStyle::Create()
{
	const FVector2D Icon12x12(12.0f, 12.0f);
	const FVector2D Icon14x14(14.0f, 14.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon24x24(24.0f, 24.0f);
	const FVector2D Icon32x32(32.0f, 32.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);
	const FVector2D Icon128x128(128.f, 128.f);
	const FVector2D Icon200x70(200.f, 70.f);
	
	StyleSet = MakeShareable(new FSlateStyleSet(GetAppStyleSetName()));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir() / TEXT("Resources"));
	
	StyleSet->Set("MDSStyleSet.MounteaLogo", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon40x40));
	StyleSet->Set("MDSStyleSet.OpenGraphSourceIcon", new IMAGE_BRUSH(TEXT("OpenGraphSourceIcon"), Icon40x40));

	StyleSet->Set("MDSStyleSet.Icon.Warning", new SVG_BRUSH_TINT(TEXT("WebIcons/triangle-alert"), Icon40x40, FLinearColor::FromSRGBColor(FColor::Orange)));
	StyleSet->Set("MDSStyleSet.Icon.Success", new SVG_BRUSH_TINT(TEXT("WebIcons/circle-check-big"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Info.Error", new SVG_BRUSH_TINT(TEXT("WebIcons/circle-x"), Icon40x40, FLinearColor::Red));

	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/sunset"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/sunrise"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/shield-check"), Icon40x40, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.AutoArrange", new SVG_BRUSH_TINT(TEXT("WebIcons/network"), Icon24x24, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.RecenterGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/locate-fixed"), Icon24x24, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.FitGraphToView", new SVG_BRUSH_TINT(TEXT("WebIcons/crosshair"), Icon24x24, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.ExportGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/folder-input"), Icon24x24, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.ReimportGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/folder-sync"), Icon24x24, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.GraphSettings", new SVG_BRUSH_TINT(TEXT("WebIcons/settings"), Icon24x24, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.ValidateGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/shield-check"), Icon24x24, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.Graph.SimpleArrow", new IMAGE_BRUSH(TEXT("SimpleArrow"), Icon16x16));

	StyleSet->Set("MDSStyleSet.Node.SoftEdges", new BOX_BRUSH(TEXT("NodeSoftCorners"), FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f)));
	StyleSet->Set("MDSStyleSet.Node.HardEdges", new BOX_BRUSH(TEXT("NodeHardCorners"), FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f)));

	StyleSet->Set("MDSStyleSet.Node.TextSoftEdges", new BOX_BRUSH(TEXT("TextSoftCorners"), FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f)));
	StyleSet->Set("MDSStyleSet.Node.TextHardEdges", new BOX_BRUSH(TEXT("TextHardCorners"), FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.RoundAll", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(12.f, 12.f, 12.f, 12.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.RoundTop", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(12.f, 12.f, 0.f, 0.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.RoundLeft", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(12.f, 0.f, 0.f, 12.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.Pill", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(999.f, 999.f, 999.f, 999.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.Tag", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(6.f, 6.f, 6.f, 6.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.Circle", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(999.f, 999.f, 999.f, 999.f)));
	StyleSet->Set("MDSStyleSet.Edge.SelectionRing", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(999.f, 999.f, 999.f, 999.f)));

	StyleSet->Set("MDSStyleSet.Icon.OK", new SVG_BRUSH_TINT(TEXT("WebIcons/x"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.Error", new SVG_BRUSH_TINT(TEXT("WebIcons/x"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.BulletPoint", new IMAGE_BRUSH(TEXT("CircleBox"), Icon16x16));

	StyleSet->Set("MDSStyleSet.Graph.CornerImage", new IMAGE_BRUSH(TEXT("Icon128"), Icon128x128));

	StyleSet->Set("MDSStyleSet.Icon.Delete", new SVG_BRUSH_TINT(TEXT("WebIcons/trash-2"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.Browse", new SVG_BRUSH_TINT(TEXT("WebIcons/folder-search"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.Edit", new SVG_BRUSH_TINT(TEXT("WebIcons/pencil"), Icon12x12, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.Buttons.Documentation", new IMAGE_BRUSH(TEXT("Documentation"), Icon200x70));
	StyleSet->Set("MDSStyleSet.Buttons.Documentation.Small", new SVG_BRUSH_TINT(TEXT("WebIcons/book-open-text"), Icon14x14, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.Node.Icon", new IMAGE_BRUSH(TEXT("DialogueNodeIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Node.Icon.Default", new SVG_BRUSH_TINT(TEXT("WebIcons/message-square-dashed"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Start", new SVG_BRUSH_TINT(TEXT("WebIcons/play"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Lead", new SVG_BRUSH_TINT(TEXT("WebIcons/message-circle"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Answer", new SVG_BRUSH_TINT(TEXT("WebIcons/user"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Delay", new SVG_BRUSH_TINT(TEXT("WebIcons/clock-3"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Complete", new SVG_BRUSH_TINT(TEXT("WebIcons/circle-check-big"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Return", new SVG_BRUSH_TINT(TEXT("WebIcons/tags"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.OpenChildGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/external-link"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Speech", new SVG_BRUSH_TINT(TEXT("WebIcons/quote"), Icon14x14, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.Node.Chip.ParticipantIcon", new SVG_BRUSH(TEXT("WebIcons/user"), Icon14x14));
	StyleSet->Set("MDSStyleSet.Node.Chip.DecoratorIcon", new SVG_BRUSH(TEXT("WebIcons/tags"), Icon14x14));
	
	StyleSet->Set("MDSStyleSet.Edge.MidpointIcon", new SVG_BRUSH(TEXT("WebIcons/sliders-horizontal"), Icon14x14));

	StyleSet->Set("MDSStyleSet.Icon.Close", new SVG_BRUSH_TINT(TEXT("WebIcons/x"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.SupportDiscord", new SVG_BRUSH_TINT(TEXT("WebIcons/Discord-Symbol-White"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.HeartIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/heart"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Icon.UBIcon", new IMAGE_BRUSH(TEXT("UnrealBucketIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.MoneyIcon", new SVG_BRUSH_TINT(TEXT("WebIcons/hand-coins"), Icon12x12, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.Help", new SVG_BRUSH_TINT(TEXT("WebIcons/message-circle-question-mark"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Dialoguer", new IMAGE_BRUSH(TEXT("Dialoguer_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Launcher", new IMAGE_BRUSH(TEXT("MPLIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Builder", new IMAGE_BRUSH(TEXT("Builder_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Wiki", new SVG_BRUSH_TINT(TEXT("WebIcons/book-open-text"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Youtube", new SVG_BRUSH_TINT(TEXT("WebIcons/brand-youtube"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Settings", new SVG_BRUSH_TINT(TEXT("WebIcons/settings"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Config", new SVG_BRUSH_TINT(TEXT("WebIcons/settings-2"), Icon12x12, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.DialogueSystemIcon", new IMAGE_BRUSH(TEXT("DialogueGraph"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Tutorial", new SVG_BRUSH_TINT(TEXT("WebIcons/graduation-cap"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Folder", new SVG_BRUSH_TINT(TEXT("WebIcons/folder-open"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Level", new SVG_BRUSH_TINT(TEXT("WebIcons/mountain-snow"), Icon40x40, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.AutoSetup", new SVG_BRUSH_TINT(TEXT("WebIcons/sparkles-2"), Icon40x40, FLinearColor::White));

	const FButtonStyle MounteaButtonStyle = FButtonStyle()
		.SetNormal(BOX_BRUSH("RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(1, 1, 1, 0.1f)))
		.SetHovered(BOX_BRUSH("RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(1, .55f, 0, 0.2f)))
		.SetPressed(BOX_BRUSH("RoundedSelection_16x", 4.0f / 16.0f,  FLinearColor(1, .55f, 0, 0.4f)));

	StyleSet->Set("MDSStyleSet.Buttons.Style", MounteaButtonStyle);

	FButtonStyle MounteaCloseButtonStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button");;
		MounteaCloseButtonStyle.Normal.TintColor = FLinearColor(1.f, 0.1f, 0.1f, 0.65f);
		MounteaCloseButtonStyle.Hovered.TintColor = FLinearColor(0.8f, 0.1f, 0.1f, 1.f);
		MounteaCloseButtonStyle.Pressed.TintColor = FLinearColor(1.0f, 0.1f, 0.1f, 0.85f);

	StyleSet->Set("MDSStyleSet.Buttons.CloseStyle", MounteaCloseButtonStyle);

	{
		const FScrollBarStyle ScrollBar = GetWidgetStyle<FScrollBarStyle>( "ScrollBar" );
		const FString interCollectionPath = StyleSet->RootToContentDir(TEXT("Fonts/Inter"), TEXT(".ttc"));
		const FString interLightPath = StyleSet->RootToContentDir(TEXT("Fonts/Inter-Light"), TEXT(".ttf"));
		const FString interSemiBoldPath = StyleSet->RootToContentDir(TEXT("Fonts/Inter-SemiBold"), TEXT(".ttf"));

		FSlateFontInfo interTitle(interCollectionPath, 12);
		interTitle.TypefaceFontName = FName(TEXT("SemiBold"));
		if (FPaths::FileExists(interSemiBoldPath))
			interTitle = FSlateFontInfo(interSemiBoldPath, 12);

		FSlateFontInfo interRegular(interCollectionPath, 14);
		interRegular.TypefaceFontName = FName(TEXT("Regular"));

		FSlateFontInfo interSmall(interCollectionPath, 12);
		interSmall.TypefaceFontName = FName(TEXT("SemiBold"));
		if (FPaths::FileExists(interSemiBoldPath))
			interSmall = FSlateFontInfo(interSemiBoldPath, 12);

		FSlateFontInfo interBody(interCollectionPath, 12);
		interBody.TypefaceFontName = FName(TEXT("Regular"));

		FSlateFontInfo interTag(interCollectionPath, 10);
		interTag.TypefaceFontName = FName(TEXT("Light"));
		if (FPaths::FileExists(interLightPath))
			interTag = FSlateFontInfo(interLightPath, 10);

		FTextBlockStyle NormalText = FTextBlockStyle()
			.SetFont(interRegular)
			.SetColorAndOpacity(FMounteaDialogueGraphVisualTokens::GetCardForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Transparent)
			.SetHighlightColor(FMounteaDialogueGraphVisualTokens::GetPrimaryAccent())
			.SetHighlightShape( BOX_BRUSH( "TextBlockHighlightShape", FMargin(3.f/8.f) ) );
		
		FTextBlockStyle NodeTitle = FTextBlockStyle(NormalText)
			.SetFont(interTitle)
			.SetColorAndOpacity(FMounteaDialogueGraphVisualTokens::GetCardForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Transparent);
		StyleSet->Set( "MDSStyleSet.NodeTitle", NodeTitle );

		FTextBlockStyle NodeTitleSmall = FTextBlockStyle(NormalText)
			.SetFont(interSmall)
			.SetColorAndOpacity(FMounteaDialogueGraphVisualTokens::GetCardForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Transparent);
		StyleSet->Set("MDSStyleSet.NodeTitle.Small", NodeTitleSmall);

		FTextBlockStyle NodeBody = FTextBlockStyle(NormalText)
			.SetFont(interBody)
			.SetColorAndOpacity(FMounteaDialogueGraphVisualTokens::GetCardForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Transparent);
		StyleSet->Set("MDSStyleSet.NodeBody", NodeBody);

		FTextBlockStyle NodeTag = FTextBlockStyle(NormalText)
			.SetFont(interTag)
			.SetColorAndOpacity(FMounteaDialogueGraphVisualTokens::GetCardForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Transparent);
		StyleSet->Set("MDSStyleSet.NodeTag", NodeTag);

		FEditableTextBoxStyle NodeTitleEditableText = FEditableTextBoxStyle()
			.SetFont(NodeTitle.Font)
			.SetBackgroundImageNormal( BOX_BRUSH( "TextBox", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageHovered( BOX_BRUSH( "TextBox_Hovered", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageFocused( BOX_BRUSH( "TextBox_Hovered", FMargin(4.0f/16.0f) ) )
			.SetBackgroundImageReadOnly( BOX_BRUSH( "TextBox_ReadOnly", FMargin(4.0f/16.0f) ) )
			.SetScrollBarStyle( ScrollBar );
		StyleSet->Set( "MDSStyleSet.NodeTitleEditableText", NodeTitleEditableText );

		StyleSet->Set( "MDSStyleSet.NodeTitleInlineEditableText", FInlineEditableTextBlockStyle()
			.SetTextStyle(NodeTitle)
			.SetEditableTextBoxStyle(NodeTitleEditableText)
			);
	}
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef SVG_BRUSH
#undef SVG_BRUSH_TINT
#undef DEFAULT_FONT
