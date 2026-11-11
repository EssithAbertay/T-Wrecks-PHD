// Fill out your copyright notice in the Description page of Project Settings.


#include "BikeLoginInstanceSubsystem.h"

void UBikeLoginInstanceSubsystem::Initialize( FSubsystemCollectionBase& Collection)
{ 
    Super::Initialize(Collection);
    UE_LOG(LogTemp, Warning, TEXT("Login System Initialised!"))

    bool AdjectivesLoaded = LoadAdjectives();
    bool DinosaurssLoaded = LoadDinosaurs();

    LoadAccounts();
}

void UBikeLoginInstanceSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

bool UBikeLoginInstanceSubsystem::CreateUser()
{
    if (!AccountDatabase)
    {
        UE_LOG(LogTemp, Error, TEXT("Account Database not loaded"));
        return false;
    }


    // create an account
    FUserAccount NewAccount;

    NewAccount.PlayerName = CreateUsername();
    FGuid AccountGUID = FGuid::NewGuid();
    NewAccount.SaveSlotName = FString::Printf(TEXT("Player_%s"), *AccountGUID.ToString());

    // add the new account to the slot
    AccountDatabase->UserAccounts.Add(NewAccount);

    //save the accounts, kinda self explanotry ahaha
    SaveAccounts();

    return true;
}


FString UBikeLoginInstanceSubsystem::CreateUsername()
{
    return RandomAdjective() + " " + RandomDinosaur();
}

void UBikeLoginInstanceSubsystem::SaveAccounts()
{
    if (!AccountDatabase)
    {
        return; // prevent attempting to save to something that doesn't exist
    }

    UGameplayStatics::SaveGameToSlot(AccountDatabase, TEXT("AccountDatabase"), 0);
}

bool UBikeLoginInstanceSubsystem::LoginUser(const FString& Username)
{
    if (!AccountDatabase)
    {
        UE_LOG(LogTemp, Error, TEXT("Account Database not loaded"));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("Looking for account"));
    FUserAccount* SelectedAccount = AccountDatabase->UserAccounts.FindByPredicate([&Username](const FUserAccount& User) {return User.PlayerName == Username; });

    // if the account doesn't exist
    if (!SelectedAccount)
    {
        UE_LOG(LogTemp, Warning, TEXT("Account not found"));
        return false;
    }
    UE_LOG(LogTemp, Warning, TEXT("account found!"));
    CurrentPlayerSave = Cast<UBikePlayerSave>(UGameplayStatics::LoadGameFromSlot(SelectedAccount->SaveSlotName, 0));

    // if the found account doesn't have a save yet
    if (!CurrentPlayerSave)
    {
        UE_LOG(LogTemp, Warning, TEXT("No Save found, creating one"));
        CurrentPlayerSave = Cast<UBikePlayerSave>(UGameplayStatics::CreateSaveGameObject(UBikePlayerSave::StaticClass()));

        if (!CurrentPlayerSave)
        {
            return false;
        }

        // Save the newly created save file
        UGameplayStatics::SaveGameToSlot(CurrentPlayerSave,SelectedAccount->SaveSlotName,0);
    }

    IsLoggedIn = true;
    CurrentPlayerAccount = *SelectedAccount;
    return true;
}

void UBikeLoginInstanceSubsystem::SaveUser()
{
    if (!CurrentPlayerSave || !IsLoggedIn)
    {
        UE_LOG(LogTemp, Warning, TEXT("No player logged in"));
        return;
    }

    UGameplayStatics::SaveGameToSlot(CurrentPlayerSave, CurrentPlayerAccount.SaveSlotName, 0);

    UE_LOG(LogTemp, Warning, TEXT("User Data Saved!"));
}

FString UBikeLoginInstanceSubsystem::RandomAdjective()
{
    if (Adjectives.Num() == 0)
    {
        return TEXT("");
    }

    const int32 RandomIndex = FMath::RandRange(0, Adjectives.Num() - 1);

    return Adjectives[RandomIndex];
}

FString UBikeLoginInstanceSubsystem::RandomDinosaur()
{
    if (Dinosaurs.Num() == 0)
    {
        return TEXT("");
    }

    const int32 RandomIndex = FMath::RandRange(0, Dinosaurs.Num() - 1);

    return Dinosaurs[RandomIndex];
}

bool UBikeLoginInstanceSubsystem::LoadAdjectives()
{
    FString FilePath = FPaths::ProjectDir() + TEXT("Adjectives.txt");

    if (FFileHelper::LoadFileToStringArray(Adjectives, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Loaded %d adjectives"), Adjectives.Num());
        return true;
    }

    return false;
}

bool UBikeLoginInstanceSubsystem::LoadDinosaurs()
{
    FString FilePath = FPaths::ProjectDir() + TEXT("Dinosaurs.txt");

    if (FFileHelper::LoadFileToStringArray(Dinosaurs, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Loaded %d Dinosaurs"), Dinosaurs.Num());
        return true;
    }

    return false;
}

void UBikeLoginInstanceSubsystem::LoadAccounts()
{
    const FString SlotName = TEXT("AccountDatabase");

    if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        AccountDatabase = Cast<UBikeProjectSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    }
    else
    {
        AccountDatabase = Cast<UBikeProjectSaveGame>(UGameplayStatics::CreateSaveGameObject(UBikeProjectSaveGame::StaticClass()));
        SaveAccounts();
    }
}

