// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

#include "Menu.generated.h"

/**
 * 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// + Menu Setup, create the menu widget and set input to UI
	// + Default values were defined in case none specific value is used.
	UFUNCTION(BlueprintCallable)
	void MenuSetup(int32 NumberOfPublicConnections = 4, FString TypeOfMatch = FString(TEXT("FreeForAll")));

protected:

	virtual bool Initialize() override;
	virtual void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld) override;

private:
	// Buttons Binding and functions
	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;
	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UFUNCTION()
	void HostButtonClicked();
	UFUNCTION()
	void JoinButtonClicked();

	// Menu Teardown (Remove the menu and set input back to Game)
	void MenuTearDown();

	// Multiplayer Session Subsystem
	// The Subsystem designed to handle all online session functionalities
	class UMultiplayerSessionsSubsystem* MultiplayerSessionSubsystem;

	// Session properties
	int32 NumPublicConnections{4};
	FString MatchType{ TEXT("FreeForAll") };
};