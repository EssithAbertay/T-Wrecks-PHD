// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformProcess.h" 
#include "HAL/Runnable.h"

#include <simpleble/Public/simpleble/SimpleBLE.h>

class BIKEPROJECT_API UBikeGameInstance;

/**
 * 
 */
class BIKEPROJECT_API BikeBluetoothInput : public FRunnable
{
	FRunnableThread* RunnableThread;
	UBikeGameInstance* GameInstanceRef;
	std::vector<SimpleBLE::Peripheral> peripherals;
	SimpleBLE::Peripheral ConnectedPeripheral;
	FThreadSafeBool connected = false;


public:
	BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef);
	~BikeBluetoothInput();

private:

	FThreadSafeBool FirstRun = true;
	FThreadSafeBool bRunThread = true;

// requirements for runnable
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
};
