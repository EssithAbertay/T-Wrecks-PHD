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


		adapter.scan_for(5000);

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

			std::vector<std::pair<SimpleBLE::BluetoothUUID, SimpleBLE::BluetoothUUID>> readable_characteristics;
			for (SimpleBLE::Service& service : ConnectedPeripheral.services()) {
				for ( auto& characteristic : service.characteristics()) {
					if (characteristic.can_read()) {
						readable_characteristics.emplace_back(service.uuid(), characteristic.uuid());
					}
				}
			}
			if (readable_characteristics.empty()) {
				UE_LOG(LogTemp, Warning, TEXT("The peripheral has no readable characteristics."))
					ConnectedPeripheral.disconnect();
				return false;
			}
			UE_LOG(LogTemp, Warning, TEXT("Readable characteristics:"));

			for (std::size_t i = 0; i < readable_characteristics.size(); i++)
			{
				FString CharacteristicUUID = UTF8_TO_TCHAR(readable_characteristics[i].first.c_str());
				FString CharacteristicValue = UTF8_TO_TCHAR(readable_characteristics[i].second.c_str());

				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[%d] %s %s"),
					static_cast<int32>(i),
					*CharacteristicUUID,
					*CharacteristicValue
				);
			}

		}
	}

    while (bRunThread)
    {
		std::string ServiceUUID ="00001818-0000-1000-8000-00805f9b34fb";

		std::string CharacteristicUUID ="00002a63-0000-1000-8000-00805f9b34fb";

		SimpleBLE::ByteArray data = ConnectedPeripheral.read(ServiceUUID, CharacteristicUUID);
		FString HexData;

		for (uint8 Byte : data)
		{
			HexData += FString::Printf(TEXT("%02X"), Byte);
		}

		int Value = FParse::HexNumber(*HexData);
		UE_LOG(LogTemp, Warning, TEXT("Value: %s"),*FString::SanitizeFloat(static_cast<float>(Value)));
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