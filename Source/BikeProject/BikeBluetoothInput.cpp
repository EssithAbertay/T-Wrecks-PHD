// Fill out your copyright notice in the Description page of Project Settings.


#include "BikeBluetoothInput.h"

#include "Misc/Paths.h" 

#include "Engine/Engine.h" 

#include "BikeGameInstance.h"
#include "BikeLoginInstanceSubsystem.h"

#include <string>

BikeBluetoothInput::BikeBluetoothInput(UBikeGameInstance* BikeInstanceRef)
{
    GameInstanceRef = BikeInstanceRef;
	LoginSubsystemRef = BikeInstanceRef->GetSubsystem<UBikeLoginInstanceSubsystem>();
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


	AsyncTask(ENamedThreads::GameThread, [this, Power]()
	{
		GameInstanceRef->SetBluetoothPower(Power);
	});
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

			AsyncTask(ENamedThreads::GameThread, [this, Bit]()
			{
				GameInstanceRef->BroadcastBluetoothInput(EBluetoothInputEvent::PRESSED, EBluetoothInputs(Bit));		
			});
		}
		else if (wasPressed && pressed) // held this tick
		{
			// send held signal
			AsyncTask(ENamedThreads::GameThread, [this, Bit]()
			{
				GameInstanceRef->BroadcastBluetoothInput(EBluetoothInputEvent::HELD, EBluetoothInputs(Bit));
			});

		}
		else if (wasPressed && !pressed) // let go this tick
		{
			// send released signal
			AsyncTask(ENamedThreads::GameThread, [this, Bit]()
			{
				GameInstanceRef->BroadcastBluetoothInput(EBluetoothInputEvent::RELEASED, EBluetoothInputs(Bit));
			});
		}
		else // not pressed at all
		{
			// don't do anything, will be handled in let go this tick
		}
	}

	uint32 NewControllerData = ControllerData;
	// tell it on game thread for safety
	AsyncTask(ENamedThreads::GameThread, [this, NewControllerData]()
	{
		GameInstanceRef->SetControllerState(NewControllerData);
	});
}

void BikeBluetoothInput::ReceieveHeartRateNotification(SimpleBLE::ByteArray payload)
{
	if (payload.size() < 2)
		return;

	int HeartRate = payload[1];
	
	AsyncTask(ENamedThreads::GameThread, [this, HeartRate]()
	{
			GameInstanceRef->SetHeartRate(HeartRate);
	});

	FString RawData;

	for (uint8 Byte : payload)
	{
		RawData += FString::Printf(TEXT("%02X "), static_cast<uint8>(Byte));
	}

	UE_LOG(LogTemp, Warning, TEXT("HR Raw Data: %s"), *RawData);
}

bool BikeBluetoothInput::ConnectToPeripheral(SimpleBLE::Peripheral* PeripheralToConnect, const FString& PeripheralAddress)
{
	auto It = ClassifiedPeripherals.find(TCHAR_TO_UTF8(*PeripheralAddress));

	if (It != ClassifiedPeripherals.end())
	{
		*PeripheralToConnect = It->second;
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

EBluetoothConnectionState BikeBluetoothInput::GetHRConnectionState()
{
	return HRConnectionState;
}

// as this gets data from the actual connecction variable we need to be more careful with it, attempting to get this during connection will cause unreal to freeze

FString BikeBluetoothInput::GetConnectedFrameName()
{
	if (FrameConnectionState != EBluetoothConnectionState::CONNECTING && ConnectedFrame.initialized() && ConnectedFrame.is_connected())
	{
		return UTF8_TO_TCHAR(ConnectedFrame.identifier().c_str());
	}

	return TEXT("N/A");
}

FString BikeBluetoothInput::GetConnectedTrainerName()
{
	if (TrainerConnectionState != EBluetoothConnectionState::CONNECTING && ConnectedTrainer.initialized() && ConnectedTrainer.is_connected())
	{
		return UTF8_TO_TCHAR(ConnectedTrainer.identifier().c_str());
	}

	return TEXT("N/A");
}

FString BikeBluetoothInput::GetConnectedHRMonitorName()
{

	if (HRConnectionState != EBluetoothConnectionState::CONNECTING && ConnectedHRMonitor.initialized() && ConnectedHRMonitor.is_connected())
	{
		return UTF8_TO_TCHAR(ConnectedHRMonitor.identifier().c_str());
	}
	
	return TEXT("N/A");
}

FString BikeBluetoothInput::GetConnectedFrameAddress()
{
	if (ConnectedFrame.initialized() && ConnectedFrame.is_connected())
	{
		return UTF8_TO_TCHAR(ConnectedFrame.address().c_str());
	}

	return TEXT("NoFrameAddress");
}

FString BikeBluetoothInput::GetConnectedTrainerAddress()
{
	if (ConnectedTrainer.initialized() && ConnectedTrainer.is_connected())
	{
		return UTF8_TO_TCHAR(ConnectedTrainer.address().c_str());
	}

	return TEXT("NoTrainerAddress");
}

FString BikeBluetoothInput::GetConnectedHRMonitorAddress()
{
	if (ConnectedHRMonitor.initialized() && ConnectedHRMonitor.is_connected())
	{
		return UTF8_TO_TCHAR(ConnectedHRMonitor.address().c_str());
	}

	return TEXT("NoHRMonitorAddress");
}

void BikeBluetoothInput::RequestConnection(const FString& p_RequestedConnectionAddress, EBluetoothConnectionDevice p_RequestedDevice)
{
	 FBluetoothConnectionRequest Request;
	 Request.Address = p_RequestedConnectionAddress;
	 Request.DeviceType = p_RequestedDevice;

	 // Added request to queue
	 ConnectionRequestQueue.Enqueue(Request);


	 UE_LOG(LogTemp, Warning, TEXT("Request recieved and added to queue"));
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
	
	if (NewResistance != 0)
	{
		int IncrementFix = NewResistance % ResistanceIncrement;
		NewResistance -= IncrementFix;
	}
		
	RequestedGearChange = true;
}

void BikeBluetoothInput::HandleFTMSSetup()
{
	// FTMS setup, we want to read the resistance max, increment, and current, and set up how it can be changed!

	// we rewrite the FTMS UUID so much that it should really be stored as a variable 

	// THESE ARE RESISTANCE UUIDS
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
	if (!ConnectedTrainer.initialized() || !ConnectedTrainer.is_connected())
	{
		return;
	}

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

bool BikeBluetoothInput::ClassifyPeripheral(SimpleBLE::Peripheral& Peripheral, EBluetoothConnectionDevice& OutDeviceType)
{
	for (SimpleBLE::Service& service : Peripheral.services())
	{
		// get the UUID of each service
		FString UUID = UTF8_TO_TCHAR(service.uuid().c_str());

		// for the trainer we look for FTMS or PMS, PMS is a fallback though so would rather avoid, for the Frame/Controller we look for the custom controller service, and for the heartrate monitor we look for the heartrate service

		if (UUID.Equals(PMSServiceUUID.c_str()) || UUID.Equals(FTMSServiceUUID.c_str()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Classified as Trainer!"));
			OutDeviceType = EBluetoothConnectionDevice::TRAINER;
			return true;
		}
		else if (UUID.Equals(ControllerServiceUUID.c_str()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Classified as Controller/Frame!"));
			OutDeviceType = EBluetoothConnectionDevice::FRAME;
			return true;
		}
		else if (UUID.Equals(HeartRateServiceUUID.c_str()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Classified as HR Monitor!"));
			OutDeviceType = EBluetoothConnectionDevice::HEARTRATE;
			return true;
		}
	}
	return false;
}

bool BikeBluetoothInput::CheckReconnect(FString DeviceAddress, EBluetoothConnectionDevice& OutDeviceType)
{
	// check reconnection is actually enabled
	if (!LoginSubsystemRef->GetDeviceReconnection().AutoReconnectEnabled)
	{
		return false;
	}

	// this is probs really inefficient but whatever
	if (LoginSubsystemRef->GetDeviceReconnection().FrameAddress == DeviceAddress)
	{
		OutDeviceType = EBluetoothConnectionDevice::FRAME;
		return true;
	}
	else if (LoginSubsystemRef->GetDeviceReconnection().TrainerAddress == DeviceAddress)
	{
		OutDeviceType = EBluetoothConnectionDevice::TRAINER;
		return true;
	}
	if (LoginSubsystemRef->GetDeviceReconnection().HRMonitorAddress == DeviceAddress)
	{
		OutDeviceType = EBluetoothConnectionDevice::HEARTRATE;
		return true;
	}

	return false;
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
	
	Adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral Peripheral) 
	{
		std::string id = Peripheral.identifier();
		std::string address = Peripheral.address();

		FString DeviceID = UTF8_TO_TCHAR(id.c_str());
		FString DeviceAddress = UTF8_TO_TCHAR(address.c_str());

		EBluetoothConnectionDevice DeviceType;

		// this will cause an issue if we recieve a device before one has connected
		// Check if this device was on the reconnection list, if so set up autoconnection
		if (CheckReconnect(DeviceAddress, DeviceType))
		{
			// TODO SKIP THIS ADDITION TO THE MAP, IT FEELS REALLY UGLY AND IS VERY MUCH BAND AID, only reason it works is because otherwise the connectperipheral function will fail on search
			ClassifiedPeripherals.insert(std::make_pair(Peripheral.address(), Peripheral));
			RequestConnection(DeviceAddress, DeviceType);
		}
		else
		{
			if (Peripheral.is_connectable() && DeviceID != "") // if connectable and has an actual name, all our intended devices have IDs so can filter out others for sake of ease of use
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("SCAN FOUND Callback! Device: %s"),
					*DeviceID
				);


				// check if we can classify this peripheral, if we can then do so, otherwise add it to the list to attempt to reclassify later
				if (ClassifyPeripheral(Peripheral, DeviceType))
				{
					ClassifiedPeripherals.insert(std::make_pair(Peripheral.address(), Peripheral));

					//inform game instance of the new device
					AsyncTask(ENamedThreads::GameThread, [this, DeviceID, DeviceType, DeviceAddress]()
					{
						GameInstanceRef->BroadcastNewDevice(DeviceID, DeviceType, DeviceAddress); // this needs changed to use address instead of array position as we dont use the vector anymore
					});
				}
				else
				{
					PendingPeripherals.insert(std::make_pair(Peripheral.address().c_str(), Peripheral));
				}

			}
		}
	});

	Adapter.set_callback_on_scan_updated([&](SimpleBLE::Peripheral Peripheral)
	{
		std::string id = Peripheral.identifier();
		std::string address = Peripheral.address();

		FString DeviceID = UTF8_TO_TCHAR(id.c_str());
		FString DeviceAddress = UTF8_TO_TCHAR(address.c_str());

		auto PendingIt = PendingPeripherals.find(Peripheral.address());
		if (PendingIt != PendingPeripherals.end()) // if connectable and has an actual name, all our intended devices have IDs so can filter out others for sake of ease of use
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Updated Callback! Device: %s"),
				*DeviceID
			);

			EBluetoothConnectionDevice DeviceType;

			// attempt to reclassify the peripheral
			if (ClassifyPeripheral(Peripheral, DeviceType))
			{
				// if the peripheral could now be found, move it to the classified peripheral map and remove from pending
				ClassifiedPeripherals.emplace(PendingIt->first, std::move(PendingIt->second));
				PendingPeripherals.erase(PendingIt);
				AsyncTask(ENamedThreads::GameThread, [this, DeviceID, DeviceType, DeviceAddress]()
				{
					GameInstanceRef->BroadcastNewDevice(DeviceID, DeviceType, DeviceAddress);
				});
			}
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
			 }
		 }

		 if (RequestedScanStop)
		 {
			 RequestedScanStop = false;

			 if (Adapter.scan_is_active()) // stop a scan if one is active
			 {
				 Adapter.scan_stop();
			 }
		 }

		 // for each item in the queue begin connection
		 FBluetoothConnectionRequest RequestedConnection;
		 while (ConnectionRequestQueue.Dequeue(RequestedConnection))
		 { 
			 if (IsInGameThread())
			 {
				 UE_LOG(LogTemp, Warning, TEXT("Running on Game Thread"));
			 }
			 else
			 {
				 UE_LOG(LogTemp, Warning, TEXT("Running on another thread"));
			 }


			SimpleBLE::Peripheral* PeripheralToConnect = nullptr;
			std::string ServiceUUID;
			std::string CharacteristicUUID;
			std::function<void(SimpleBLE::ByteArray)> Callback;

		
			switch (RequestedConnection.DeviceType)
			{
			case EBluetoothConnectionDevice::FRAME:
				UE_LOG(LogTemp, Warning, TEXT("connection to Frame!"));

				PeripheralToConnect = &ConnectedFrame;
				FrameConnectionState = EBluetoothConnectionState::CONNECTING;
				ServiceUUID = ControllerServiceUUID;
				CharacteristicUUID = ControllerCharacteristicUUID;

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
				ServiceUUID = PMSServiceUUID;
				CharacteristicUUID = PMSCharacteristicUUID;

				Callback = [this](SimpleBLE::ByteArray Payload)
				{
					ReceivePowerNotification(Payload);
				};
				break;
			case EBluetoothConnectionDevice::HEARTRATE:
				UE_LOG(LogTemp, Warning, TEXT("connection to HR Sensor!"));

				PeripheralToConnect = &ConnectedHRMonitor;
				HRConnectionState = EBluetoothConnectionState::CONNECTING;

				// these are the PMS (power service) UUIDs, used as a fallback if FTMS is not present
				ServiceUUID = HeartRateServiceUUID;
				CharacteristicUUID = HeartRateCharacteristicUUID;

				Callback = [this](SimpleBLE::ByteArray Payload)
				{
					ReceieveHeartRateNotification(Payload);
				};
				break;
			default:
				break;
			}

			if (PeripheralToConnect != nullptr)
			{
				ConnectToPeripheral(PeripheralToConnect, RequestedConnection.Address);

				if (PeripheralToConnect->is_connected())
				{
					// for the trainer, we want to connect to FTMS, if the device does not have FTMS then we will use PMS as a fallback!
					if (RequestedConnection.DeviceType == EBluetoothConnectionDevice::TRAINER)
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

					switch (RequestedConnection.DeviceType)
					{
					case EBluetoothConnectionDevice::FRAME:
						FrameConnectionState = EBluetoothConnectionState::CONNECTED;
						break;
					case EBluetoothConnectionDevice::TRAINER:
						TrainerConnectionState = EBluetoothConnectionState::CONNECTED;
						break;
					case EBluetoothConnectionDevice::HEARTRATE:
						HRConnectionState = EBluetoothConnectionState::CONNECTED;
					default:
						break;
					}

					HandleNotificationSetup(PeripheralToConnect, ServiceUUID, CharacteristicUUID, Callback);
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

	if (ConnectedHRMonitor.initialized() && ConnectedHRMonitor.is_connected())
	{
		ConnectedHRMonitor.disconnect();
		HRConnectionState = EBluetoothConnectionState::DISCONNECTED;
		UE_LOG(LogTemp, Warning, TEXT("Heartrate Monitor disconnected"));
	}


	UE_LOG(LogTemp, Warning, TEXT("Devices all disconnected"));

    bRunThread = false;
}