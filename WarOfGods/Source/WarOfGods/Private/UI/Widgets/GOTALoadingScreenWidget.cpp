// White Tiger Studios 2024

#include "UI/Widgets/GOTALoadingScreenWidget.h"

#include "Characters/GOTAHeroDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UGOTALoadingScreenWidget::ConfigureHero(const UGOTAHeroDefinition& Hero, const FText& ArenaName, const FText& ModeName, const FText& Tip)
{
	UTexture2D* Art = Hero.LoadingArt.IsNull() ? Hero.Portrait.LoadSynchronous() : Hero.LoadingArt.LoadSynchronous();
	if (Image_Art && Art)
	{
		Image_Art->SetBrushFromTexture(Art, false);
	}
	if (Text_Kicker) { Text_Kicker->SetText(FText::Format(NSLOCTEXT("GOTA", "LoadKickerFmt", "{0}  •  {1}"), ModeName, ArenaName)); }
	if (Text_Title) { Text_Title->SetText(Hero.DisplayName.ToUpper()); }
	if (Text_Subtitle) { Text_Subtitle->SetText(Hero.Title); }
	if (Text_Body)
	{
		FString Body = Hero.Quote.IsEmpty() ? FString() : FString::Printf(TEXT("“%s”\n\n"), *Hero.Quote.ToString());
		for (int32 i = 0; i < FMath::Min(2, Hero.Abilities.Num()); ++i)
		{
			Body += FString::Printf(TEXT("%s  —  %s\n"), *Hero.Abilities[i].Name.ToString().ToUpper(), *Hero.Abilities[i].Description.ToString());
		}
		Text_Body->SetText(FText::FromString(Body.TrimEnd()));
	}
	if (Text_Tip) { Text_Tip->SetText(Tip); }
}

void UGOTALoadingScreenWidget::ConfigureArena(UTexture2D* Art, const FText& ArenaName, const FText& Description, const FText& ModeName, const FText& Tip)
{
	if (Image_Art && Art)
	{
		Image_Art->SetBrushFromTexture(Art, false);
	}
	if (Text_Kicker) { Text_Kicker->SetText(ModeName); }
	if (Text_Title) { Text_Title->SetText(ArenaName.ToUpper()); }
	if (Text_Subtitle) { Text_Subtitle->SetText(FText::GetEmpty()); }
	if (Text_Body) { Text_Body->SetText(Description); }
	if (Text_Tip) { Text_Tip->SetText(Tip); }
}
