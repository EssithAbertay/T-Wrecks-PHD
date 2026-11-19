// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BikeScoreComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnScoreChanged, int32, NewScore, int32, ScoreDifference);

UENUM(BlueprintType)
enum class EScoringType : uint8
{
	SINGLE UMETA(DisplayName = "Single Stage"),
	INFINITE UMETA(DisplayName = "Infinite")
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BIKEPROJECT_API UBikeScoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBikeScoreComponent();

	UFUNCTION(BlueprintCallable)
	EScoringType GetScoringType() const;
	UFUNCTION(BlueprintCallable)
	void SetScoringType(EScoringType p_ScoringType);

	UFUNCTION(BlueprintCallable)
	int32 GetScore() const;
	UFUNCTION(BlueprintCallable)
	void SetScore(int32 p_Score);

	UFUNCTION(BlueprintCallable)
	void IncrementScore(int32 p_ScoreIncrement);

	UPROPERTY(BlueprintAssignable)
	FOnScoreChanged ScoreChangedSignal;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UPROPERTY(BlueprintGetter = GetScoringType, BlueprintSetter = SetScoringType)
	EScoringType ScoringType;

	UPROPERTY(BlueprintGetter = GetScore, BlueprintSetter = SetScore)
	int32 CurrentScore;
};
