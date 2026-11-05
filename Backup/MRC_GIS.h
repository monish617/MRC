// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "MRC_GIS.generated.h"

// Forward Declaration
class IOnlineSubsystem;
UCLASS()
class MRC_API UMRC_GIS : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UMRC_GIS();
private:

	///Multiplayer Subsystem variables
	IOnlineSubsystem* OnlineSubsystem;
	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearchSettings;

protected:
	UFUNCTION(BlueprintCallable,Category="Multiplayer")
	void MCreateSession();

	UFUNCTION(BlueprintCallable, Category = "Multiplayer")
	void MJoinSession();



	//Delegate call back function
	void OnCreateSessionComplete(FName SessionName, bool bWasSucessfull);
	void OnFindSessionsComplete(bool bWasSucessfull);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);


private:
	///Multiplayer Delegates
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	FOnFindSessionsCompleteDelegate OnFindSessionCompleteDelegate;
	FOnJoinSessionCompleteDelegate OnJoinSessionCompleteDelegate;






	
};
