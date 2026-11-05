// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint\UserWidget.h"
#include "StartMenu.generated.h"

/**
 * 
 */
//Forward Declaration
class UButton;
class UMRC_GIS;

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


	FString MapLocation{TEXT("/Game/Maps/Lobby?Listen")};


	UFUNCTION()
	void OnStartGameClicked();

	UFUNCTION()
	void OnJoinGameClicked();

	///Delegate Callback functions

	UFUNCTION()
	void OnMultiplayerCreateSession(bool bWasSuccesfull);
	
	void RemoveMenu();

};
