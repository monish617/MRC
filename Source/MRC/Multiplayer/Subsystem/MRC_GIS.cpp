// Fill out your copyright notice in the Description page of Project Settings.


#include "MRC_GIS.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online\OnlineSessionNames.h"
#include "Interfaces\OnlineSessionInterface.h"
#include "Interfaces\OnlineIdentityInterface.h"

/// The NULL subsystem can only do LAN sessions. Steam (or any other online service) uses presence/lobbies.
static bool ShouldUseLan(IOnlineSubsystem* Subsystem)
{
	return Subsystem && Subsystem->GetSubsystemName() == FName(TEXT("NULL"));
}

UMRC_GIS::UMRC_GIS() :
	OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete)),
	OnFindSessionsCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete)),
	OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete)),
	OnDestroySessionCompleteDelegate(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete))
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Red, OnlineSubsystem->GetSubsystemName().ToString());
		}
	}
}

void UMRC_GIS::CreateSession(int32 NumPlayers, FString MatchType)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerCreateSessionCompleteDelegate.Broadcast(false);
		return;
	}

	/// An old session must be fully destroyed before a new one can be created.
	/// DestroySession is asynchronous, so remember the request and create from OnDestroySessionComplete.
	if (SessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		bCreateSessionOnDestroy = true;
		PendingNumPlayers = NumPlayers;
		PendingMatchType = MatchType;

		if (OnDestroySessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		}
		OnDestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegate);

		if (!SessionInterface->DestroySession(NAME_GameSession))
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
			bCreateSessionOnDestroy = false;
			MultiplayerCreateSessionCompleteDelegate.Broadcast(false);
		}
		return;
	}

	/// Never keep a stale binding (this is what caused callbacks to fire twice)
	if (OnCreateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}
	OnCreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	const bool bLan = ShouldUseLan(OnlineSubsystem);

	/// MatchType is advertised EXACTLY as passed in, so the joiner can filter on it.
	/// The unique, human-readable name is stored under a separate key.
	CurrentSessionUniqueName = MatchType + GenerateUniqueSessionId();

	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	SessionSettings->bIsLANMatch = bLan;
	SessionSettings->NumPublicConnections = NumPlayers;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = !bLan;
	SessionSettings->bUsesPresence = !bLan;
	SessionSettings->bUseLobbiesIfAvailable = !bLan;
	SessionSettings->Set(FName("MatchType"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings->Set(FName("SessionUniqueName"), CurrentSessionUniqueName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	UE_LOG(LogTemp, Warning, TEXT("CreateSession: subsystem=%s LAN=%d MatchType='%s' Unique='%s'"),
		OnlineSubsystem ? *OnlineSubsystem->GetSubsystemName().ToString() : TEXT("none"), bLan, *MatchType, *CurrentSessionUniqueName);

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		MultiplayerCreateSessionCompleteDelegate.Broadcast(false);
	}
}

void UMRC_GIS::FindSessions()
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerFindSessionsCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}

	if (OnFindSessionsCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	}
	OnFindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);

	const bool bLan = ShouldUseLan(OnlineSubsystem);

	SessionSearchSettings = MakeShareable(new FOnlineSessionSearch());
	SessionSearchSettings->MaxSearchResults = 10000;
	SessionSearchSettings->bIsLanQuery = bLan;
	if (!bLan)
	{
		/// LOBBYSEARCH tells the Steam subsystem to search Steam lobbies. Without it Steam searches the
		/// dedicated-server list instead, and a lobby created with bUseLobbiesIfAvailable is never returned.
		SessionSearchSettings->QuerySettings.Set(FName(TEXT("LOBBYSEARCH")), true, EOnlineComparisonOp::Equals);
		SessionSearchSettings->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals);
	}

	UE_LOG(LogTemp, Warning, TEXT("FindSessions: subsystem=%s LAN=%d"),
		OnlineSubsystem ? *OnlineSubsystem->GetSubsystemName().ToString() : TEXT("none"), bLan);

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearchSettings.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
		MultiplayerFindSessionsCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
	}
}

void UMRC_GIS::JoinSession(const FOnlineSessionSearchResult& SessionSearchResult)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
		return;
	}

	const bool bLan = ShouldUseLan(OnlineSubsystem);

	FOnlineSessionSearchResult Result = SessionSearchResult;
	Result.Session.SessionSettings.bUseLobbiesIfAvailable = !bLan;
	Result.Session.SessionSettings.bUsesPresence = !bLan;

	if (OnJoinSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
	}
	OnJoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	if (!LocalPlayer || !SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, Result))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		MultiplayerJoinSessionCompleteDelegate.Broadcast(EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UMRC_GIS::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
		OnCreateSessionCompleteDelegateHandle.Reset();
	}
	MultiplayerCreateSessionCompleteDelegate.Broadcast(bWasSuccessful);

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 50.f, FColor::Green, FString::Printf(TEXT("Session Name: %s"), *CurrentSessionUniqueName));

}

void UMRC_GIS::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (!SessionInterface.IsValid())
	{
		MultiplayerFindSessionsCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), false);
		return;
	}
	SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);
	OnFindSessionsCompleteDelegateHandle.Reset();

	if (!SessionSearchSettings.IsValid() || SessionSearchSettings->SearchResults.Num() <= 0)
	{
		MultiplayerFindSessionsCompleteDelegate.Broadcast(TArray<FOnlineSessionSearchResult>(), bWasSuccessful);
		return;
	}
	MultiplayerFindSessionsCompleteDelegate.Broadcast(SessionSearchSettings->SearchResults, bWasSuccessful);

}

void UMRC_GIS::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		OnJoinSessionCompleteDelegateHandle.Reset();
	}
	MultiplayerJoinSessionCompleteDelegate.Broadcast(Result);
}

void UMRC_GIS::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		OnDestroySessionCompleteDelegateHandle.Reset();
	}

	if (bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		if (bWasSuccessful)
		{
			CreateSession(PendingNumPlayers, PendingMatchType);
		}
		else
		{
			MultiplayerCreateSessionCompleteDelegate.Broadcast(false);
		}
	}
}

FString UMRC_GIS::GenerateUniqueSessionId() const
{

	/// Player Name + _ + Time Now
	FString PlayerSteamName = FString("");

	if (IOnlineSubsystem* OnlineSubsystemSteam = IOnlineSubsystem::Get(STEAM_SUBSYSTEM))
	{
		IOnlineIdentityPtr Identity = OnlineSubsystemSteam->GetIdentityInterface();
		if (Identity.IsValid())
		{
			const FUniqueNetIdPtr LocalPlayerID = Identity->GetUniquePlayerId(0);
			if (LocalPlayerID.IsValid())
				PlayerSteamName = Identity->GetPlayerNickname(*LocalPlayerID);
		}
	}

	int64 Timestamp = FDateTime::Now().ToUnixTimestamp();
	FString TimestampString = FString::Printf(TEXT("%04d"), Timestamp % 10000);

	return PlayerSteamName + TEXT("_") + TimestampString;
}
