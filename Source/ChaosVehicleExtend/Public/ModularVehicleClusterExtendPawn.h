// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularVehicleExtendComponent.h"
#include "ChaosModularVehicle/ModularVehicleClusterPawn.h"
#include "ModularVehicleClusterExtendPawn.generated.h"

UCLASS(BlueprintType, Blueprintable)
class CHAOSVEHICLEEXTEND_API AModularVehicleClusterExtendPawn : public AModularVehicleClusterPawn
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Vehicle Extend")
	UModularVehicleExtendComponent* VehicleExtendComponent;
	
	UFUNCTION()
	UModularVehicleExtendComponent* GetVehicleSimulationExtendComponent() const { return Cast<UModularVehicleExtendComponent>(VehicleSimComponent); }
};
