// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedStruct.h"
#include "BikeProjectStatsStruct.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FPlayerStats
{
    GENERATED_BODY()


    // Use UPROPERTY() to decorate member variables as they allow for easier integration with network replication as well as potential garbage collection processing
    UPROPERTY()
    float PlayerMaxPower[3]; // DEPRECATED, only for old systems, now please use maxbluetoothpower

    // the above uses an array for mobile/physical/keyboard, im storing bluetooth independently so that I can untether from it more easily
    UPROPERTY()
    float PlayerMaxBluetoothPower;

    UPROPERTY(BlueprintReadWrite)
    float TotalDistanceRan;
    UPROPERTY(BlueprintReadWrite)
    int TotalCurrency;
    UPROPERTY()
    int CostumeStatus[3];

    UPROPERTY(BlueprintReadWrite)
    int MainGameModeCount;
    UPROPERTY(BlueprintReadWrite)
    int EndlessGameModeCount;
    UPROPERTY(BlueprintReadWrite)
    int StagesComplete;
    UPROPERTY(BlueprintReadWrite)
    int BossChunks;
    

    //default values of everything
    FPlayerStats()
    {        
        PlayerMaxPower[0] = 0;
        PlayerMaxPower[1] = 0;
        PlayerMaxPower[2] = 0;
        TotalDistanceRan = 0;
        TotalCurrency = 0;
        CostumeStatus[0] = 2;
        CostumeStatus[1] = 0;
        CostumeStatus[2] = 0;
        MainGameModeCount = 0;
        EndlessGameModeCount = 0;
        StagesComplete = 0;
        BossChunks = 0;
    }
};
