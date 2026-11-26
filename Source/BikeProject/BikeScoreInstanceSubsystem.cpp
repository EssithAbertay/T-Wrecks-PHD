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

void UBikeScoreInstanceSubsystem::IncrementScore(EScoringType Type)
{
	int32 Increment = 0;

	switch (Type)
	{
	case EScoringType::NOSCORINGTYPE:
		break;
	case EScoringType::OBSTACLEBAD:
		Increment = ObstacleBadValue;
		break;
	case EScoringType::OBSTACLEBADDODGE:
		Increment = ObstacleBadDodgeValue;
		break;
	case EScoringType::OBSTACELGOOD:
		Increment = ObstacelGoodValue;
		break;
	case EScoringType::SLOWLANE:
		Increment = SlowLaneValue;
		break;
	case EScoringType::MEDIUMLANE:
		Increment = MediumLaneValue;
		break;
	case EScoringType::FASTLANE:
		Increment = FastLaneValue;
		break;
	case EScoringType::CHECKPOINT:
		Increment = CalculateCheckpointScore();
		ResetLaneTime();
		break;
	default:
		break;
	}

	SetScore(CurrentScore + Increment);
	ScoreChangedSignal.Broadcast(CurrentScore, Increment, Type);
}

void UBikeScoreInstanceSubsystem::IncrementScoreByAmount(int32 Increment)
{
	SetScore(CurrentScore + Increment);
	ScoreChangedSignal.Broadcast(CurrentScore, Increment, EScoringType::NOSCORINGTYPE);
}

int UBikeScoreInstanceSubsystem::CalculateCheckpointScore()
{
	int CheckpointScoreToAward = 0;

	float TotalLaneTime = 0;
		
	for (int32 i = 0; i < LaneTimes.Num(); ++i)
	{
		TotalLaneTime += LaneTimes[i];
	}

	// take proportion of time spent in each lane, multiply by its base value, then total those
	double TimeProportionInLane = 0;
	for (int32 i = 0; i < LaneTimes.Num(); ++i)
	{
		TimeProportionInLane = (LaneTimes[i] / TotalLaneTime) * 100;
		CheckpointScoreToAward += TimeProportionInLane * LaneTimeValues[i];
		UE_LOG(LogTemp, Log, TEXT("Time Proportion: %f"), TimeProportionInLane);
	}

	return CheckpointScoreToAward;
}

void UBikeScoreInstanceSubsystem::IncreaseLaneTime(int Lane, float Time)
{
	LaneTimes[Lane] += Time;
}

void UBikeScoreInstanceSubsystem::ResetLaneTime()
{
	for (int32 i = 0; i < LaneTimes.Num(); ++i)
	{
		LaneTimes[i] = 0;
	}
}

float UBikeScoreInstanceSubsystem::GetLaneTime(int Lane) const
{
	return LaneTimes[Lane];
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
