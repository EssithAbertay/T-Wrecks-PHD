// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BikeScoreInstanceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, int32, NewScore, int32, ScoreDifference);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMultiplierChanged, int32, NewMultiplier, int32, MultiplierDifference);


UENUM(BlueprintType)
enum class EScoringStage : uint8
{
	SINGLE UMETA(DisplayName = "Single Stage"),
	INFINITE UMETA(DisplayName = "Infinite")
};


UENUM(BlueprintType)
enum class EScoringType : uint8
{
	NOSCORINGTYPE UMETA(DisplayName = "No Scoring Type Selected"),
	OBSTACLEBAD UMETA(DisplayName = "Obstacle Bad"),
	OBSTACELGOOD UMETA(DisplayName = "Obstacle Good"),
	FASTLANE UMETA(DisplayName = "Fast Lane"),
	CHECKPOINT UMETA(DisplayName = "Checkpoint")
};
/**
 * 
 */
UCLASS()
class BIKEPROJECT_API UBikeScoreInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	EScoringStage GetScoringStage() const;

	UFUNCTION(BlueprintCallable)
	void SetScoringStage(EScoringStage p_ScoringStage);

	// score

	UFUNCTION(BlueprintCallable)
	int32 GetScore() const;

	UFUNCTION(BlueprintCallable)
	void ResetScore();


	// only use the increment if type is none
	UFUNCTION(BlueprintCallable)
	void IncrementScore(EScoringType Type, int32 Increment);

	UPROPERTY(BlueprintAssignable)
	FOnScoreChanged ScoreChangedSignal;

	//multiplier

	UFUNCTION(BlueprintCallable)
	int32 GetMultiplier() const;

	UFUNCTION(BlueprintCallable)
	void ResetMultiplier();

	// only use the increment if type is none
	UFUNCTION(BlueprintCallable)
	void IncrementMultiplier(int32 Increment);

	UPROPERTY(BlueprintAssignable)
	FOnMultiplierChanged MultiplierChangedSignal;
private:
	// user doesn't need to set score directly
	void SetScore(int32 p_Score);

	// user doesn't need to set score directly
	void SetMultiplier(int32 Multiplier);

	UPROPERTY(BlueprintGetter = GetScoringStage, BlueprintSetter = SetScoringStage)
	EScoringStage ScoringStage;

	UPROPERTY(BlueprintGetter = GetScore)
	int32 CurrentScore;

	int32 ObstacelGoodValue = 50;
	int32 ObstacleBadValue = -50;
	int32 FastLaneValue = 10;
	int32 CheckpointValue = 500;

	UPROPERTY(BlueprintGetter = GetMultiplier)
	int32 CurrentMultiplier = 1;
	
};
