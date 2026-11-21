// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint\UserWidget.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "StartMenu.generated.h"

/**
 * 
 */
//Forward Declaration
class UButton;
class UComboBoxString;
class UEditableTextBox;
class UCheckBox;
class UMRC_GIS;
class FOnlineSessionSearchResult;

UCLASS()
class MRC_API UStartMenu :public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "StartMenu")
	void StartMenu();

protected:
	virtual bool Initialize() override;

private:
	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UButton> Start_btn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Join_btn;

	UPROPERTY(EditAnywhere,Category ="Startmenu")
	TObjectPtr<UMRC_GIS> SessionSubsystem;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> NumberOfPlayers;

	UPROPERTY(meta =(BindWidget))
	TObjectPtr<UCheckBox> bCustomSessionID;

	UPROPERTY(meta = (BindWidget))	
	TObjectPtr<UEditableTextBox> NewSessionID;


	FString MatchTypeName{ TEXT("ProMultiplayerGame") };

	FString MapLocation{TEXT("/Game/Maps/Lobby?Listen")};


	UFUNCTION()
	void OnStartGameClicked();

	UFUNCTION()
	void OnCustomSessionClicked(bool bIsChecked);


	UFUNCTION()
	void OnJoinGameClicked();

	void RemoveMenu();

	///Delegate Callback functions

	UFUNCTION()
	void OnMultiplayerCreateSession(bool bWasSuccesfull);

	void OnMultiplayerFindSessionsComplete(const TArray<FOnlineSessionSearchResult>& SearchResults, bool bWasSuccessful);

	void OnMultiplayerJoinSessionComplete(EOnJoinSessionCompleteResult::Type Result);
	
	

};
