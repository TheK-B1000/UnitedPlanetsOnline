// White Tiger Studios 2024

#include "WarOfGods.h"
#include "Arenas/ArenaConsoleCommands.h"
#include "Modules/ModuleManager.h"

class FWarOfGodsModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		FArenaConsoleCommands::Register();
	}

	virtual void ShutdownModule() override
	{
		FArenaConsoleCommands::Unregister();
		FDefaultGameModuleImpl::ShutdownModule();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FWarOfGodsModule, WarOfGods, "WarOfGods");
