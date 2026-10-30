#pragma once

#include "CoreMinimal.h"
#include "BikeBluetoothConnections.generated.h"

UENUM(BlueprintType)
enum class EBluetoothConnectionDevice : uint8
{
	FRAME UMETA(DisplayName = "Square Frame"),
	TRAINER UMETA(DisplayName = "Trainer")
};


UENUM(BlueprintType)
enum class EBluetoothInputs : uint8
{
	LFRONTSHIFT = 0 UMETA(DisplayName = "Left Front Shift"),
	//1
	//2
	//3
	//4
	//5
	//6
	//7
	LEFTA = 8 UMETA(DisplayName = "Left Arrow"),
	UPA = 9 UMETA(DisplayName = "Up Arrow"),
	RIGHTA = 10 UMETA(DisplayName = "Right Arrow"),
	DOWNA = 11 UMETA(DisplayName = "Down Arrow"),
	SQUARE = 12 UMETA(DisplayName = "Square Button"),
	CROSS = 13 UMETA(DisplayName = "Cross Button"),
	//14
	//15
	//16
	//17
	//18
	//19
	//20
	//21
	//22
	//23
	ABUT = 24 UMETA(DisplayName = "A Button"),
	YBUT = 25 UMETA(DisplayName = "Y Button"),
	ZBUT = 26 UMETA(DisplayName = "Z Button"),
	BBUT = 27 UMETA(DisplayName = "B Button"),
	TRIANGLE = 28 UMETA(DisplayName = "Triangle Button"),
	CIRCLE = 29 UMETA(DisplayName = "Circle Button")
	//30
	//31
};

USTRUCT(BlueprintType)
struct FBluetoothDeviceStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Address;
};

