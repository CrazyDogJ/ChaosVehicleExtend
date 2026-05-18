// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosModularVehicle/VehicleSimBaseComponent.h"
#include "SimModule/SimulationModuleBase.h"
#include "VehicleSimSteerWheelComponent.generated.h"

namespace Chaos
{
	struct FSteerWheelSettings
	{
		FSteerWheelSettings()
		{
		}

		EAxis::Type Axis = EAxis::Z;
		float Angle = 90.0f;
	};

	class FSteerWheelSimModule : public ISimulationModuleBase, public TSimModuleSettings<FSteerWheelSettings>, public TSimulationModuleTypeable<FSteerWheelSimModule>
	{
	public:
		DEFINE_CHAOSSIMTYPENAME(FSteerWheelSimModule);
		CHAOSVEHICLEEXTEND_API FSteerWheelSimModule(const FSteerWheelSettings& Settings);
		
		virtual TSharedPtr<FModuleNetData> GenerateNetData(const int32 NodeArrayIndex) const override { return nullptr; }
		
		virtual const FString GetDebugName() const override { return TEXT("SteerWheel"); }

		virtual bool IsBehaviourType(eSimModuleTypeFlags InType) const override { return (InType & NonFunctional); }

		CHAOSVEHICLEEXTEND_API virtual void Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem) override;
		
		CHAOSVEHICLEEXTEND_API virtual void Animate() override;
	private:
		float SteeringInput;
	};
}

UCLASS(ClassGroup = (ModularVehicle), meta = (BlueprintSpawnableComponent), hidecategories = (Object, Tick, Replication, Cooking, Activation, LOD))
class CHAOSVEHICLEEXTEND_API UVehicleSimSteerWheelComponent : public UVehicleSimBaseComponent
{
	GENERATED_BODY()

public:
	UVehicleSimSteerWheelComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	TEnumAsByte<EAxis::Type> Axis = EAxis::Z;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float Angle = 90.0f;
	
	virtual ESimModuleType GetModuleType() const override { return ESimModuleType::Undefined; }

	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
};
