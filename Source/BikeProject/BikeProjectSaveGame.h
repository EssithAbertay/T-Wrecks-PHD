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
struct FUserAccount
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
    FString PlayerName;
    
    UPROPERTY(VisibleAnywhere, Category = Basic)
    FString SaveSlotName;

    // any data we need easy access to (for leaderboards etc, store here as well)

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    int HighScore = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
    FColor UserColor = FColor::White;
};


UCLASS()
class BIKEPROJECT_API UBikeProjectSaveGame : public USaveGame
{
	GENERATED_BODY()
	
public:

    UPROPERTY(VisibleAnywhere, Category = Basic)
    TArray<FUserAccount> UserAccounts;

    UBikeProjectSaveGame();
};
