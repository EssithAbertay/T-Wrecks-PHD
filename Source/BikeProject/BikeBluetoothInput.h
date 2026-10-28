// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformProcess.h" 
#include "HAL/Runnable.h"

#include <simpleble/Public/simpleble/SimpleBLE.h>

#include "BikeBluetoothConnections.h"

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
	SimpleBLE::Adapter Adapter;

	FThreadSafeBool Connected = false;

	int counter = 0;

public:
	BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef);
	~BikeBluetoothInput();
	void RequestConnection(const FString& RequestedConnectionAddress, EBluetoothConnectionDevice RequestedDevice, int PeripheralArrayPosition);

	// used in the thread
	void ReceivePowerNotification(SimpleBLE::ByteArray payload); // receive notification callback func
	bool ConnectToPeripheral();
private:
	FThreadSafeBool FirstRun = true;
	FThreadSafeBool bRunThread = true;

	// requesting a connection
	FString RequestedConnectionAddress;
	EBluetoothConnectionDevice RequestedDevice;
	int RequestedPeripheralArrayPosition;

	FThreadSafeBool ConnectionRequested = false;
	FThreadSafeBool Connecting = false;

	double ScanTime = 15;
	double ScanStartTime = 0;
	bool ScanTimeOut = false;
// requirements for runnable
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
};
