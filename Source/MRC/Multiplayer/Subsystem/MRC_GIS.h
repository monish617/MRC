// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems\GameInstanceSubsystem.h"
#include "Interfaces\OnlineSessionInterface.h"
#include "MRC_GIS.generated.h"

/// Forward Declaration
class IOnlineSubsystem;

/// Our Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMultiplayerCreateSessionCompleteDelegate, bool, bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_TwoParams(FMultiplayerFindSessionsCompleteDelegate, const TArray<FOnlineSessionSearchResult>& SessionResults, bool bWasSuccessful);
DECLARE_MULTICAST_DELEGATE_OneParam(FMultiplayerJoinSessionCompleteDelegate, EOnJoinSessionCompleteResult::Type Result);


/**
 *
 */
UCLASS()
class MRC_API UMRC_GIS : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMRC_GIS();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void CreateSession(int32 NumPlayers, FString MatchType);

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void FindSessions();

	// UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void JoinSession(const FOnlineSessionSearchResult& SessionSearchResult);

	/// Our Delegate
	FMultiplayerCreateSessionCompleteDelegate MultiplayerCreateSessionCompleteDelegate;
	FMultiplayerFindSessionsCompleteDelegate MultiplayerFindSessionsCompleteDelegate;
	FMultiplayerJoinSessionCompleteDelegate MultiplayerJoinSessionCompleteDelegate;

protected:
	/// Delegate Callback functions
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

private:
	/// Multiplayer Subsystem Variables
	IOnlineSubsystem* OnlineSubsystem;
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearchSettings;

	UPROPERTY(BlueprintReadOnly, Category = "MultiplayerSessionsSubsystem", meta = (AllowPrivateAccess = "true"))
	FString CurrentSessionUniqueName{ FString() };

	/// Multiplayer Delegates
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;

	/// Generate Custom Session ID
	FString GenerateUniqueSessionId() const;

};