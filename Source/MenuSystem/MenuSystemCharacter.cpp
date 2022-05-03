// Copyright Epic Games, Inc. All Rights Reserved.

#include "MenuSystemCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"




//////////////////////////////////////////////////////////////////////////
// AMenuSystemCharacter

AMenuSystemCharacter::AMenuSystemCharacter() : 
	CreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateGameSessionComplete)),
	FindSessionCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionComplete)),
	JoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rate for input
	TurnRateGamepad = 50.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)



	/* ONLINE SUBSYSTEM */
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();

	if (OnlineSubsystem)
	{
		OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 15.0f, FColor::Red, FString::Printf(TEXT("Found Subsystem %s"), *OnlineSubsystem->GetSubsystemName().ToString()));
		}
	}


}

//////////////////////////////////////////////////////////////////////////
// Input

void AMenuSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AMenuSystemCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AMenuSystemCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AMenuSystemCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AMenuSystemCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMenuSystemCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMenuSystemCharacter::TouchStopped);
}

void AMenuSystemCharacter::CreateGameSession()
{
	// called when pressed the 1 key

	// if Session INterface is not valid, return
	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	// if SessionInterface is valid, get the Session name and store to ExistingSession
	auto ExistingSession = OnlineSessionInterface->GetNamedSession(NAME_GameSession);

	// if Existing Session is not nullptr, we can destroy, because obviously there's a Session.
	if (ExistingSession != nullptr)
	{
		OnlineSessionInterface->DestroySession(NAME_GameSession);
	}

	// adding the Delegate function to our OnlineSessionINterface, in order to respond to our CreateSession 
	OnlineSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegate);

	// create a local variable Session Settings of type FOnlineSessionSettings, from the SessionSettings library, MakeShareable will wrap this in a TSharedptr
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = false;
	SessionSettings->NumPublicConnections = 4;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;

	SessionSettings->Set(FName("MatchType"), FString("FreeforAll"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// Get a UniqueNetId to use in the CreateSessionfunction below
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// the dereference FUniqueNetId operator returns a reference to FUniqueNetId, so we need to include the asterisc to dereference it. (*LocalPlayer->GetPreferredUniqueNetId()...)
	// the same for *SessionSettings
	OnlineSessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);


}

void AMenuSystemCharacter::OnCreateGameSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.0f,
				FColor::Green,
				FString::Printf(TEXT("Created Session: %s"), *SessionName.ToString())
			);
		}

		UWorld* World = GetWorld();
		if (World)
		{
			// travel to lobby level and open as LISTEN
			World->ServerTravel(FString("/Game/ThirdPerson/Maps/Lobby?listen"));
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.0f,
				FColor::Red,
				FString(TEXT("Failed to create Session"))
			);
		}
	}
}

void AMenuSystemCharacter::JoinGameSession()
{
	// find game sessions

	if (!OnlineSessionInterface.IsValid())
	{
		return;
	}

	//Add Find SessionComplete delegate to the OnlineSessionInterface delegate list
	OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindSessionCompleteDelegate);
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->MaxSearchResults = 10000; // set to a high number because as we are using the ID(480) for Spacewar, there's a big chance to find lots of sessions with this ID
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals); // make sure that every presence we find is using PRESENCE (bUsesPresence)

	// Get a UniqueNetId to use in the FindSession below
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	// the last param SessionSearch in this case was a TSharedPointer, and the function FindSession takes a TSharedRef param, so we convert to TSharedRef by using SssionSearch.ToSharedRef
	OnlineSessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
	
}

void AMenuSystemCharacter::OnFindSessionComplete(bool bWasSuccessful)
{
	if (!OnlineSessionInterface.IsValid()) return;

	// iterate through the SearchResults
	for (auto Result : SessionSearch->SearchResults)
	{
		FString Id = Result.GetSessionIdStr();
		FString UserName = Result.Session.OwningUserName;

		FString MatchType;
		Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.0f,
				FColor::Yellow,
				FString::Printf(TEXT("Id: %s, User: %s"), *Id, *UserName)
			);
		}

		if (MatchType == FString("FreeforAll"))
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					-1,
					15.0f,
					FColor::Orange,
					FString::Printf(TEXT("Joining MatchType: %s"), *MatchType)
				);
			}
			//Add Join SessionComplete delegate to the OnlineSessionInterface delegate list
			OnlineSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegate);

			// Get a UniqueNetId to use in the FindSession below
			const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			OnlineSessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result);
		}
	}
}

void AMenuSystemCharacter::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (!OnlineSessionInterface.IsValid()) return;
	// Address declared in order to store the String from GetResolvedConnectString function below.
	FString Address;
	if (OnlineSessionInterface->GetResolvedConnectString(NAME_GameSession, Address))
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				15.0f,
				FColor::Yellow,
				FString::Printf(TEXT("Connect String: %s"), *Address)
			);
		}

		// get a reference to player controller through GameInstance
		APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();

		if (PlayerController)
		{
			// if playercontroller is valid, call client travel, to travel to the Lobby Map
			PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
		}
	}
}

void AMenuSystemCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AMenuSystemCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AMenuSystemCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMenuSystemCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMenuSystemCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMenuSystemCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}
