// White Tiger Studios 2024

#include "UI/GOTAMenuContentDefinition.h"

bool UGOTAMenuContentDefinition::FindDestination(EGOTAMenuAction Action, FGOTAMenuDestination& OutDestination) const
{
	for (const FGOTAMenuDestination& Destination : Destinations)
	{
		if (Destination.Action == Action)
		{
			OutDestination = Destination;
			return true;
		}
	}
	return false;
}
