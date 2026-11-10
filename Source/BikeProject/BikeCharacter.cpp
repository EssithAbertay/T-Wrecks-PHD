// Fill out your copyright notice in the Description page of Project Settings.

#include "BikeCharacter.h"

#include "BikeGameInstance.h"

// Sets default values
ABikeCharacter::ABikeCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create our components
	
	// Our root component will be a sphere that reacts to physics
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("RootComponent"));
	RootComponent = CapsuleComponent;
	CapsuleComponent->SetCapsuleRadius(25);
	CapsuleComponent->SetCapsuleHalfHeight(60);
	CapsuleComponent->SetCollisionProfileName(TEXT("Pawn"));

	PlayerVisibleComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PlayerVisibleComponent"));
	PlayerVisibleComponent->SetupAttachment(RootComponent);

	PlayerVisibleComponent->SetRelativeTransform(FTransform(FRotator(0.f, 180.f, 0.f), FVector(-19.5f, 0.f, -57.f), FVector(0.8f)));

	PlayerCameraSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraSpringArm"));
	PlayerCameraSpringArm->SetupAttachment(RootComponent);

	CameraDistance = 300.f;
	PlayerCameraSpringArm->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 50.f), FRotator(-15.0f, 0.0f, 0.0f));
	PlayerCameraSpringArm->TargetArmLength = CameraDistance;
	PlayerCameraSpringArm->bEnableCameraLag = false;
	PlayerCameraSpringArm->bEnableCameraRotationLag = true;
	PlayerCameraSpringArm->bDoCollisionTest = false;
	PlayerCameraSpringArm->CameraLagSpeed = 3.0f;
	PlayerCameraSpringArm->CameraRotationLagSpeed = 3.0f;

	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	PlayerCamera->SetupAttachment(PlayerCameraSpringArm, USpringArmComponent::SocketName);

	// Create an instance of our movement component, and tell it to update the root.
	MovementComponent = CreateDefaultSubobject<UBikeMovementComponent>(TEXT("PlayerMovementComponent"));
	MovementComponent->UpdatedComponent = RootComponent;

	//Take control of the default Player
	AutoPossessPlayer = EAutoReceiveInput::Player0;

	// Default variables, percents used to be 0.7, 0.5 
	UpperPercent = 0.8f;
	MiddlePercent = 0.4f;
	LowerPercent = 0.3f;

	BikeLanes = nullptr;
	PowerLane = 1;
	LaneWidth = 110.f;
	LaneSpeed = 1.5f;
	SpeedBase = 400.f;
	SpeedMultiplier = 600.f;
	LaneBlocked = false;
	LaneSwitching = false;

	FOVBase = 80.f;
	FOVMultiplier = 30.f;
	PPMed = 0.3f;
	PPAlpha = 0.f;
	PPAlphaMult = 2.f;
}

// Called when the game starts or when spawned
void ABikeCharacter::BeginPlay()
{
	Super::BeginPlay();

	LoadMaxPower();

	FVector LanesLocation = GetActorLocation();
	FRotator LanesRotation(GetActorRotation());
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoFail = true;
	SpawnInfo.Owner = this;
	BikeLanes = GetWorld()->SpawnActor<ABikeLaneActor>(LanesLocation, LanesRotation, SpawnInfo);

	BikeLanes->InitValues(LaneWidth, LaneSpeed);

	check(GEngine != nullptr);

	// Display a debug message for five seconds. 
	// The -1 "Key" value argument prevents the message from being updated or refreshed.
}

// Called every frame
void ABikeCharacter::Tick(float DeltaTime)
{
}

// Checks current power of dinosaur and moves them into correct lane
// Moves dinosaur forward at a speed value relative to power
void ABikeCharacter::Movement(float DeltaTime)
{
	BikeLanes->Move(MovementComponent->GetPreviousMovement());

	float ForwardValue;
	MoveNewLane(DeltaTime);

	ForwardValue = SpeedBase + GetPowerPercent() * SpeedMultiplier;
	//GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Red, TEXT("Speed: ") + FString::SanitizeFloat(ForwardValue), true);

	IntendedMovement = ForwardValue * GetActorForwardVector();
	MovementComponent->AddInputVector(IntendedMovement);

	UBikeGameInstance* GameInstanceRef = Cast<UBikeGameInstance>(GetGameInstance());
	GameInstanceRef->IncDistTravelled(ForwardValue * DeltaTime / 1000.f);
}

FVector ABikeCharacter::GetPrevMov()
{
	return IntendedMovement;
}

void ABikeCharacter::ZeroPrevMov()
{
	IntendedMovement = FVector(0.f);
}

void ABikeCharacter::DestroySelf_Implementation()
{
	BikeLanes->Destroy();
	BikeLanes = nullptr;
}

void ABikeCharacter::PostProcessTransition(float DeltaTime)
{
	float AlphaChange = DeltaTime * PPAlphaMult;

	switch (PowerLane)
	{
	case 0:
		PPAlpha = FMath::Clamp(PPAlpha - AlphaChange, 0.f, PPMed);
		break;
	case 1:
		if (PPAlpha > PPMed) PPAlpha = FMath::Clamp(PPAlpha - AlphaChange, PPMed, 1.f);
		else PPAlpha = FMath::Clamp(PPAlpha + AlphaChange, 0.f, PPMed);
		break;
	case 2:
		PPAlpha = FMath::Clamp(PPAlpha + AlphaChange, PPMed, 1.f);
		break;
	default:
		break;
	}

	PlayerCamera->SetFieldOfView(FOVBase + FOVMultiplier * PPAlpha);

	PlayerCameraSpringArm->SetRelativeLocation(FVector(0.f, 0.f, 30.f + PPAlpha * 20.f));
	PlayerCameraSpringArm->SetRelativeRotation(FRotator(-15.f + PPAlpha * 10.f, 0.f, 0.f));
	PlayerCameraSpringArm->TargetArmLength = CameraDistance + PPAlpha * (CameraDistance * 0.1f);
}

void ABikeCharacter::MoveNewLane_Implementation(float DeltaTime)
{
	FVector NewHorizontalPos = GetActorLocation();

	// moving to a lane
	if (LaneSwitching || LaneBlocked)
	{
		switch (PowerLane)
		{
		case 0:
			NewHorizontalPos = BikeLanes->MoveLeft(false, DeltaTime);
			break;
		case 1:
			NewHorizontalPos = BikeLanes->MoveCenter(false, DeltaTime, GetActorLocation());
			break;
		case 2:
			NewHorizontalPos = BikeLanes->MoveRight(false, DeltaTime);
			break;
		default:
			break;
		}
	}

	//checking if should be moving to a lane
	else
	{
		// Checks if CURRENTPOWER is past the upper of the power scale
		if (CURRENTPOWER > UPPERPOWER)
		{
			// move one lane at a time
			if (PowerLane == 0)
			{
				NewHorizontalPos = BikeLanes->MoveCenter(true, DeltaTime, GetActorLocation());
				PowerLane = 1;
			}
			else
			{
				if (PowerLane == 1) NewHorizontalPos = BikeLanes->MoveRight(true, DeltaTime); // only move right when there is space
				PowerLane = 2;
			}
		}
		else if (CURRENTPOWER > MIDDLEPOWER)
		{
			if (PowerLane != 1) NewHorizontalPos = BikeLanes->MoveCenter(true, DeltaTime, GetActorLocation());
			PowerLane = 1;
		}
		else
		{
			if (PowerLane == 2)
			{
				NewHorizontalPos = BikeLanes->MoveCenter(true, DeltaTime, GetActorLocation());
				PowerLane = 1;
			}
			else
			{
				if (PowerLane == 1) NewHorizontalPos = BikeLanes->MoveLeft(true, DeltaTime);
				PowerLane = 0;
			}
		}
	}
	LaneSwitching = !BikeLanes->IsFinishedMove();
	
	PostProcessTransition(DeltaTime);

	NewHorizontalPos.Z = GetActorLocation().Z;

	SetActorLocation(NewHorizontalPos);
}

UBikeMovementComponent* ABikeCharacter::GetMovementComponent() const
{
	return MovementComponent;
}

ABikeLaneActor* ABikeCharacter::GetLaneActor() const
{
	return BikeLanes;
}

void ABikeCharacter::Turn(float Angle, FVector CenterPoint)
{
	float NewAngle = GetActorRotation().Yaw + Angle;

	FRotator NewRotation = FRotator(GetActorRotation().Pitch, NewAngle, GetActorRotation().Roll);
	SetActorRotation(NewRotation);

	BikeLanes->Rotate(NewAngle, CenterPoint, GetActorLocation());
	LaneSwitching = true;
}

float ABikeCharacter::GetPostProcessAlpha() const
{
	return PPAlpha;
}

float ABikeCharacter::GetPowerPercent() const
{
	return FMath::Clamp(CURRENTPOWER / MAXPOWER, 0.f, 1.f);
}

// Returns different power values based on Scale input
float ABikeCharacter::GetRawPower(int Scale) const
{
	switch (Scale)
	{
	case 0:
		return LOWERPOWER;
		break;
	case 1:
		return MIDDLEPOWER;
		break;
	case 2:
		return UPPERPOWER;
		break;
	case 3:
		return MAXPOWER;
		break;
	case 4:
	default:
		return CURRENTPOWER;
		break;
	}
}

void ABikeCharacter::SetCurrentPower(float NewPower)
{
	CURRENTPOWER = NewPower;
}

// Sets MAXPOWER from the GameInstance
void ABikeCharacter::LoadMaxPower()
{
	UBikeGameInstance* GameInstanceRef = Cast<UBikeGameInstance>(GetGameInstance());
	MAXPOWER = GameInstanceRef->GetMaxPower();

	//GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Max Power: ") + FString::SanitizeFloat(MAXPOWER), true);
	// Sets three power stages to be a percentage of MAXPOWER
	UPPERPOWER = MAXPOWER * UpperPercent;
	MIDDLEPOWER = MAXPOWER * MiddlePercent;
	LOWERPOWER = MAXPOWER * LowerPercent;
}

void ABikeCharacter::SetMaxPower(float NewPower)
{
	MAXPOWER = NewPower;
	// Sets three power stages to be a percentage of MAXPOWER
	UPPERPOWER = MAXPOWER * UpperPercent;
	MIDDLEPOWER = MAXPOWER * MiddlePercent;
	LOWERPOWER = MAXPOWER * LowerPercent;
}

void ABikeCharacter::SetLanePos(FVector Easy, FVector Med, FVector Hard)
{
	EasyLanePos = Easy;
	MedLanePos = Med;
	HardLanePos = Hard;

	EasyLanePos.Z = GetActorLocation().Z;
	MedLanePos.Z = GetActorLocation().Z;
	HardLanePos.Z = GetActorLocation().Z;
}

void ABikeCharacter::SetLaneBlocked_Implementation(bool Blocking)
{
	LaneBlocked = Blocking;
}

bool ABikeCharacter::GetLaneSwitching() const
{
	return LaneSwitching;
}

void ABikeCharacter::ChangePowerLane(int NewLane, float DeltaTime)
{
	if (PowerLane != NewLane)
	{
		FVector NewHorizontalPos = GetActorLocation();

		PowerLane = NewLane;
		switch (PowerLane)
		{
		case 0:
			NewHorizontalPos = BikeLanes->MoveLeft(true, DeltaTime);
			break;
		case 1:
			NewHorizontalPos = BikeLanes->MoveCenter(true, DeltaTime, GetActorLocation());
			break;
		case 2:
			NewHorizontalPos = BikeLanes->MoveRight(true, DeltaTime);
			break;
		default:
			break;
		}
		LaneSwitching = true;

		NewHorizontalPos.Z = GetActorLocation().Z;

		SetActorLocation(NewHorizontalPos);
	}
}

int ABikeCharacter::GetPowerLane() const
{
	return PowerLane;
}

float ABikeCharacter::GetLaneWidth()
{
	return LaneWidth;
}
