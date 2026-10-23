// Fill out your copyright notice in the Description page of Project Settings.


#include "BikeBluetoothInput.h"

#include "Misc/Paths.h" 

#include "Engine/Engine.h" 

#include "BikeGameInstance.h"

#include <simpleble/Public/simpleble/SimpleBLE.h>

#include <string>

BikeBluetoothInput::BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef)
{
    GameInstanceRef = BikeInstanceRef;
    FirstRun = true;
    UE_LOG(LogTemp, Warning, TEXT("Constructor of bluetooth task thread"));
    RunnableThread = FRunnableThread::Create(this, TEXT("Bluetooth Task"), 0, TPri_BelowNormal);
}

BikeBluetoothInput::~BikeBluetoothInput()
{
    UE_LOG(LogTemp, Warning, TEXT("Deconstructor of bluetooth task thread"));
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

		std::vector<SimpleBLE::Peripheral> peripherals;
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

		bool connected = false;

		while (connected)
		{

			// Sleep so we don't use all resources waiting for a connection
			FPlatformProcess::Sleep(0.03F);
		}
    }

    while (bRunThread)
    {
        // do something
    }

    return 0;
}

void BikeBluetoothInput::Stop()
{
   UE_LOG(LogTemp, Warning, TEXT("STOP of bluetooth task thread"));

   bRunThread = false;
}