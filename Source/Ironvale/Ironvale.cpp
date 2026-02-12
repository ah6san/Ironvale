// =============================================================================
// Ironvale.cpp — Main module implementation
// Project Ironvale: First-person grounded medieval RPG
// =============================================================================

#include "Ironvale.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogIronvale);

void FIronvaleModule::StartupModule()
{
	UE_LOG(LogIronvale, Log, TEXT("Ironvale module starting up"));
}

void FIronvaleModule::ShutdownModule()
{
	UE_LOG(LogIronvale, Log, TEXT("Ironvale module shutting down"));
}

IMPLEMENT_PRIMARY_GAME_MODULE(FIronvaleModule, Ironvale, "Ironvale");
