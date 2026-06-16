// Copyright Narrative Tools 2025. 


#include "NarrativeCommonUISubsystem.h"
#include "NarrativeCommonHUD.h"

void UNarrativeCommonUISubsystem::ShowNotification(const FText& NotificationText, const float Duration)
{
	if (CommonHUD && !NotificationText.IsEmptyOrWhitespace())
	{
		CommonHUD->ShowNotification(NotificationText, Duration);
	}
}
