// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularVehicleExtendComponent.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChaosVehicleExtendLibrary.generated.h"

class UVehicleSimWheelComponent;
class UVehicleSimBaseComponent;
class UModularVehicleBaseComponent;

USTRUCT(BlueprintType)
struct FWheelRepSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UModularVehicleBaseComponent* VehicleBaseComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHandbrakeEnabled = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSteeringEnabled = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bTractionControlEnabled = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool ReverseDirection = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float MaxSteeringAngle = 45.0f;
};

UCLASS()
class CHAOSVEHICLEEXTEND_API UChaosVehicleExtendLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Vehicle")
	static float GetModularVehicleSpeed(const UModularVehicleBaseComponent* Comp);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	static void SetModularVehicleCOM(const UModularVehicleBaseComponent* Comp, const FVector& COM);
	
	UFUNCTION(BlueprintPure, Category = "Vehicle")
	static float GetModularVehicleRPM(const UModularVehicleBaseComponent* Comp);

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	static float GetModularVehicleTorque(const UModularVehicleBaseComponent* Comp);

	UFUNCTION(BlueprintPure, Category = "Vehicle")
	static float GetModularVehicleThrottle(const UModularVehicleBaseComponent* Comp);
	
	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	static void SetWheelRepSettings(USceneComponent* WheelComp, FWheelRepSettings Settings);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	static void SetWheelRepSettingsUpdate(USceneComponent* WheelComp, FWheelRepSettings Settings, FSimTreeUpdatesHandle Handle);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	static TMap<USceneComponent*, int> GetComponentToPhysicsObjects(const UModularVehicleBaseComponent* Comp);
};
