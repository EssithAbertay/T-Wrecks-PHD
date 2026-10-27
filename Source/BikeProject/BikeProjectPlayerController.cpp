// Fill out your copyright notice in the Description page of Project Settings.

#include "BikeProjectPlayerController.h"

#include "BikeCharacter.h"
#include "BikeGameInstance.h"

ABikeProjectPlayerController::ABikeProjectPlayerController()
{
	// Sets the starting values for non bike input to current time, so the first input isn't very long
	TimeStartLeft = FPlatformTime::Seconds();
	TimeStartRight = FPlatformTime::Seconds();
	// Reserves memory for the pedal array
	PedalTimes.Reserve(ARRAYMAXSIZE + 1);

	// Default variables
	PowerLevelCurrent = 0;
	PowerLevelTarget = 0;
	PowerAlpha = 0;

	MovePauseBlocked = false;
	MoveUIBlocked = false;

	PlayerHealth = 8;
	CurrencyCount = 0;
	CurrencyBase = 5;
	CurrencyMultPos = 0.1f;
	CurrencyMultRevive = 0.2f;
	CurrencyMultDead = 0.8f;

	PlayerMoveEnum = PME_Normal;
}

void ABikeProjectPlayerController::BeginPlay()
{
    Super::BeginPlay();
    SetInputMode(FInputModeGameOnly());

	ABikeCharacter* PawnInstanceRef = Cast<ABikeCharacter>(GetPawn());
	SetViewTarget(PawnInstanceRef);
	PowerLevelMax = PawnInstanceRef->GetRawPower(3);
}

// Called every frame
void ABikeProjectPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ABikeCharacter* PawnInstanceRef = Cast<ABikeCharacter>(GetPawn());
	UBikeGameInstance* GameInstanceRef = Cast<UBikeGameInstance>(GetGameInstance());

	switch (PlayerMoveEnum)
	{
	case PME_BossCharge:
		if (PowerLevelMax < PowerLevelTarget) PowerLevelMax = PowerLevelTarget;
	case PME_Normal:
		//GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Green, TEXT("In Normal"), true);
	case PME_BossDodge:
	case PME_BossCooldown:

		float TempPower;

		PowerLevelBP = GameInstanceRef->GetBluetoothPower(); // CHANGED FROM GET SPEED! MOVE BACK LATER

		if (PowerLevelKB > PowerLevelBP)
		{
			PowerLevelKB -= 0.5f * DeltaTime;
			TempPower = PowerLevelKB;
		}
		else TempPower = PowerLevelBP;

		PowerTransition(DeltaTime, TempPower);

		break;
	case PME_BossAttack:
		
		PowerTransition(DeltaTime, PowerLevelAuto);
		break;
	case PME_SlowDown:

		PowerLevelAuto -= PowerLevelAuto * 0.95f * DeltaTime;
		PowerTransition(DeltaTime, PowerLevelAuto);
		break;
	default:
		break;
	}

	//GEngine->AddOnScreenDebugMessage(-1, DeltaTime, FColor::Green, TEXT("Power: ") + FString::SanitizeFloat(PowerLevel), true);
	PawnInstanceRef->SetCurrentPower(PowerLevel);

	if (!MovePauseBlocked && !MoveUIBlocked && !GetWorld()->IsPaused()) PawnInstanceRef->Movement(DeltaTime);
	else PawnInstanceRef->ZeroPrevMov();
}

// Called to bind functionality to input
void ABikeProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Set up "movement" bindings.
	InputComponent->BindAction("PedalLeftButton", IE_Pressed, this, &ABikeProjectPlayerController::PedalLeftStart);
	InputComponent->BindAction("PedalRightButton", IE_Pressed, this, &ABikeProjectPlayerController::PedalRightStart);
}

float ABikeProjectPlayerController::GetPowerLevel() const
{
	return PowerLevel;
}

float ABikeProjectPlayerController::GetPowerLevelMax() const
{
	return PowerLevelMax;
}

// Sets time for left input and calls AddTime
void ABikeProjectPlayerController::PedalLeftStart()
{
	TimeStartLeft = FPlatformTime::Seconds();
	AddTime();
}

// Sets time for right input and calls AddTime
void ABikeProjectPlayerController::PedalRightStart()
{
	TimeStartRight = FPlatformTime::Seconds();
	AddTime();
}

// Finds the absolute difference between the two inputs and adds to the PedalTimes array
// Calls CalculateBPM
void ABikeProjectPlayerController::AddTime()
{
	double TimeValue = abs(TimeStartLeft - TimeStartRight);

	// Adds the value to the array and checks to make sure the array is ARRAYMAXSIZE long
	PedalTimes.Add(TimeValue);
	if (PedalTimes.Num() > ARRAYMAXSIZE) PedalTimes.RemoveAt(0);

	CalculateBPM();
}

// Calculates the PowerLevel
void ABikeProjectPlayerController::CalculateBPM()
{
	RPM = 0;
	for (auto& Time : PedalTimes)
	{
		RPM += Time;
	}
	// Length of one beat
	RPM = RPM / PedalTimes.Num();
	// Beats-per-second
	RPM = 1 / RPM;
	// Beats-per-minute
	RPM *= 60;

	// Scale to other devices
	PowerLevelKB = RPM / 40;
}

void ABikeProjectPlayerController::PowerTransition(float DeltaTime, float NewPower)
{
	if (PowerLevelTarget != NewPower)
	{
		PowerLevelCurrent = PowerLevelTarget;
		PowerLevelTarget = NewPower;
		PowerAlpha = 0;
	}

	PowerAlpha += DeltaTime / 0.25f;
	PowerAlpha = FMath::Clamp(PowerAlpha, 0.f, 1.f);
	PowerLevel = FMath::Lerp(PowerLevelCurrent, PowerLevelTarget, PowerAlpha);
}

bool ABikeProjectPlayerController::GetMovePauseBlocked() const
{
	return MovePauseBlocked;
}

void ABikeProjectPlayerController::SetMovePauseBlocked(bool Blocking)
{
	MovePauseBlocked = Blocking;
}

bool ABikeProjectPlayerController::GetMoveUIBlocked() const
{
	return MoveUIBlocked;
}

void ABikeProjectPlayerController::SetMoveUIBlocked(bool Blocking)
{
	if (Blocking) PowerLevelKB = 0;
	MoveUIBlocked = Blocking;
}

void ABikeProjectPlayerController::IncPlayerHealth_Implementation(bool MaxHealth)
{
	if (MaxHealth) PlayerHealth = 8;
	else if (PlayerHealth < 8) PlayerHealth++;
}

void ABikeProjectPlayerController::PlayerHit_Implementation(bool OffScreen)
{
	PlayerHealth--;
}

int ABikeProjectPlayerController::GetPlayerHealth() const
{
	return PlayerHealth;
}

void ABikeProjectPlayerController::ResetCurrency()
{
	CurrencyCount = 0;
}

void ABikeProjectPlayerController::SetCurrency_Implementation(bool PositiveChange, int Multiplier)
{
	if (PositiveChange)
	{
		CurrencyCount += CurrencyBase * Multiplier + CurrencyCount * CurrencyMultPos;
	}
	else CurrencyCount -= CurrencyCount * CurrencyMultRevive;
}

int ABikeProjectPlayerController::GetCurrency() const
{
	return CurrencyCount;
}

void ABikeProjectPlayerController::SetMoveEnum_Implementation(EPlayerMove NewState, float DeltaTime)
{
	PlayerMoveEnum = NewState;
	ABikeCharacter* PawnInstanceRef = Cast<ABikeCharacter>(GetPawn());

	switch (PlayerMoveEnum)
	{
	case PME_Normal:
		SetViewTargetWithBlend(PawnInstanceRef, 1.0f);
		PawnInstanceRef->SetLaneBlocked(false);
		break;
	case PME_BossCharge:
		PawnInstanceRef->ChangePowerLane(1, DeltaTime);
		PawnInstanceRef->SetLaneBlocked(true);
		break;
	case PME_BossDodge:
		SetViewTargetWithBlend(PawnInstanceRef, 1.0f);
		PawnInstanceRef->SetLaneBlocked(false);
		break;
	case PME_BossAttack:
		PowerLevelAuto = PowerLevelTarget;
		PawnInstanceRef->ChangePowerLane(1, DeltaTime);
		PawnInstanceRef->SetLaneBlocked(true);
		break;
	case PME_BossCooldown:
		PowerLevelAuto = PowerLevelTarget;
		PawnInstanceRef->ChangePowerLane(1, DeltaTime);
		PawnInstanceRef->SetLaneBlocked(true);
		break;
	case PME_SlowDown:
		SetViewTargetWithBlend(PawnInstanceRef, 1.0f);
		PawnInstanceRef->SetLaneBlocked(true);
		break;
	default:
		break;
	}
}

EPlayerMove ABikeProjectPlayerController::GetMoveEnum() const
{
	return PlayerMoveEnum;
}