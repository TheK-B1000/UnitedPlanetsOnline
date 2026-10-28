// White Tiger Studios 2024
// Helpers to invoke existing Blueprint GameInstance / MenuController session API by name.

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

namespace GOTAMenuBP
{
	static bool CallObjectFunction(UObject* Object, const FName FunctionName)
	{
		if (!Object)
		{
			return false;
		}
		if (UFunction* Function = Object->FindFunction(FunctionName))
		{
			// Existing menu BPs use parameterless custom events for these entry points.
			Object->ProcessEvent(Function, nullptr);
			return true;
		}
		UE_LOG(LogTemp, Warning, TEXT("GOTA menu: missing function %s on %s"),
			*FunctionName.ToString(), *GetNameSafe(Object));
		return false;
	}

	bool CallGameInstance(UObject* WorldContext, const FName FunctionName)
	{
		return CallObjectFunction(UGameplayStatics::GetGameInstance(WorldContext), FunctionName);
	}

	bool CallMenuController(UObject* WorldContext, const FName FunctionName)
	{
		return CallObjectFunction(UGameplayStatics::GetPlayerController(WorldContext, 0), FunctionName);
	}
}
