// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformProcess.h" 
#include "HAL/Runnable.h"
#include "Async/Async.h"
#include <simpleble/Public/simpleble/SimpleBLE.h>

#include "BikeBluetoothConnections.h"

class BIKEPROJECT_API UBikeGameInstance;
class BIKEPROJECT_API UBikeLoginInstanceSubsystem;





/**
 * 
 */
class BIKEPROJECT_API BikeBluetoothInput : public FRunnable
{
	FRunnableThread* RunnableThread;
	UBikeGameInstance* GameInstanceRef = nullptr;
	UBikeLoginInstanceSubsystem* LoginSubsystemRef = nullptr;

	std::unordered_map<std::string, SimpleBLE::Peripheral> PendingPeripherals;
	std::unordered_map<std::string, SimpleBLE::Peripheral> ClassifiedPeripherals;

	SimpleBLE::Peripheral ConnectedFrame;
	SimpleBLE::Peripheral ConnectedTrainer;
	SimpleBLE::Peripheral ConnectedHRMonitor;
	SimpleBLE::Adapter Adapter;

	FThreadSafeBool Connected = false;

	int counter = 0;

public:
	BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef);
	~BikeBluetoothInput();
	void RequestConnection(const FString& RequestedConnectionAddress, EBluetoothConnectionDevice RequestedDevice);

	void HandleNotificationSetup(SimpleBLE::Peripheral* PeripheralToConnect, const std::string& RequestedConnectionServiceUUID, const  std::string& RequestedConnectionCharacteristicUUID, std::function<void(SimpleBLE::ByteArray)> Callback);

	// requests are used to maintain thread safety
	void RequestScanStart();
	void RequestScanStop();

	void RequestGearChange(int NewGear);


	// used in the thread
	void ReceivePowerNotification(SimpleBLE::ByteArray payload); // receive notification callback func
	void ReceiveControllerNotification(SimpleBLE::ByteArray payload); // receive notification callback func
	void ReceieveHeartRateNotification(SimpleBLE::ByteArray payload); // receive notification callback func

	bool ConnectToPeripheral(SimpleBLE::Peripheral * PeripheralToConnect, const FString& PeripheralAddress);

	// these could probably be unified into one function for each device that returns the details struct, idk

	EBluetoothConnectionState GetFrameConnectionState() const;
	EBluetoothConnectionState GetTrainerConnectionState() const;
	EBluetoothConnectionState GetHRConnectionState() const;
	
	FString GetConnectedFrameName() const;
	FString GetConnectedTrainerName() const;
	FString GetConnectedHRMonitorName() const;

	FString GetConnectedFrameAddress() const;
	FString GetConnectedTrainerAddress() const;
	FString GetConnectedHRMonitorAddress() const;
private:
	FThreadSafeBool FirstRun = true;
	FThreadSafeBool bRunThread = true;

	// requesting a connection
	//FString RequestedConnectionAddress;
	//EBluetoothConnectionDevice RequestedDevice;

	TQueue<FBluetoothConnectionRequest, EQueueMode::Mpsc> ConnectionRequestQueue;

	//FThreadSafeBool ConnectionRequested = false;
	//FThreadSafeBool Connecting = false;

	FThreadSafeBool RequestedScanStart = false;
	FThreadSafeBool RequestedScanStop = false;

	FThreadSafeBool FTMSPresent = false;

	uint32 ControllerData;
	uint32 PreviousControllerData;

	int MinimumResistance = 0;
	int MaximumResistnace = 0;
	int ResistanceIncrement = 0;
	int CurrentResistance = 0;
	int NewResistance = 0;

	FThreadSafeBool ResistanceSupported = false;
	FThreadSafeBool ResistanceControlSupported = false;
	FThreadSafeBool RequestedGearChange = false;


	// relevant services and characteristics that we use, im reusing the same ftms service woops

	// FTMS, where we get the current power
	const std::string FTMSServiceUUID = "00001826-0000-1000-8000-00805f9b34fb";
	const std::string FTMSCharacteristicUUID = "00002ad2-0000-1000-8000-00805f9b34fb";
	
	//FTMS Resistance range , where we read the min max increment resistance of the trainer
	const std::string FTMSResistanceRangeServiceUUID = "00001826-0000-1000-8000-00805f9b34fb";
	const std::string FTMSResistanceCharacteristicUUID = "00002ad8-0000-1000-8000-00805f9b34fb";

	//FTMS Control Point, where we write the gear changes and stuff too
	const std::string FTMSControlPointServiceUUID = "00001826-0000-1000-8000-00805f9b34fb";
	const std::string FTMSControlPointCharacteristicUUID = "00002ad9-0000-1000-8000-00805f9b34fb";

	//The other bike power one, we sometimes read the bike power from here
	const std::string PMSServiceUUID = "00001818-0000-1000-8000-00805f9b34fb";
	const std::string PMSCharacteristicUUID = "00002a63-0000-1000-8000-00805f9b34fb";

	// The controller, get button inputs
	const std::string ControllerServiceUUID = "347b0001-7635-408b-8918-8ff3949ce592";
	const std::string ControllerCharacteristicUUID = "347b0043-7635-408b-8918-8ff3949ce592";


	// Heartrate monitor, get the heartrate
	const std::string HeartRateServiceUUID = "0000180d-0000-1000-8000-00805f9b34fb";
	const std::string HeartRateCharacteristicUUID = "00002a37-0000-1000-8000-00805f9b34fb";

	FBluetoothDeviceDetailsStruct TrainerDetails;
	FBluetoothDeviceDetailsStruct FrameDetails;
	FBluetoothDeviceDetailsStruct HRDetails;

	// internal functions
	bool ClassifyPeripheral(SimpleBLE::Peripheral& Peripheral, EBluetoothConnectionDevice& OutDeviceType);

	bool CheckReconnect(FString DeviceAddress, EBluetoothConnectionDevice& OutDeviceType);

	void HandleExistingConnection(SimpleBLE::Peripheral* Peripheral, EBluetoothConnectionDevice DeviceType);

	void HandleFTMSSetup();

	void ChangeGear();

	// used to check if any device is in the process of connecting, does what it says on the tin
	bool CheckAnyDeviceConnecting();

// requirements for runnable
public:
    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
};
