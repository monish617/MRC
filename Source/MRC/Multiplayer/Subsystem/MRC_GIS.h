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
	IOnlineSubsystem* OnlineSubsystem;
	IOnlineSessionPtr SessionInterface;

	
};
