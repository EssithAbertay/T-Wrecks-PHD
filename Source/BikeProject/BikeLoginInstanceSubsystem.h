// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "Kismet/GameplayStatics.h"

#include "BikeGameInstance.h"

#include "BikeProjectSaveGame.h"
#include "BikePlayerSave.h"

#include "BikeLoginInstanceSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class BIKEPROJECT_API UBikeLoginInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	// Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// End USubsystem


	UFUNCTION(BlueprintCallable)
	bool CreateUser(const FString& Username);


	UFUNCTION(BlueprintCallable)
	FString CreateUniqueUsername();

	UFUNCTION(BlueprintCallable)
	void SaveAccounts();

	UFUNCTION(BlueprintCallable)
	bool LoginUser(const FString& Username);

	UFUNCTION(BlueprintCallable)
	void SaveUser();

	UFUNCTION(BlueprintCallable) // sets the stats used by the current save file, prepping for a save
	void SetUserStats();

	// returns an uneditable reference to the accounts
	UFUNCTION(BlueprintCallable)
	const TArray<FUserAccount>& GetAllAccounts() const;

	UFUNCTION(BlueprintCallable)
	TArray<FString> GetAllAccountNames();

	UFUNCTION(BlueprintCallable)
	FString GetCurrentPlayerName();

	// returns uneditable reference to top 10 accounts (by score)
	UFUNCTION(BlueprintCallable)
	const TArray<FUserAccount> GetTopTenAccounts() const;

	// const, make sure youve sorted the array before using this, as this function will not. Give actual place looking for, this function will -1 to account for array
	UFUNCTION(BlueprintCallable)
	const FUserAccount GetAccountByRank(int Rank) const;

	UFUNCTION(BlueprintCallable)
	void SortAccounts();

	UFUNCTION(BLueprintCallable)
	void SetDeviceReconnection(FDeviceReconnection Reconnection);

	UFUNCTION(BLueprintCallable)
	FDeviceReconnection GetDeviceReconnection() const; // will return the full struct


	UFUNCTION(BlueprintCallable)
	bool GetCurrentPlayerTutorialState() const; // True for completed tutorial

	UFUNCTION(BlueprintCallable)
	void SetCurrentPlayerTutorialState(bool NewState); // set if tutorial has been completed
private:
	bool IsLoggedIn = false;


	FString RandomAdjective();
	FString RandomDinosaur();

	bool LoadAdjectives();
	bool LoadDinosaurs();

	void LoadAccounts();

	bool CheckForExistingAccountByUsername(const FString& Username);

	UPROPERTY()
	UBikeProjectSaveGame* AccountDatabase;

	UPROPERTY()
	UBikePlayerSave* CurrentPlayerSave;

	UPROPERTY()
	FUserAccount CurrentPlayerAccount;

	// All my variables
	TArray<FString> Adjectives;
	TArray<FString> Dinosaurs;
};
