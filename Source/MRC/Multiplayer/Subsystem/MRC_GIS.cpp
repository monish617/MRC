// Fill out your copyright notice in the Description page of Project Settings.


#include "MRC_GIS.h"
#include "OnlineSubsystem.h"

UMRC_GIS::UMRC_GIS()
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
