// Fill out your copyright notice in the Description page of Project Settings.


#include "BikeScoreInstanceSubsystem.h"

EScoringStage UBikeScoreInstanceSubsystem::GetScoringStage() const
{
	return ScoringStage;
}

void UBikeScoreInstanceSubsystem::SetScoringStage(EScoringStage p_ScoringStage)
{
	ScoringStage = p_ScoringStage;
}

int32 UBikeScoreInstanceSubsystem::GetScore() const
{
	return CurrentScore;
}

void UBikeScoreInstanceSubsystem::ResetScore()
{
	SetScore(0);
}


void UBikeScoreInstanceSubsystem::SetScore(int32 p_Score)
{
	CurrentScore = p_Score;
	if (CurrentScore < 0)
	{
		CurrentScore = 0;
	}
}

void UBikeScoreInstanceSubsystem::IncrementScore(EScoringType Type, int32 ScoreIncrement)
{
	int32 Increment = 0;

	switch (Type)
	{
	case EScoringType::NOSCORINGTYPE:
		Increment = ScoreIncrement;
		break;
	case EScoringType::OBSTACLEBAD:
		Increment = ObstacleBadValue;
		break;
	case EScoringType::OBSTACELGOOD:
		Increment = ObstacelGoodValue;
		break;
	case EScoringType::FASTLANE:
		Increment = FastLaneValue;
		break;
	case EScoringType::CHECKPOINT:
		Increment = CheckpointValue;
		break;
	default:
		break;
	}

	SetScore(CurrentScore + Increment);
	ScoreChangedSignal.Broadcast(CurrentScore, Increment);
}

void UBikeScoreInstanceSubsystem::IncrementMultiplier(int32 Increment)
{
	SetMultiplier(CurrentMultiplier + Increment);
	MultiplierChangedSignal.Broadcast(CurrentMultiplier, Increment);
}

int32 UBikeScoreInstanceSubsystem::GetMultiplier() const
{
	return CurrentMultiplier;
}

void UBikeScoreInstanceSubsystem::ResetMultiplier()
{
	// minimum is 1
	SetMultiplier(1);
}

void UBikeScoreInstanceSubsystem::SetMultiplier(int32 Multiplier)
{
	CurrentMultiplier = Multiplier;

	// minimum multiplier is 1
	if (CurrentMultiplier == 0)
	{
		CurrentMultiplier = 1;
	}
}
