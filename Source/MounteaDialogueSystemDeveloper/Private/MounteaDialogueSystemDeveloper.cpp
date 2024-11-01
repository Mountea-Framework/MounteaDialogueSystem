#include "MounteaDialogueSystemDeveloper.h"

DEFINE_LOG_CATEGORY(MounteaDialogueSystemDeveloper);

#define LOCTEXT_NAMESPACE "FMounteaDialogueSystemDeveloper"

void FMounteaDialogueSystemDeveloper::StartupModule()
{	
	UE_LOG(MounteaDialogueSystemDeveloper, Warning, TEXT("MounteaDialogueSystemDeveloper module has been loaded"));
}

void FMounteaDialogueSystemDeveloper::ShutdownModule()
{	
	UE_LOG(MounteaDialogueSystemDeveloper, Warning, TEXT("MounteaDialogueSystemDeveloper module has been unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMounteaDialogueSystemDeveloper, MounteaDialogueSystemDeveloper)