// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformProcess.h" 
#include "HAL/Runnable.h"

class BIKEPROJECT_API UBikeGameInstance;

/**
 * 
 */
class BIKEPROJECT_API BikeBluetoothInput : public FRunnable
{
	FRunnableThread* RunnableThread;
	UBikeGameInstance* GameInstanceRef;

public:
	BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef);
	~BikeBluetoothInput();

private:

	bool FirstRun;
	bool bRunThread;

// requirements for runnable
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
};
