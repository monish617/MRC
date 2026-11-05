// Fill out your copyright notice in the Description page of Project Settings.


#include "MRC_GIS.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

UMRC_GIS::UMRC_GIS() : OnCreateSessionCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this,&ThisClass::OnCreateSessionComplete)),
                       OnFindSessionCompleteDelegate(FOnFindSessionsCompleteDelegate::CreateUObject(this,&ThisClass::OnFindSessionsComplete)),
	                   OnJoinSessionCompleteDelegate(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete))
{
	OnlineSubsystem = IOnlineSubsystem::Get(); 
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, ("OnlineSubsystem: " + OnlineSubsystem->GetSubsystemName().ToString()));
		}
	}

}


void UMRC_GIS::MCreateSession()
{
	if (!SessionInterface.IsValid()) return;
	  
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession !=nullptr)
	
	SessionInterface->DestroySession(NAME_GameSession);	

	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);

	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());

	SessionSettings->bIsLANMatch = true;
	SessionSettings->NumPublicConnections = 10;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;	
	SessionSettings->bAllowInvites = true;	
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->Set(FName("MatchType"), FString("FreeForAll"),EOnlineDataAdvertisementType::ViaOnlineServiceAndPing); 


	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();

	SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,*SessionSettings);

}

void UMRC_GIS::MJoinSession()
{
	if (!SessionInterface.IsValid()) return;

	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionCompleteDelegate);

	SessionSearchSettings = MakeShareable(new FOnlineSessionSearch());
	SessionSearchSettings->MaxSearchResults = 10000;
	SessionSearchSettings->bIsLanQuery = true;
	SessionSearchSettings->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearchSettings.ToSharedRef());
}

 void UMRC_GIS::OnCreateSessionComplete(FName SessionName, bool bWasSucessfull)
{
	if (bWasSucessfull)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, "Session Created Successfully");
		}

		if (UWorld* World = GetWorld())
		{
			World->ServerTravel("/Game/Maps/Lobby?listen");
		}
	}
	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, "Session Creation Failed");
		}
	}
}
 void UMRC_GIS::OnFindSessionsComplete(bool bWasSucessfull)
{
	if (!SessionInterface.IsValid()) return;
	if (bWasSucessfull)
	{
		for (auto Result : SessionSearchSettings->SearchResults)
		{

			FString Id = Result.GetSessionIdStr();
			FString User = Result.Session.OwningUserName;

			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, FString::Printf(TEXT("Session Id: %s,user:%s"), *Id, *User));
			}

			FString MatchType;
			Result.Session.SessionSettings.Get(FName("MatchType"), MatchType);
			if (MatchType == FString("FreeForAll"))
			{
				Result.Session.SessionSettings.bUseLobbiesIfAvailable = true;
				Result.Session.SessionSettings.bUsesPresence = true;
				SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
				const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
				SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(),NAME_GameSession,Result);
			}

				
		}
	}
}

 void UMRC_GIS::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
 {

	 if (!SessionInterface.IsValid()) return;

	 FString MatchAddress;
	 SessionInterface->GetResolvedConnectString(NAME_GameSession, MatchAddress);
	// UE_LOG(LogTemp, Warning, TEXT("Address Resolved: %s | Address: %s"), bResolved ? TEXT("TRUE") : TEXT("FALSE"), *MatchAddress);

	 if(GEngine)
	 {
		 GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Yellow, FString::Printf(TEXT("Joining Session at %s"), *MatchAddress));
	 }
	 if (APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController())
	 {
		 UE_LOG(LogTemp, Warning, TEXT("[GIS] Calling ClientTravel: %s"), *MatchAddress);
			 	 
		 PlayerController->ClientTravel(MatchAddress, ETravelType::TRAVEL_Absolute);

	 }  
	 else
	 {
		 UE_LOG(LogTemp, Warning, TEXT("[GIS] PlayerController is Invalid"));
	 }

	 UE_LOG(LogTemp, Warning, TEXT("Join Result: %d"), (int32)Result);
		
	 	 
	  

 }

