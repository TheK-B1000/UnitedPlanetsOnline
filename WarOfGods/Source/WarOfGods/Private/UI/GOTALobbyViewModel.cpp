// White Tiger Studios 2024

#include "UI/GOTALobbyViewModel.h"

UGOTALobbyViewModel::UGOTALobbyViewModel()
{
	ResetLobby();
}

void UGOTALobbyViewModel::ResetLobby()
{
	Slots.Reset(MaxLobbyPlayers);
	for (int32 i = 0; i < MaxLobbyPlayers; ++i)
	{
		FGOTALobbyPlayerSlot Slot;
		Slot.SlotIndex = i;
		Slot.State = EGOTALobbySlotState::Empty;
		Slot.PlayerName = (i == 0)
			? NSLOCTEXT("GOTA", "HostSlot", "Host")
			: FText::Format(NSLOCTEXT("GOTA", "FriendSlot", "Friend {0}"), FText::AsNumber(i));
		Slots.Add(Slot);
	}

	bSessionActive = false;
	bLocalPlayerIsHost = false;
	SelectedArena = EArenaId::TitansQuarry;
	SelectedMode = EArenaMode::Skirmish;
	Privacy = EGOTASessionPrivacy::FriendsOnly;
	StatusMessage = FText::GetEmpty();
	ErrorMessage = FText::GetEmpty();
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetSessionActive(bool bActive, bool bLocalIsHost)
{
	bSessionActive = bActive;
	bLocalPlayerIsHost = bLocalIsHost;
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetSessionPrivacy(EGOTASessionPrivacy InPrivacy)
{
	Privacy = InPrivacy;
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetSelectedArena(EArenaId ArenaId)
{
	SelectedArena = ArenaId;
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetSelectedMode(EArenaMode Mode)
{
	SelectedMode = Mode;
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetStatusMessage(FText Message)
{
	StatusMessage = MoveTemp(Message);
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetErrorMessage(FText Message)
{
	ErrorMessage = MoveTemp(Message);
	BroadcastUpdate();
}

void UGOTALobbyViewModel::UpsertPlayerSlot(int32 SlotIndex, FText PlayerName, EGOTALobbySlotState State, bool bIsHost, bool bIsLocal, bool bIsReady)
{
	if (!IsValidSlot(SlotIndex))
	{
		return;
	}

	FGOTALobbyPlayerSlot& Slot = Slots[SlotIndex];
	Slot.SlotIndex = SlotIndex;
	Slot.PlayerName = MoveTemp(PlayerName);
	Slot.State = State;
	Slot.bIsHost = bIsHost;
	Slot.bIsLocalPlayer = bIsLocal;
	Slot.bIsReady = bIsReady;
	if (bIsReady && State == EGOTALobbySlotState::Occupied)
	{
		Slot.State = EGOTALobbySlotState::Ready;
	}
	BroadcastUpdate();
}

void UGOTALobbyViewModel::ClearPlayerSlot(int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex))
	{
		return;
	}

	FGOTALobbyPlayerSlot& Slot =Slots[SlotIndex];
	Slot.State = EGOTALobbySlotState::Empty;
	Slot.bIsHost = false;
	Slot.bIsLocalPlayer = false;
	Slot.bIsReady = false;
	Slot.PlayerName = (SlotIndex == 0)
		? NSLOCTEXT("GOTA", "HostSlot", "Host")
		: FText::Format(NSLOCTEXT("GOTA", "FriendSlot", "Friend {0}"), FText::AsNumber(SlotIndex));
	BroadcastUpdate();
}

void UGOTALobbyViewModel::SetPlayerReady(int32 SlotIndex, bool bReady)
{
	if (!IsValidSlot(SlotIndex))
	{
		return;
	}
	Slots[SlotIndex].bIsReady = bReady;
	if (Slots[SlotIndex].State == EGOTALobbySlotState::Occupied || Slots[SlotIndex].State == EGOTALobbySlotState::Ready)
	{
		Slots[SlotIndex].State = bReady ? EGOTALobbySlotState::Ready : EGOTALobbySlotState::Occupied;
	}
	BroadcastUpdate();
}

int32 UGOTALobbyViewModel::GetOccupiedCount() const
{
	int32 Count = 0;
	for (const FGOTALobbyPlayerSlot& Slot :Slots)
	{
		if (Slot.State != EGOTALobbySlotState::Empty && Slot.State != EGOTALobbySlotState::Disconnected)
		{
			++Count;
		}
	}
	return Count;
}

bool UGOTALobbyViewModel::CanHostStartMatch() const
{
	if (!bSessionActive || !bLocalPlayerIsHost)
	{
		return false;
	}
	if (SelectedArena == EArenaId::None)
	{
		return false;
	}
	// Host may start solo or with friends; ready-check is optional until BP wires ready RPCs.
	return GetOccupiedCount() >= 1;
}

void UGOTALobbyViewModel::BroadcastUpdate()
{
	OnLobbyUpdated.Broadcast();
}

bool UGOTALobbyViewModel::IsValidSlot(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex);
}
