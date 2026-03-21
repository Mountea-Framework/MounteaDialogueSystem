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
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);
	const FVector2D Icon128x128(128.f, 128.f);
	const FVector2D Icon200x70(200.f, 70.f);
	
	StyleSet = MakeShareable(new FSlateStyleSet(GetAppStyleSetName()));
	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin("MounteaDialogueSystem")->GetBaseDir() / TEXT("Resources"));
	
	StyleSet->Set("MDSStyleSet.MounteaLogo.Small", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon16x16));
	StyleSet->Set("MDSStyleSet.MounteaLogo", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon40x40));
	StyleSet->Set("MDSStyleSet.MounteaLogo.large", new IMAGE_BRUSH(TEXT("Mountea_Logo"), Icon64x64));

	StyleSet->Set("MDSStyleSet.OpenGraphSourceIcon.Small", new IMAGE_BRUSH(TEXT("OpenGraphSourceIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.OpenGraphSourceIcon", new IMAGE_BRUSH(TEXT("OpenGraphSourceIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.OpenGraphSourceIcon.large", new IMAGE_BRUSH(TEXT("OpenGraphSourceIcon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Icon.Warning.Small", new IMAGE_BRUSH(TEXT("InfoWarning_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Icon.Warning", new IMAGE_BRUSH(TEXT("InfoWarning_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Icon.Warning.large", new IMAGE_BRUSH(TEXT("InfoWarning_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Icon.Success.Small", new IMAGE_BRUSH(TEXT("InfoSuccess_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Icon.Success", new IMAGE_BRUSH(TEXT("InfoSuccess_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Icon.Success.large", new IMAGE_BRUSH(TEXT("InfoSuccess_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Info.Error.Small", new IMAGE_BRUSH(TEXT("InfoError_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Info.Error", new IMAGE_BRUSH(TEXT("InfoError_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Info.Error.large", new IMAGE_BRUSH(TEXT("InfoError_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_GetterIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon", new IMAGE_BRUSH(TEXT("K2Node_GetterIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.K2Node_GetterIcon.large", new IMAGE_BRUSH(TEXT("K2Node_GetterIcon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_SetterIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon", new IMAGE_BRUSH(TEXT("K2Node_SetterIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.K2Node_SetterIcon.large", new IMAGE_BRUSH(TEXT("K2Node_SetterIcon"), Icon64x64));
	
	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_ValidateIcon"), Icon16x16));
    StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon", new IMAGE_BRUSH(TEXT("K2Node_ValidateIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.K2Node_ValidateIcon.large", new IMAGE_BRUSH(TEXT("K2Node_ValidateIcon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.K2Node_MessageIcon.Small", new IMAGE_BRUSH(TEXT("K2Node_MessageIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.K2Node_MessageIcon", new IMAGE_BRUSH(TEXT("K2Node_MessageIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.K2Node_MessageIcon.large", new IMAGE_BRUSH(TEXT("K2Node_MessageIcon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.AutoArrange.Small", new IMAGE_BRUSH(TEXT("AutoArrangeIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.AutoArrange", new IMAGE_BRUSH(TEXT("AutoArrangeIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.AutoArrange.large", new IMAGE_BRUSH(TEXT("AutoArrangeIcon"), Icon64x64));
	StyleSet->Set("MDSStyleSet.RecenterGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/locate-fixed"), Icon16x16, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.FitGraphToView", new SVG_BRUSH_TINT(TEXT("WebIcons/crosshair"), Icon16x16, FLinearColor::White));

	StyleSet->Set("MDSStyleSet.ExportGraph.Small", new IMAGE_BRUSH(TEXT("ExportGraphIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.ExportGraph", new IMAGE_BRUSH(TEXT("ExportGraphIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.ExportGraph.large", new IMAGE_BRUSH(TEXT("ExportGraphIcon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.ReimportGraph.Small", new IMAGE_BRUSH(TEXT("ReimportGraphIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.ReimportGraph", new IMAGE_BRUSH(TEXT("ReimportGraphIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.ReimportGraph.large", new IMAGE_BRUSH(TEXT("ReimportGraphIcon"), Icon64x64));
	
	StyleSet->Set("MDSStyleSet.GraphSettings.Small", new IMAGE_BRUSH(TEXT("GraphSettings"), Icon16x16));
	StyleSet->Set("MDSStyleSet.GraphSettings", new IMAGE_BRUSH(TEXT("GraphSettings"), Icon40x40));
	StyleSet->Set("MDSStyleSet.GraphSettings.large", new IMAGE_BRUSH(TEXT("GraphSettings"), Icon64x64));

	StyleSet->Set("MDSStyleSet.ValidateGraph.Small", new IMAGE_BRUSH(TEXT("ValidateGraph"), Icon16x16));
	StyleSet->Set("MDSStyleSet.ValidateGraph", new IMAGE_BRUSH(TEXT("ValidateGraph"), Icon40x40));
	StyleSet->Set("MDSStyleSet.ValidateGraph.large", new IMAGE_BRUSH(TEXT("ValidateGraph"), Icon64x64));
	
	StyleSet->Set("MDSStyleSet.Graph.SimpleArrow", new IMAGE_BRUSH(TEXT("SimpleArrow"), Icon16x16));

	StyleSet->Set("MDSStyleSet.Node.SoftEdges", new BOX_BRUSH( TEXT("NodeSoftCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));
	StyleSet->Set("MDSStyleSet.Node.HardEdges", new BOX_BRUSH( TEXT("NodeHardCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));

	StyleSet->Set("MDSStyleSet.Node.TextSoftEdges", new BOX_BRUSH( TEXT("TextSoftCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));
	StyleSet->Set("MDSStyleSet.Node.TextHardEdges", new BOX_BRUSH( TEXT("TextHardCorners") , FMargin(16.f/64.f, 25.f/64.f, 16.f/64.f, 16.f/64.f) ));
	StyleSet->Set("MDSStyleSet.Node.Flat.RoundAll", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(12.f, 12.f, 12.f, 12.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.RoundTop", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(12.f, 12.f, 0.f, 0.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.Pill", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(999.f, 999.f, 999.f, 999.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.Tag", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(6.f, 6.f, 6.f, 6.f)));
	StyleSet->Set("MDSStyleSet.Node.Flat.Circle", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(999.f, 999.f, 999.f, 999.f)));
	StyleSet->Set("MDSStyleSet.Edge.SelectionRing", new FSlateRoundedBoxBrush(FLinearColor::White, FVector4(999.f, 999.f, 999.f, 999.f)));

	StyleSet->Set("MDSStyleSet.Icon.OK", new IMAGE_BRUSH(TEXT("OKIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Icon.Error", new IMAGE_BRUSH(TEXT("ErrorIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Icon.BulletPoint", new IMAGE_BRUSH(TEXT("CircleBox"), Icon16x16));
	
	StyleSet->Set("MDSStyleSet.Graph.CornerImage", new IMAGE_BRUSH(TEXT("Icon128"), Icon128x128));

	StyleSet->Set("MDSStyleSet.Icon.Browse", new IMAGE_BRUSH(TEXT("BrowseIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.Edit", new IMAGE_BRUSH(TEXT("EditIcon"), Icon12x12));
	
	StyleSet->Set("MDSStyleSet.Buttons.Documentation", new IMAGE_BRUSH(TEXT("Documentation"), Icon200x70));
	StyleSet->Set("MDSStyleSet.Buttons.Documentation.Small", new IMAGE_BRUSH(TEXT("DocumentationIcon"), Icon12x12));

	StyleSet->Set("MDSStyleSet.Node.Icon.large", new IMAGE_BRUSH(TEXT("DialogueNodeIcon"), Icon64x64));
	StyleSet->Set("MDSStyleSet.Node.Icon", new IMAGE_BRUSH(TEXT("DialogueNodeIcon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Node.Icon.Small", new IMAGE_BRUSH(TEXT("DialogueNodeIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Node.Icon.Start", new SVG_BRUSH_TINT(TEXT("WebIcons/play"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Lead", new SVG_BRUSH_TINT(TEXT("WebIcons/message-circle"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Answer", new SVG_BRUSH_TINT(TEXT("WebIcons/user"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Delay", new SVG_BRUSH_TINT(TEXT("WebIcons/clock-3"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Complete", new SVG_BRUSH_TINT(TEXT("WebIcons/circle-check-big"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.Return", new SVG_BRUSH_TINT(TEXT("WebIcons/tags"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Icon.OpenChildGraph", new SVG_BRUSH_TINT(TEXT("WebIcons/external-link"), Icon14x14, FLinearColor::White));
	StyleSet->Set("MDSStyleSet.Node.Chip.ParticipantIcon", new SVG_BRUSH(TEXT("WebIcons/user"), Icon14x14));
	StyleSet->Set("MDSStyleSet.Node.Chip.DecoratorIcon", new SVG_BRUSH(TEXT("WebIcons/tags"), Icon14x14));
	StyleSet->Set("MDSStyleSet.Edge.MidpointIcon", new SVG_BRUSH(TEXT("WebIcons/sliders-horizontal"), Icon14x14));

	StyleSet->Set("MDSStyleSet.Icon.Close", new IMAGE_BRUSH(TEXT("CloseIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.SupportDiscord", new IMAGE_BRUSH(TEXT("Help_Icon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.HeartIcon", new IMAGE_BRUSH(TEXT("HeartIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.UBIcon", new IMAGE_BRUSH(TEXT("UnrealBucketIcon"), Icon12x12));
	StyleSet->Set("MDSStyleSet.Icon.MoneyIcon", new IMAGE_BRUSH(TEXT("MoneyIcon"), Icon12x12));

	StyleSet->Set("MDSStyleSet.Help", new IMAGE_BRUSH(TEXT("Help_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Help.Small", new IMAGE_BRUSH(TEXT("Help_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Help.large", new IMAGE_BRUSH(TEXT("Help_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Dialoguer", new IMAGE_BRUSH(TEXT("Dialoguer_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Dialoguer.Small", new IMAGE_BRUSH(TEXT("Dialoguer_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Dialoguer.large", new IMAGE_BRUSH(TEXT("Dialoguer_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Launcher", new IMAGE_BRUSH(TEXT("MPLIcon"), Icon40x40));

	StyleSet->Set("MDSStyleSet.Wiki", new IMAGE_BRUSH(TEXT("Wiki_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Wiki.Small", new IMAGE_BRUSH(TEXT("Wiki_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Wiki.large", new IMAGE_BRUSH(TEXT("Wiki_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Youtube", new IMAGE_BRUSH(TEXT("youtube_icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Youtube.Small", new IMAGE_BRUSH(TEXT("youtube_icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Youtube.large", new IMAGE_BRUSH(TEXT("youtube_icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Settings", new IMAGE_BRUSH(TEXT("settings_icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Settings.Small", new IMAGE_BRUSH(TEXT("settings_icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Settings.large", new IMAGE_BRUSH(TEXT("settings_icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.Config", new IMAGE_BRUSH(TEXT("configIcon"), Icon40x40));

	StyleSet->Set("MDSStyleSet.DialogueSystemIcon", new IMAGE_BRUSH(TEXT("DialogueGraph"), Icon16x16));

	StyleSet->Set("MDSStyleSet.Tutorial", new IMAGE_BRUSH(TEXT("tutorialPage_icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Folder", new IMAGE_BRUSH(TEXT("FolderIcon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Level", new IMAGE_BRUSH(TEXT("LevelIcon"), Icon40x40));

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
