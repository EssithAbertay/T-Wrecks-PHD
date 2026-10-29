// Fill out your copyright notice in the Description page of Project Settings.


#include "BikeBluetoothInput.h"

#include "Misc/Paths.h" 

#include "Engine/Engine.h" 

#include "BikeGameInstance.h"

#include <string>

BikeBluetoothInput::BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef)
{
    GameInstanceRef = BikeInstanceRef;
    FirstRun = true;
	bRunThread = true;
    UE_LOG(LogTemp, Warning, TEXT("Constructor of bluetooth task thread"));
    RunnableThread = FRunnableThread::Create(this, TEXT("Bluetooth Task"), 0, TPri_BelowNormal);

	GameInstanceRef->SetMaxPower(160); // TEMP, JUST FOR TESTING!
}

BikeBluetoothInput::~BikeBluetoothInput()
{
    UE_LOG(LogTemp, Warning, TEXT("Deconstructor of bluetooth task thread"));

	//if (peripheral.is_connected())
	//{
	//	peripheral.disconnect();
	//	UE_LOG(LogTemp, Warning, TEXT("BLE device disconnected"));
	//}

	Stop();


	if (RunnableThread != nullptr)
	{
		RunnableThread->WaitForCompletion();

		delete RunnableThread;
		RunnableThread = nullptr;
	}
}

void BikeBluetoothInput::ReceivePowerNotification(SimpleBLE::ByteArray payload)
{
	FString HexData;

	for (uint8 Byte : payload)
	{
		HexData += FString::Printf(TEXT("%02X"), Byte);
	}
	
	uint16 Flags = uint16(payload[0]) | (uint16(payload[1]) << 8);
	FString flagData = FString::Printf(TEXT("%04X"), Flags); // should really invesitgate these further but not required

	uint16 Power = uint16(payload[2]) |(uint16(payload[3]) << 8); // these are always the power flags

	UE_LOG(LogTemp, Warning, TEXT("%u watts"), Power);

	GameInstanceRef->SetBluetoothPower(Power);
}

bool BikeBluetoothInput::ConnectToPeripheral(SimpleBLE::Peripheral* PeripheralToConnect)
{
	int indexToConnect = RequestedPeripheralArrayPosition;

	if (!(indexToConnect >= peripherals.size()))
	{
		PeripheralToConnect = &peripherals[indexToConnect];
	}
	else 
	{
		UE_LOG(LogTemp, Warning, TEXT("Peripheral Out Of Bounds!"));
		return false;
	}

	FString DeviceID = UTF8_TO_TCHAR(PeripheralToConnect->identifier().c_str());

	FString Msg = TEXT("Connecting to peripheral: ") + DeviceID;

	PeripheralToConnect->connect();

	Connected = true;

	UE_LOG(LogTemp, Warning, TEXT("Connected!"));


	for (SimpleBLE::Service& service : PeripheralToConnect->services())
		{
			for (auto& characteristic : service.characteristics())
			{
				const bool bCanRead = characteristic.can_read();
				const bool bCanNotify = characteristic.can_notify();
				const bool bCanIndicate = characteristic.can_indicate();
		
		
				const FString ServiceUUID =
					UTF8_TO_TCHAR(service.uuid().c_str());
		
				const FString CharacteristicUUID =
					UTF8_TO_TCHAR(characteristic.uuid().c_str());
		
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("Service: %s | Characteristic: %s | Read: %s | Notify: %s | Indicate: %s"),
					*ServiceUUID,
					*CharacteristicUUID,
					bCanRead ? TEXT("Yes") : TEXT("No"),
					bCanNotify ? TEXT("Yes") : TEXT("No"),
					bCanIndicate ? TEXT("Yes") : TEXT("No")
		
				);
			}
	}



	return true;
}

void BikeBluetoothInput::RequestConnection(const FString& p_RequestedConnectionAddress, EBluetoothConnectionDevice p_RequestedDevice, int PeripheralArrayPosition)
{
	 RequestedConnectionAddress = p_RequestedConnectionAddress;
	 RequestedDevice = p_RequestedDevice;
	 RequestedPeripheralArrayPosition = PeripheralArrayPosition;
	 ConnectionRequested = true;

	 UE_LOG(LogTemp, Warning, TEXT("Request Recieved, will attempt to connect to peripheral"));
}

// i wonder if these two functions could be the same quite easily
void BikeBluetoothInput::HandleNotificationSetup(SimpleBLE::Peripheral* PeripheralToConnect, const std::string& RequestedConnectionServiceUUID, const std::string& RequestedConnectionCharacteristicUUID, std::function<void(SimpleBLE::ByteArray)> Callback)
{
	PeripheralToConnect->notify(RequestedConnectionServiceUUID, RequestedConnectionCharacteristicUUID, [Callback](SimpleBLE::ByteArray payload) {Callback(payload); });
}

bool BikeBluetoothInput::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("INIT of bluetooth task thread"));

	if (!SimpleBLE::Adapter::bluetooth_enabled()) {
		UE_LOG(LogTemp, Warning, TEXT("Bluetooth disabled"));
		UE_LOG(LogTemp, Warning, TEXT("Returning false in init, exiting thread"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Bluetooth enabled, REALLY LONG BIT OF TEXT ON THE END TO MAKE IT EASIER TO SEE IN THE LOG"));

	auto adapters = SimpleBLE::Adapter::get_adapters();
	if (adapters.empty()) {
		UE_LOG(LogTemp, Warning, TEXT("No Bluetooth adapters found."));
		UE_LOG(LogTemp, Warning, TEXT("Bluetooth disabled"));
		return false;
	}

	Adapter = adapters[0];
	UE_LOG(LogTemp, Warning, TEXT("Using adapter: OUTPUT INFO HERE"));

	Adapter.set_callback_on_scan_start([]() { UE_LOG(LogTemp, Warning, TEXT("Scan started.")); });
	Adapter.set_callback_on_scan_stop([]() { UE_LOG(LogTemp, Warning, TEXT("Scan stopped.")); });
	Adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {

	std::string id = peripheral.identifier();

	std::string address = peripheral.address();

	FString DeviceID = UTF8_TO_TCHAR(id.c_str());
	FString DeviceAddress = UTF8_TO_TCHAR(address.c_str());

		if (peripheral.is_connectable() && DeviceID != "") // if connectable and has an actual name, all our intended devices have IDs so can filter out others for sake of ease of use
		{
			auto x = peripheral.services();
			peripherals.push_back(peripheral);
			GameInstanceRef->AddToDeviceArray(DeviceID, DeviceAddress);
			GameInstanceRef->BroadcastNewDevice(DeviceID, DeviceAddress, peripherals.size()-1);
			
			
			FString Msg = TEXT("Found device: ") + DeviceID + TEXT(" | Address: ") + DeviceAddress + TEXT("Counter: ") + FString::SanitizeFloat(counter) + TEXT(" PASSED: ") + FString::SanitizeFloat(peripherals.size() - 1);
			counter++;
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

		}
	});

    return true;
}

uint32 BikeBluetoothInput::Run()
{
     UE_LOG(LogTemp, Warning, TEXT("RUN of bluetooth task thread"));

	 //scan for devices
	 while (bRunThread && !Connecting)
	 {
		 if (!Connected && !ScanTimeOut)
		 {
			 if (!Adapter.scan_is_active()) // start a scan if one isn't active
			 {
				 Adapter.scan_start();
				 //ScanStartTime = FPlatformTime::Seconds();
				 //ScanTimeOut = false;
			 }

			 //if (FPlatformTime::Seconds() - ScanStartTime > ScanTime)
			 //{
				// ScanTimeOut = true;
				// UE_LOG(LogTemp, Warning, TEXT("Scan timed out"));

			 //}
		 }

		 if (ConnectionRequested/* || ScanTimeOut*/)
		 {
			if (Adapter.scan_is_active()) // stop scanning if we are scanning already
			{
				Adapter.scan_stop();
			}

			Connecting = true;

			SimpleBLE::Peripheral* PeripheralToConnect = nullptr;
			std::string ServiceUUID;
			std::string CharacteristicUUID;
			std::function<void(SimpleBLE::ByteArray)> Callback;
			if (RequestedDevice == EBluetoothConnectionDevice::FRAME)
			{
				PeripheralToConnect = &ConnectedFrame;

				ServiceUUID = "347b0001-7635-408b-8918-8ff3949ce592";
				CharacteristicUUID = "347b0043-7635-408b-8918-8ff3949ce592";

				Callback = [this](SimpleBLE::ByteArray Payload)
				{
					ReceivePowerNotification(Payload);
				};
			}
			else if (RequestedDevice == EBluetoothConnectionDevice::TRAINER)
			{
				PeripheralToConnect = &ConnectedTrainer;

				ServiceUUID = "00001818-0000-1000-8000-00805f9b34fb";
				CharacteristicUUID = "00002a63-0000-1000-8000-00805f9b34fb";

				Callback = [this](SimpleBLE::ByteArray Payload)
				{
					ReceivePowerNotification(Payload);
				};
			}

			if (PeripheralToConnect != nullptr)
			{
				ConnectToPeripheral(PeripheralToConnect);
				
				if(PeripheralToConnect->is_connected())
				{ 
					HandleNotificationSetup(PeripheralToConnect, ServiceUUID, CharacteristicUUID, Callback); 
				}
			}
		 }

		FPlatformProcess::Sleep(0.03F);
	 }
    return 0;
}

void BikeBluetoothInput::Stop()
{
   UE_LOG(LogTemp, Warning, TEXT("STOP of bluetooth task thread"));

	if (ConnectedFrame.initialized() && ConnectedFrame.is_connected())
	{
		ConnectedFrame.disconnect();
		UE_LOG(LogTemp, Warning, TEXT("Frame device disconnected"));
	}
   
	if (ConnectedTrainer.initialized() && ConnectedTrainer.is_connected())
	{
		ConnectedTrainer.disconnect();
		UE_LOG(LogTemp, Warning, TEXT("Trainer device disconnected"));
	}

   bRunThread = false;
}