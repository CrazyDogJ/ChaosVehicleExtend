// Fill out your copyright notice in the Description page of Project Settings.


#include "ModularVehicleClusterExtendPawn.h"

#include "ModularVehicleExtendComponent.h"

AModularVehicleClusterExtendPawn::AModularVehicleClusterExtendPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass(TEXT("VehicleSimComponent0"), UModularVehicleExtendComponent::StaticClass()))
{
	VehicleExtendComponent = Cast<UModularVehicleExtendComponent>(VehicleSimComponent);
}
