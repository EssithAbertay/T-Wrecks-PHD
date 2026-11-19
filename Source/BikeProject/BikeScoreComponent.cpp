// Fill out your copyright notice in the Description page of Project Settings.


#include "BikeScoreComponent.h"


// Sets default values for this component's properties
UBikeScoreComponent::UBikeScoreComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

EScoringType UBikeScoreComponent::GetScoringType() const
{
	return ScoringType;
}

void UBikeScoreComponent::SetScoringType(EScoringType p_ScoringType)
{
	ScoringType = p_ScoringType;
}

int32 UBikeScoreComponent::GetScore() const
{
	return CurrentScore;
}

void UBikeScoreComponent::SetScore(int32 p_Score)
{
	int32 Difference = p_Score - CurrentScore;

	if (Difference == 0)
	{
		return;
	}

	CurrentScore = p_Score;

	ScoreChangedSignal.Broadcast(CurrentScore, Difference);
}

void UBikeScoreComponent::IncrementScore(int32 p_ScoreIncrement)
{
	SetScore(CurrentScore + p_ScoreIncrement);
}



// Called when the game starts
void UBikeScoreComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UBikeScoreComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

