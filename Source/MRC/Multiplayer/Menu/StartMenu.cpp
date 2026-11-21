// Fill out your copyright notice in the Description page of Project Settings.


#include "StartMenu.h"
#include "Components\Button.h"
#include "MRC\Multiplayer\Subsystem\MRC_GIS.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"
#include "Interfaces/OnlineSessionInterface.h"

/// Must match the MatchType passed to CreateSession in OnStartGameClicked
static const TCHAR* GMatchType = TEXT("ProMultiplayerGame");

void UStartMenu::StartMenu()
{
	AddToViewport();
	SetVisibility(ESlateVisibility::Visible);
	SetIsFocusable(true);

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(true);
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		SessionSubsystem = GameInstance->GetSubsystem<UMRC_GIS>();

		if (SessionSubsystem)
		{
			/// Remove first so calling StartMenu() more than once never double-binds
			SessionSubsystem->MultiplayerFindSessionsCompleteDelegate.RemoveAll(this);
			SessionSubsystem->MultiplayerJoinSessionCompleteDelegate.RemoveAll(this);

			SessionSubsystem->MultiplayerCreateSessionCompleteDelegate.AddUniqueDynamic(this, &UStartMenu::OnMultiplayerCreateSession);
			SessionSubsystem->MultiplayerFindSessionsCompleteDelegate.AddUObject(this, &UStartMenu::OnMultiplayerFindSessionsComplete);
			SessionSubsystem->MultiplayerJoinSessionCompleteDelegate.AddUObject(this, &UStartMenu::OnMultiplayerJoinSessionComplete);
		}
	}
}




bool UStartMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (Start_btn)
	{
		Start_btn->OnClicked.AddUniqueDynamic(this, &UStartMenu::OnStartGameClicked);
	}

	if (Join_btn)
	{
		Join_btn->OnClicked.AddUniqueDynamic(this, &UStartMenu::OnJoinGameClicked);
	}

	if (bCustomSessionID)
	{
		bCustomSessionID->OnCheckStateChanged.AddUniqueDynamic(this, &UStartMenu::OnCustomSessionClicked);
	}

	return true;

}

void UStartMenu::RemoveMenu()
{
	RemoveFromParent();
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputMode;
			PlayerController->SetInputMode(InputMode);
			PlayerController->SetShowMouseCursor(false);
		}
	}
}

void UStartMenu::OnStartGameClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("OnStartGameClicked widget=%p"), this);

	if(SessionSubsystem)
	{

		int32 NumbPlayers = FCString::Atoi(*NumberOfPlayers->GetSelectedOption());
		/// Prevent double clicks creating two sessions
		if (Start_btn) Start_btn->SetIsEnabled(false);
		SessionSubsystem->CreateSession(NumbPlayers, GMatchType);

	}

}

void UStartMenu::OnCustomSessionClicked(bool bIsChecked)
{
	NewSessionID->SetIsReadOnly(!bIsChecked);
}

void UStartMenu::OnJoinGameClicked()
{
	if(SessionSubsystem)
	{
		if (Join_btn)
		{
			Join_btn->SetIsEnabled(false);
		}
		SessionSubsystem->FindSessions();
	}
}

void UStartMenu::OnMultiplayerCreateSession(bool bWasSuccesfull)
{
	if (bWasSuccesfull)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Multiplayer Session Created"));
		}

		if (UWorld* world = GetWorld())
		{
			world->ServerTravel(MapLocation);
			RemoveMenu();
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Failed to create session"));
		}
		if (Start_btn) Start_btn->SetIsEnabled(true);
	}
}

void UStartMenu::OnMultiplayerFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful)
{
	if (!SessionSubsystem) return;

	UE_LOG(LogTemp, Warning, TEXT("Find complete: success=%d results=%d"), bWasSuccessful, SearchResults.Num());

	if (bWasSuccessful)
	{
		for (const FOnlineSessionSearchResult& Result : SearchResults)
		{
			FString MatchType;
			Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);
			UE_LOG(LogTemp, Warning, TEXT("Found session, MatchType='%s'"), *MatchType);

			if (bCustomSessionID->IsChecked())
			{

				if (MatchType == (MatchTypeName + NewSessionID->GetText().ToString()))
				{
					SessionSubsystem->JoinSession(Result);
					return;
				}

				///*FString SessionID;
				//Result.Session.SessionSettings.Get(FName("SessionID"), SessionID);
				//UE_LOG(LogTemp, Warning, TEXT("Found session, SessionID='%s'"), *SessionID);*/
			}
			else
			{
				if (MatchType == FString(GMatchType))
				{
					SessionSubsystem->JoinSession(Result);
					return;   // join only the first match
				}
			}

			
		}
	}

	/// Nothing found / failed: allow the user to try again
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("No session found"));
	}
	if (Join_btn) Join_btn->SetIsEnabled(true);
}

void UStartMenu::OnMultiplayerJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("Join failed: %d"), (int32)Result);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Join failed"));
		}
		if (Join_btn) Join_btn->SetIsEnabled(true);
		return;
	}

	if(IOnlineSubsystem* OnlineSubsystemPtr = IOnlineSubsystem::Get())
	{
		if (IOnlineSessionPtr SessionInterface = OnlineSubsystemPtr->GetSessionInterface())
		{
			FString MatchAddress;
			SessionInterface->GetResolvedConnectString(NAME_GameSession, MatchAddress);
			UE_LOG(LogTemp, Warning, TEXT("Join success, connect string='%s'"), *MatchAddress);

			if (MatchAddress.IsEmpty())
			{
				if (Join_btn) Join_btn->SetIsEnabled(true);
				return;
			}

			if(APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
			{
				RemoveMenu();
				PlayerController->ClientTravel(MatchAddress, ETravelType::TRAVEL_Absolute);

			}
		}
	}

}
