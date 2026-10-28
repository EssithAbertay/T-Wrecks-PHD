#pragma once

#include "CoreMinimal.h"
#include "BikeBluetoothConnections.generated.h"

UENUM(BlueprintType)
enum class EBluetoothConnectionDevice : uint8
{
	FRAME UMETA(DisplayName = "Square Frame"),
	TRAINER UMETA(DisplayName = "Trainer")
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
