// =============================================================================
// Ironvale.h — Main module header
// Project Ironvale: First-person grounded medieval RPG
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogIronvale, Log, All);

class FIronvaleModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
