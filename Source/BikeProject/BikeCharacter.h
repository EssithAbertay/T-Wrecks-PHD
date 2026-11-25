// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

#include "BikeMovementComponent.h"
#include "BikeLaneActor.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h" 

#include "BikeCharacter.generated.h"

UCLASS()
class BIKEPROJECT_API ABikeCharacter : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABikeCharacter();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void DestroySelf();

protected:
	// Variables
	UPROPERTY(EditAnywhere)
	float CameraDistance;
	UPROPERTY(EditAnywhere)
	float FOVBase;
	UPROPERTY(EditAnywhere)
	float FOVMultiplier;
	UPROPERTY(EditAnywhere)
	float PPMed;
	UPROPERTY(EditAnywhere)
	float PPAlphaMult;
	UPROPERTY(BlueprintGetter = GetPostProcessAlpha)
	float PPAlpha;

	UPROPERTY()
	float CURRENTPOWER;
	UPROPERTY()
	float MAXPOWER;
	UPROPERTY()
	float UPPERPOWER;
	UPROPERTY()
	float MIDDLEPOWER;
	UPROPERTY()
	float LOWERPOWER;
	UPROPERTY(EditAnywhere)
	float UpperPercent;
	UPROPERTY(EditAnywhere)
	float MiddlePercent;
	UPROPERTY(EditAnywhere)
	float LowerPercent;

	UPROPERTY(EditAnywhere)
	float LaneWidth;
	UPROPERTY(EditAnywhere)
	float LaneSpeed;
	UPROPERTY(EditAnywhere)
	float SpeedBase;
	UPROPERTY(EditAnywhere)
	float SpeedMultiplier;
	UPROPERTY()
	int PowerLane;
	UPROPERTY(BlueprintGetter = GetLaneSwitching)
	bool LaneSwitching;
	UPROPERTY()
	bool LaneBlocked;

	UPROPERTY()
	ABikeLaneActor* BikeLanes;
	UPROPERTY()
	FVector EasyLanePos;
	UPROPERTY()
	FVector MedLanePos;
	UPROPERTY()
	FVector HardLanePos;

	UPROPERTY(VisibleAnywhere)
	UCapsuleComponent* CapsuleComponent;
	UPROPERTY(BlueprintReadWrite)
	USkeletalMeshComponent* PlayerVisibleComponent;

	UPROPERTY(VisibleAnywhere)
	USpringArmComponent* PlayerCameraSpringArm;
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* PlayerCamera;

	// Movement component.
	UPROPERTY()
	class UBikeMovementComponent* MovementComponent;
	UPROPERTY()
	FVector IntendedMovement;

	UFUNCTION()
	void PostProcessTransition(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void MoveNewLane(float DeltaTime);

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;



	// scoring

	UPROPERTY()
	float ScoreCooldown = 0;
	UPROPERTY()
	float ScoreCooldownMax = 0.2;
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual UBikeMovementComponent* GetMovementComponent() const override;

	UFUNCTION(BlueprintCallable)
	ABikeLaneActor* GetLaneActor() const;

	UFUNCTION(BlueprintCallable)
	void Turn(float Angle, FVector CenterPoint);

	// Getter and Setter
	UFUNCTION(BlueprintCallable)
	float GetPostProcessAlpha() const;

	UFUNCTION(BlueprintCallable)
	float GetPowerPercent() const;
	UFUNCTION(BlueprintCallable)
	float GetRawPower(int Scale) const;

	UFUNCTION()
	void SetCurrentPower(float NewPower);
	UFUNCTION()
	void Movement(float DeltaTime);
	UFUNCTION(BlueprintCallable)
	FVector GetPrevMov();
	UFUNCTION()
	void ZeroPrevMov();

	UFUNCTION(BlueprintCallable)
	void LoadMaxPower();
	UFUNCTION(BlueprintCallable)
	void SetMaxPower(float NewPower);

	UFUNCTION(BlueprintCallable)
	void SetLanePos(FVector Easy, FVector Med, FVector Hard);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetLaneBlocked(bool Blocking);
	UFUNCTION(BlueprintCallable)
	bool GetLaneSwitching() const;

	UFUNCTION(BlueprintCallable)
	void ChangePowerLane(int NewLane, float DeltaTime);
	UFUNCTION(BlueprintCallable)
	int GetPowerLane() const;

	UFUNCTION(BlueprintCallable)
	float GetLaneWidth();
};
