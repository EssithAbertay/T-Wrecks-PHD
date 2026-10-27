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
	FString flagData = FString::Printf(TEXT("%04X"), Flags);

	uint16 Power = uint16(payload[2]) |(uint16(payload[3]) << 8);

	UE_LOG(LogTemp, Warning, TEXT("%u watts"), Power);

	GameInstanceRef->SetBluetoothPower(Power);
}

bool BikeBluetoothInput::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("INIT of bluetooth task thread"));


    return true;
}

uint32 BikeBluetoothInput::Run()
{
    if (FirstRun) {
        FirstRun = false;
        UE_LOG(LogTemp, Warning, TEXT("RUN of bluetooth task thread"));

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

		auto adapter = adapters[0];
		UE_LOG(LogTemp, Warning, TEXT("Using adapter: OUTPUT INFO HERE"));


		adapter.set_callback_on_scan_start([]() { UE_LOG(LogTemp, Warning, TEXT("Scan started.")); });
		adapter.set_callback_on_scan_stop([]() { UE_LOG(LogTemp, Warning, TEXT("Scan stopped.")); });
		adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) {

			std::string id = peripheral.identifier();
			std::string address = peripheral.address();

			FString DeviceId = UTF8_TO_TCHAR(id.c_str());
			FString DeviceAddress = UTF8_TO_TCHAR(address.c_str());

			FString Msg = TEXT("Found device: ") + DeviceId + TEXT(" | Address: ") + DeviceAddress;
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);

			FString ForArray = DeviceId + TEXT(" | ") + DeviceAddress;

			if (peripheral.is_connectable()) {
				peripherals.push_back(peripheral);
				GameInstanceRef->AddToDeviceArray(ForArray);
			}
			});


		adapter.scan_for(15000);

    }


	UE_LOG(LogTemp, Warning, TEXT("Waiting to connect to peripheral"));


	int indexToConnect = -1;
	//after scanning for devices, connect to one
	while (!connected && bRunThread)
	{
		indexToConnect = GameInstanceRef->GetDesiredConnection();

		// -1 is default value, when you get an actual number attempt to connect to it
		if (indexToConnect != -1)
		{
			if (!(indexToConnect >= peripherals.size()))
			{
				ConnectedPeripheral = peripherals[indexToConnect];
			}

			UE_LOG(LogTemp, Warning, TEXT("Connecting to peripheral"));
			ConnectedPeripheral.connect();

			connected = true;
			UE_LOG(LogTemp, Warning, TEXT("Connected!"));

			UE_LOG(LogTemp, Warning, TEXT("Discovered characteristics:"));

			bool bFoundReadableCharacteristic = false;

			for (SimpleBLE::Service& service : ConnectedPeripheral.services())
			{
				for (auto& characteristic : service.characteristics())
				{
					const bool bCanRead = characteristic.can_read();
					const bool bCanNotify = characteristic.can_notify();
					const bool bCanIndicate = characteristic.can_indicate();

					if (bCanRead)
					{
						bFoundReadableCharacteristic = true;
					}

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

			if (!bFoundReadableCharacteristic)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("The peripheral has no readable characteristics.")
				);

				ConnectedPeripheral.disconnect();
				return false;
			}

			std::string ServiceUUID = "00001818-0000-1000-8000-00805f9b34fb";
			std::string CharacteristicUUID ="00002a63-0000-1000-8000-00805f9b34fb";
			ConnectedPeripheral.notify(ServiceUUID, CharacteristicUUID, [this](SimpleBLE::ByteArray payload){ReceivePowerNotification(payload);});
		}

		FPlatformProcess::Sleep(0.03f);
	}

	while (bRunThread)
	{
		FPlatformProcess::Sleep(0.03F);
	}

    return 0;
}

void BikeBluetoothInput::Stop()
{
   UE_LOG(LogTemp, Warning, TEXT("STOP of bluetooth task thread"));

	if (ConnectedPeripheral.initialized() && ConnectedPeripheral.is_connected())
	{
		ConnectedPeripheral.disconnect();
		UE_LOG(LogTemp, Warning, TEXT("BLE device disconnected"));
	}
   

   bRunThread = false;
}