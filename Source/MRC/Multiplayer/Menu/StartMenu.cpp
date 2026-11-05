// Fill out your copyright notice in the Description page of Project Settings.


#include "StartMenu.h"
#include "Components\Button.h"
#include "MRC\Multiplayer\Subsystem\MRC_GIS.h"

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
			SessionSubsystem->MultiplayerCreateSessionCompleteDelegate.AddDynamic(this, &UStartMenu::OnMultiplayerCreateSession);
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
		Start_btn->OnClicked.AddDynamic(this, &UStartMenu::OnStartGameClicked);
	}
	
	if (Join_btn)
	{
		Join_btn->OnClicked.AddDynamic(this, &UStartMenu::OnJoinGameClicked);
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
	if(SessionSubsystem)
	{
		SessionSubsystem->CreateSession(4, "TestMatch");
	}

}

void UStartMenu::OnJoinGameClicked()
{
	if(SessionSubsystem)
	{
		SessionSubsystem->FindSessions();
		if (Join_btn)
		{
			Join_btn->SetIsEnabled(false);
		}
	
		//SessionSubsystem->JoinSession();
	}
}

void UStartMenu::OnMultiplayerCreateSession(bool bWasSuccesfull)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Multiplayer Session Created"));

	}

	if (bWasSuccesfull)
	{
		if (UWorld* world = GetWorld())
		{
			world->ServerTravel(MapLocation);
			RemoveMenu();
			
		}
	}
}
