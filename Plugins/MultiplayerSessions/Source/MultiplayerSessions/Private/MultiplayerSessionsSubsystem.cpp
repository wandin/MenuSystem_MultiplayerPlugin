// Fill out your copyright notice in the Description page of Project Settings.


#include "MultiplayerSessionsSubsystem.h"

#include "OnlineSubsystem.h"


// This is a GameInstance Subsystem - to hold our multiplayer functionalities
UMultiplayerSessionsSubsystem::UMultiplayerSessionsSubsystem()
{
	// pointer to OnlineSubsystem
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem)
	{
		// Store in the SessionInterface variable, interfaces from IOnlineSubsystem functionalities
		SessionInterface = Subsystem->GetSessionInterface();
	}
}
