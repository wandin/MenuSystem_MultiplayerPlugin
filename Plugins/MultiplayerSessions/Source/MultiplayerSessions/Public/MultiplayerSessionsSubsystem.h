// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Interfaces/OnlineSessionInterface.h"

#include "MultiplayerSessionsSubsystem.generated.h"

/**
 * 
 */

// This is a GameInstance Subsystem - to hold our multiplayer functionalities
UCLASS()
class MULTIPLAYERSESSIONS_API UMultiplayerSessionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()


public:
	UMultiplayerSessionsSubsystem();

protected:


private:
	// Session interface declaration, which we will use to access functions from IOnlineSubsystem (see this classe's constructor)
	IOnlineSessionPtr SessionInterface;
};
