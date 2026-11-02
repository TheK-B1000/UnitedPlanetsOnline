// White Tiger Studios 2024

#include "WarOfGods.h"
#include "Arenas/ArenaConsoleCommands.h"
#include "Characters/GOTAPlayerCharacter.h"
#include "Modules/ModuleManager.h"

class FWarOfGodsModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		AGOTAPlayerCharacter::InstallGlobalDilationGuard();
		FArenaConsoleCommands::Register();
	}

	virtual void ShutdownModule() override
	{
		FArenaConsoleCommands::Unregister();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FWarOfGodsModule, WarOfGods, "WarOfGods");
