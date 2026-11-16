// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "GameFramework/GameState.h"

void ALobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	int32 NumberofPlayers = GameState.Get()->PlayerArray.Num();
	if (NumberofPlayers >= 2)
	{
		if (UWorld* World = GetWorld())
		{

			World->ServerTravel(FString("/Game/Maps/RaceMap?listen"));
		}
	}
}
