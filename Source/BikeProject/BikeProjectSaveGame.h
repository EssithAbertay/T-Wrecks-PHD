// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BikeProjectStatsStruct.h"
#include "BikeProjectSaveGame.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FDeviceReconnection
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    bool AutoReconnectEnabled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    FString FrameAddress = "FrameDefault";

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    FString TrainerAddress = "TrainerDefault";

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    FString HRMonitorAddress = "HRMonitorDefault";
};



USTRUCT(BlueprintType)
struct FUserAccount
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FString PlayerName;
    
    UPROPERTY(VisibleAnywhere, Category = Basic)
    FString SaveSlotName;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    FColor UserColor = FColor::White;
    // any data we need easy access to (for leaderboards etc, store here as well)
    // these variables are stored twice, here, and in the actual save file 
    //(the same comment is there as well), this is not the source of truth, these are only to be used for the login page and other places where we dont load the save files of other users, whenever you nee the actual values please use the version from BikeProjectStatsStruct

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    int HighScore = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    int CurrentRank = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    int TotalEnergy = 0;
};


UCLASS()
class BIKEPROJECT_API UBikeProjectSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:
    UPROPERTY(VisibleAnywhere, Category = Basic)
    TArray<FUserAccount> UserAccounts;

    UPROPERTY(VisibleAnywhere, Category = Basic)
    FDeviceReconnection DeviceReconnection;
    
    UBikeProjectSaveGame();
};
