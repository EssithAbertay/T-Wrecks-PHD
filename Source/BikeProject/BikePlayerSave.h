// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BikeProjectStatsStruct.h"
#include "BikePlayerSave.generated.h"

/**
 * 
 */
UCLASS()
class BIKEPROJECT_API UBikePlayerSave : public USaveGame
{
	GENERATED_BODY()

public:

    UPROPERTY(VisibleAnywhere, Category = Basic)
    FPlayerStats SaveStats;

    UBikePlayerSave();
};
