// Copyright Dominik Pavlicek 2023. All Rights Reserved.

#include "FMounteaDialogueGraphEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Misc/Paths.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( StyleSet->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

TSharedPtr<FSlateStyleSet> FMounteaDialogueGraphEditorStyle::StyleSet = nullptr;

void FMounteaDialogueGraphEditorStyle::Initialize()
{
	if (!StyleSet.IsValid() )
	{
		Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

		StyleSet->SetParentStyleName(FAppStyle::GetAppStyleSetName());
		FAppStyle::SetAppStyleSet(*StyleSet);
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

	StyleSet->Set("MDSStyleSet.Node.IndexCircle", new IMAGE_BRUSH(TEXT("IndexIcon"), Icon16x16));

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

	StyleSet->Set("MDSStyleSet.Wiki", new IMAGE_BRUSH(TEXT("Wiki_Icon"), Icon40x40));
	StyleSet->Set("MDSStyleSet.Wiki.Small", new IMAGE_BRUSH(TEXT("Wiki_Icon"), Icon16x16));
	StyleSet->Set("MDSStyleSet.Wiki.large", new IMAGE_BRUSH(TEXT("Wiki_Icon"), Icon64x64));

	StyleSet->Set("MDSStyleSet.DialogueSystemIcon", new IMAGE_BRUSH(TEXT("DialogueSystem_Icon"), Icon16x16));

	const FButtonStyle MounteaButtonStyle = FButtonStyle()
		.SetNormal(BOX_BRUSH("RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(1, 1, 1, 0.1f)))
		.SetHovered(BOX_BRUSH("RoundedSelection_16x", 4.0f / 16.0f, FLinearColor(1, .55f, 0, 0.2f)))
		.SetPressed(BOX_BRUSH("RoundedSelection_16x", 4.0f / 16.0f,  FLinearColor(1, .55f, 0, 0.4f)));

	StyleSet->Set("MDSStyleSet.Buttons.Style", MounteaButtonStyle);

	{
		const FScrollBarStyle ScrollBar = GetWidgetStyle<FScrollBarStyle>( "ScrollBar" );

		FTextBlockStyle NormalText = FTextBlockStyle()
			.SetFont(DEFAULT_FONT("Regular", FCoreStyle::RegularTextSize))
			.SetColorAndOpacity(FSlateColor::UseForeground())
			.SetShadowOffset(FVector2D::ZeroVector)
			.SetShadowColorAndOpacity(FLinearColor::Black)
			.SetHighlightColor( FLinearColor( 0.02f, 0.3f, 0.0f ) )
			.SetHighlightShape( BOX_BRUSH( "TextBlockHighlightShape", FMargin(3.f/8.f) ) );
		
		FTextBlockStyle NodeTitle = FTextBlockStyle(NormalText)
			.SetFont( DEFAULT_FONT( "Bold", 14 ) )
			.SetColorAndOpacity( FLinearColor(230.0f/255.0f,230.0f/255.0f,230.0f/255.0f) )
			.SetShadowOffset( FVector2D( 2,2 ) )
			.SetShadowColorAndOpacity( FLinearColor(0.f,0.f,0.f, 0.7f) );
		StyleSet->Set( "MDSStyleSet.NodeTitle", NodeTitle );

		FEditableTextBoxStyle NodeTitleEditableText = FEditableTextBoxStyle()
			.SetFont(NormalText.Font)
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
#undef DEFAULT_FONT