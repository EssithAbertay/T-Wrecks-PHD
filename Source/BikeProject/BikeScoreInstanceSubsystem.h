// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BikeScoreInstanceSubsystem.generated.h"


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
	OBSTACLEBADDODGE UMETA(DisplayName = "Obstacle Bad Dodge"),
	OBSTACELGOOD UMETA(DisplayName = "Obstacle Good"),
	SLOWLANE UMETA(DisplayName = "Slow Lane"),
	MEDIUMLANE UMETA(DisplayName = "Medium Lane"),
	FASTLANE UMETA(DisplayName = "Fast Lane"),
	CHECKPOINT UMETA(DisplayName = "Checkpoint")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnScoreChanged, int32, NewScore, int32, ScoreDifference, EScoringType, ScoringType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMultiplierChanged, int32, NewMultiplier, int32, MultiplierDifference);
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
	void IncrementScore(EScoringType Type);

	// just for odd situations where a weird amount of score needs added, idrk where this would be needed
	UFUNCTION(BlueprintCallable)
	void IncrementScoreByAmount(int32 Increment);

	UPROPERTY(BlueprintAssignable)
	FOnScoreChanged ScoreChangedSignal;

	UFUNCTION(BlueprintCallable)
	void IncreaseLaneTime(int Lane, float Time);

	UFUNCTION(BlueprintCallable)
	void ResetLaneTime();

	UFUNCTION(BlueprintCallable)
	float GetLaneTime(int Lane) const;


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

	int32 ObstacelGoodValue = 100;
	int32 ObstacleBadValue = -50;
	int32 ObstacleBadDodgeValue = 50;
	int32 SlowLaneValue = 1;
	int32 MediumLaneValue = 5;
	int32 FastLaneValue = 10;
	int32 CheckpointValue = 500;

	int CalculateCheckpointScore();

	// I wonder if the lane times should be stored in the bike character 
	TStaticArray<float, 3> LaneTimes;
	TArray<int> LaneTimeValues = {5, 15, 25}; // CANT USE INITLAISER FOR STATIC ARRAY IDK ITS DUMB, but shouldn't matter as we won't change this

	double SlowLaneCheckpointValue = 5;
	double MediumLaneCheckpointValue = 15;
	double FastLaneCheckpointValue = 25;


	UPROPERTY(BlueprintGetter = GetMultiplier)
	int32 CurrentMultiplier = 1;
};
