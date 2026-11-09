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
	int16 Power;

	for (uint8 Byte : payload) // process the data we have recieved
	{
		HexData += FString::Printf(TEXT("%02X"), Byte);
	}
	

	if (FTMSPresent) // if ftms is present the bits we scan are different to when using PMS
	{
		if (payload.size() < 2) // if the data doesn't contain the flags for whatever reason
		{
			return;
		}

		uint16_t flags = uint16(payload[0]) | (uint16(payload[1]) << 8);

		if (!(flags & (1 << 6)))
		{
			return; // bit 6 of flags indicates whether power is present
		}

		size_t offset = 2;

		// Skip optional fields before instantaneous power

		if (!(flags & (1 << 0))) offset += 2; // Speed
		if (flags & (1 << 1)) offset += 2;    // Average speed
		if (flags & (1 << 2)) offset += 2;    // Cadence
		if (flags & (1 << 3)) offset += 2;    // Average cadence
		if (flags & (1 << 4)) offset += 3;    // Total distance
		if (flags & (1 << 5)) offset += 2;    // Resistance


		if (offset + 2 > payload.size())
			return;

		Power = static_cast<int16>((uint16(payload[offset]) | (uint16(payload[offset + 1]) << 8)));

		// Shouldn't be hard to read speed here as well if you want to!
	}
	else // read power from PMS
	{
		uint16 Flags = uint16(payload[0]) | (uint16(payload[1]) << 8);
		FString flagData = FString::Printf(TEXT("%04X"), Flags); // should really invesitgate these further but not required

		Power = static_cast<int16>(uint16(payload[2]) | (uint16(payload[3]) << 8)); // these are always the power flags

	}

	UE_LOG(LogTemp, Warning, TEXT("%d watts"), Power);
	GameInstanceRef->SetBluetoothPower(Power);
}


void BikeBluetoothInput::ReceiveControllerNotification(SimpleBLE::ByteArray payload)
{	
	PreviousControllerData = ControllerData;

	ControllerData =
		(static_cast<uint32>(payload[3]) << 24) |
		(static_cast<uint32>(payload[4]) << 16) |
		(static_cast<uint32>(payload[5]) << 8) |
		(static_cast<uint32>(payload[6]));


	// processing the data, if it was
	// Check all 32 bits, when checking we 

	for (int32 Bit = 0; Bit < 32; Bit++)
	{
		uint32 Mask = (1u << Bit);

		bool pressed = (ControllerData & Mask) != 0;
		bool wasPressed = (PreviousControllerData & Mask) != 0;

		if (!wasPressed && pressed ) // pressed this tick
		{
			// send pressed signal
			UE_LOG(LogTemp, Warning, TEXT("Input %d ACTIVE"), Bit);
			GameInstanceRef->BroadcastBluetoothInput(EBluetoothInputEvent::PRESSED, EBluetoothInputs(Bit));
		}
		else if (wasPressed && pressed) // held this tick
		{
			// send held signal
			GameInstanceRef->BroadcastBluetoothInput(EBluetoothInputEvent::HELD, EBluetoothInputs(Bit));

		}
		else if (wasPressed && !pressed) // let go this tick
		{
			// send released signal
			GameInstanceRef->BroadcastBluetoothInput(EBluetoothInputEvent::RELEASED, EBluetoothInputs(Bit));
		}
		else // not pressed at all
		{
			// don't do anything, will be handled in let go this tick
		}
	}

	GameInstanceRef->SetControllerState(ControllerData);
}

bool BikeBluetoothInput::ConnectToPeripheral(SimpleBLE::Peripheral* PeripheralToConnect)
{
	int indexToConnect = RequestedPeripheralArrayPosition;

	if (!(indexToConnect >= peripherals.size()))
	{
		*PeripheralToConnect = peripherals[indexToConnect];
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
				const bool bCanWriteCommand = characteristic.can_write_command();
				const bool bCanWriteRequest = characteristic.can_write_request();

				const FString ServiceUUID =
					UTF8_TO_TCHAR(service.uuid().c_str());
		
				const FString CharacteristicUUID =
					UTF8_TO_TCHAR(characteristic.uuid().c_str());
		
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("Service: %s | Characteristic: %s | Read: %s | Notify: %s | Indicate: %s | Write Command: %s | Write Request: %s"),
					*ServiceUUID,
					*CharacteristicUUID,
					bCanRead ? TEXT("Yes") : TEXT("No"),
					bCanNotify ? TEXT("Yes") : TEXT("No"),
					bCanIndicate ? TEXT("Yes") : TEXT("No"),
					bCanWriteCommand ? TEXT("Yes") : TEXT("No"),
					bCanWriteRequest ? TEXT("Yes") : TEXT("No")
				);
			}
	}

	return true;
}

EBluetoothConnectionState BikeBluetoothInput::GetFrameConnectionState()
{
	return FrameConnectionState;
}

EBluetoothConnectionState BikeBluetoothInput::GetTrainerConnectionState()
{
	return TrainerConnectionState;
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
	PeripheralToConnect->notify(RequestedConnectionServiceUUID, RequestedConnectionCharacteristicUUID, [Callback](SimpleBLE::ByteArray payload) {Callback(payload);});
}

void BikeBluetoothInput::RequestScanStart()
{
	RequestedScanStart = true;
}

void BikeBluetoothInput::RequestScanStop()
{
	RequestedScanStop = true;
}

void BikeBluetoothInput::RequestGearChange(int NewGear)
{
	NewResistance = (MaximumResistnace / GameInstanceRef->GetMaxGear()) * NewGear;
	int IncrementFix = NewResistance % ResistanceIncrement;
	NewResistance -= IncrementFix;

	RequestedGearChange = true;
}

void BikeBluetoothInput::HandleFTMSSetup()
{
	// FTMS setup, we want to read the resistance max, increment, and current, and set up how it can be changed!

	// we rewrite the FTMS UUID so much that it should really be stored as a variable 
	std::string ServiceUUID ="00001826-0000-1000-8000-00805f9b34fb";
	std::string CharacteristicUUID ="00002ad6-0000-1000-8000-00805f9b34fb";


	SimpleBLE::ByteArray Data = ConnectedTrainer.read(ServiceUUID, CharacteristicUUID);

	if (Data.size() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to read FTMS resistance range"));
		return;
	}

	for (int32 i = 0; i < Data.size(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("Byte %d: 0x%02X"),
			i, static_cast<uint8>(Data[i]));
	}

	MinimumResistance =
		static_cast<uint16>(static_cast<uint8>(Data[0])) |
		(static_cast<uint16>(static_cast<uint8>(Data[1])) << 8);

	MaximumResistnace =
		static_cast<uint16>(static_cast<uint8>(Data[2])) |
		(static_cast<uint16>(static_cast<uint8>(Data[3])) << 8);

	ResistanceIncrement =
		static_cast<uint16>(static_cast<uint8>(Data[4])) |
		(static_cast<uint16>(static_cast<uint8>(Data[5])) << 8);

	UE_LOG(LogTemp, Warning,
		TEXT("FTMS Resistance - Min: %d | Max: %d | Increment: %d"),
		static_cast<int32>(MinimumResistance),
		static_cast<int32>(MaximumResistnace),
		static_cast<int32>(ResistanceIncrement)
	);


	ConnectedTrainer.indicate(
		FTMSControlPointServiceUUID,
		FTMSControlPointCharacteristicUUID,
		[](SimpleBLE::ByteArray Data)
		{
			// add confimrations for different recieved codes

			UE_LOG(LogTemp, Warning, TEXT("FTMS indication received"));

			for (int i = 0; i < Data.size(); i++)
			{
				UE_LOG(LogTemp, Warning, TEXT("Byte %d: %02X"),
					i, static_cast<uint8>(Data[i]));
			}
		}
	);

	// Request control of the fitness machine
	SimpleBLE::ByteArray RequestControlData = { 0x00 };

	ConnectedTrainer.write_request(
		FTMSControlPointServiceUUID,
		FTMSControlPointCharacteristicUUID,
		RequestControlData
	);
}

void BikeBluetoothInput::ChangeGear()
{
	int16 TargetResistance = NewResistance;

	UE_LOG(LogTemp, Warning, TEXT("Setting Target Resistance: %d"), static_cast<int32>(TargetResistance));

	SimpleBLE::ByteArray Data =
	{
		0x04,
		static_cast<uint8>(TargetResistance & 0xFF),
		static_cast<uint8>((TargetResistance >> 8) & 0xFF)
	};

	ConnectedTrainer.write_request(FTMSControlPointServiceUUID,FTMSControlPointCharacteristicUUID,Data);
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
	Adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral) 
	{
		std::string id = peripheral.identifier();
		std::string address = peripheral.address();

		FString DeviceID = UTF8_TO_TCHAR(id.c_str());
		FString DeviceAddress = UTF8_TO_TCHAR(address.c_str());

		if (peripheral.is_connectable() && DeviceID != "") // if connectable and has an actual name, all our intended devices have IDs so can filter out others for sake of ease of use
		{
			auto x = peripheral.services();
			peripherals.push_back(peripheral);

			// deprecated, remove when possible
			GameInstanceRef->AddToDeviceArray(DeviceID, DeviceAddress);


			EBluetoothConnectionDevice DeviceType;
			for (SimpleBLE::Service& service : peripheral.services())
			{
				const FString ServiceUUID = UTF8_TO_TCHAR(service.uuid().c_str());

				UE_LOG(
					LogTemp,
					Warning,
					TEXT("Advertised Service: %s"),
					*ServiceUUID
				);
			}

			// check what type of device it is before connection, trainer or frame, trainer will have the bike power service, frame will have the custom controller service
			for ( SimpleBLE::Service& service : peripheral.services())
			{
				const FString ServiceUUID =UTF8_TO_TCHAR(service.uuid().c_str());
				// bike power service
				if (ServiceUUID.Equals("00001818-0000-1000-8000-00805f9b34fb"))
				{
					DeviceType = EBluetoothConnectionDevice::TRAINER;
					break;
				}
				else // this will grab all devices that don't have bike power service, as the controller schema service is not adevertised, if you only want the Sqaure check if DeviceID == "SQUARE"
				{
					DeviceType = EBluetoothConnectionDevice::FRAME;
				}
			}


			// tell the game instance the devices name, what type it is, and where it is in the array, super ugly, but since we don't reorder it anywhere it's fine!!!
			GameInstanceRef->BroadcastNewDevice(DeviceID, DeviceType, peripherals.size()-1);
			
			// debug message not really needed anymore
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
	 while (bRunThread)
	 {
		 if (RequestedScanStart)
		 {
			 RequestedScanStart = false;

			 if (!Adapter.scan_is_active()) // start a scan if one isn't active
			 {
				 Adapter.scan_start(); 
				 
				 //ScanStartTime = FPlatformTime::Seconds();
				 //ScanTimeOut = false;
			 }
		 }
		 //if (FPlatformTime::Seconds() - ScanStartTime > ScanTime)
			 //{
				// ScanTimeOut = true;
				// UE_LOG(LogTemp, Warning, TEXT("Scan timed out"));

			 //}
		 if (RequestedScanStop)
		 {
			 RequestedScanStop = false;

			 if (Adapter.scan_is_active()) // stop a scan if one is active
			 {
				 Adapter.scan_stop();
			 }
		 }

		 if (ConnectionRequested)
		 {
			ConnectionRequested = false;
			SimpleBLE::Peripheral* PeripheralToConnect = nullptr;
			std::string ServiceUUID;
			std::string CharacteristicUUID;
			std::function<void(SimpleBLE::ByteArray)> Callback;
			
			
			switch (RequestedDevice)
			{
			case EBluetoothConnectionDevice::FRAME:
				UE_LOG(LogTemp, Warning, TEXT("connection to Frame!"));

				PeripheralToConnect = &ConnectedFrame;
				FrameConnectionState =  EBluetoothConnectionState::CONNECTING;
				ServiceUUID = "347b0001-7635-408b-8918-8ff3949ce592";
				CharacteristicUUID = "347b0043-7635-408b-8918-8ff3949ce592";

				Callback = [this](SimpleBLE::ByteArray Payload)
				{
					ReceiveControllerNotification(Payload);
				};
				break;
			case EBluetoothConnectionDevice::TRAINER:
				UE_LOG(LogTemp, Warning, TEXT("connection to Trainer!"));

				PeripheralToConnect = &ConnectedTrainer;
				TrainerConnectionState = EBluetoothConnectionState::CONNECTING;

				// these are the PMS (power service) UUIDs, used as a fallback if FTMS is not present
				ServiceUUID = "00001818-0000-1000-8000-00805f9b34fb";
				CharacteristicUUID = "00002a63-0000-1000-8000-00805f9b34fb";

				Callback = [this](SimpleBLE::ByteArray Payload)
				{
					ReceivePowerNotification(Payload);
				};
				break;
			default:
				break;
			}

			if (PeripheralToConnect != nullptr)
			{
				ConnectToPeripheral(PeripheralToConnect);
				
				if(PeripheralToConnect->is_connected())
				{ 
					// for the trainer, we want to connect to FTMS, if the device does not have FTMS then we will use PMS as a fallback!
					if (RequestedDevice == EBluetoothConnectionDevice::TRAINER)
					{
						// check for FTMS
						std::string FTMSUUID = "00001826-0000-1000-8000-00805f9b34fb";
						FTMSPresent = false;
						UE_LOG(LogTemp, Warning, TEXT("LOOKING FOR FTMS"))
						for (SimpleBLE::Service& service : PeripheralToConnect->services())
						{
							
							if (service.uuid() == FTMSUUID)
							{
								UE_LOG(LogTemp, Warning, TEXT("FTMS FOUND!"));
								FTMSPresent = true;
								
								ServiceUUID = FTMSUUID;
								CharacteristicUUID = "00002ad2-0000-1000-8000-00805f9b34fb";
								HandleFTMSSetup();
								break; // we've found FTMS so no point in continuing
								// TODO: Maybe check for the bike power characteristic as well?
							}
						}
					}


					HandleNotificationSetup(PeripheralToConnect, ServiceUUID, CharacteristicUUID, Callback); 
				}

				switch (RequestedDevice)
				{
				case EBluetoothConnectionDevice::FRAME:
					FrameConnectionState = EBluetoothConnectionState::CONNECTED;
					break;
				case EBluetoothConnectionDevice::TRAINER:
					TrainerConnectionState = EBluetoothConnectionState::CONNECTED;
					break;
				default:
					break;
				}
			}
		 }


		 if (RequestedGearChange)
		 {
			 RequestedGearChange = false;

			 ChangeGear();
		 }

		FPlatformProcess::Sleep(0.03F);
	 }
    return 0;
}

void BikeBluetoothInput::Stop()
{
	UE_LOG(LogTemp, Warning, TEXT("STOP of bluetooth task thread"));
	UE_LOG(LogTemp, Warning, TEXT("Disconnecting connected devices"));


	if (ConnectedFrame.initialized() && ConnectedFrame.is_connected())
	{
		ConnectedFrame.disconnect();
		FrameConnectionState = EBluetoothConnectionState::DISCONNECTED;
		UE_LOG(LogTemp, Warning, TEXT("Frame device disconnected"));
		
	}
   
	if (ConnectedTrainer.initialized() && ConnectedTrainer.is_connected())
	{
		ConnectedTrainer.disconnect();
		TrainerConnectionState = EBluetoothConnectionState::DISCONNECTED;
		UE_LOG(LogTemp, Warning, TEXT("Trainer device disconnected"));
	}

	UE_LOG(LogTemp, Warning, TEXT("Devices all disconnected"));

    bRunThread = false;
}