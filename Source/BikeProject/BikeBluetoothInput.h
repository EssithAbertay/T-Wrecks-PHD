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
	SimpleBLE::Peripheral ConnectedFrame;
	SimpleBLE::Peripheral ConnectedTrainer;
	SimpleBLE::Adapter Adapter;

	FThreadSafeBool Connected = false;

	int counter = 0;

public:
	BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef);
	~BikeBluetoothInput();
	void RequestConnection(const FString& RequestedConnectionAddress, EBluetoothConnectionDevice RequestedDevice, int PeripheralArrayPosition);

	void HandleNotificationSetup(SimpleBLE::Peripheral* PeripheralToConnect, const std::string& RequestedConnectionServiceUUID, const  std::string& RequestedConnectionCharacteristicUUID, std::function<void(SimpleBLE::ByteArray)> Callback);

	// requests are used to maintain thread safety
	void RequestScanStart();
	void RequestScanStop();

	// used in the thread
	void ReceivePowerNotification(SimpleBLE::ByteArray payload); // receive notification callback func
	void ReceiveControllerNotification(SimpleBLE::ByteArray payload); // receive notification callback func


	bool ConnectToPeripheral(SimpleBLE::Peripheral * PeripheralToConnect);
private:
	FThreadSafeBool FirstRun = true;
	FThreadSafeBool bRunThread = true;

	// requesting a connection
	FString RequestedConnectionAddress;
	EBluetoothConnectionDevice RequestedDevice;
	int RequestedPeripheralArrayPosition;



	FThreadSafeBool ConnectionRequested = false;
	FThreadSafeBool Connecting = false;

	FThreadSafeBool RequestedScanStart = false;
	FThreadSafeBool RequestedScanStop = false;

	double ScanTime = 15;
	double ScanStartTime = 0;
	bool ScanTimeOut = false;
// requirements for runnable
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
};
