// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "ChaosModularVehicle/VehicleSimEngineComponent.h"
#include "SimModule/EngineModule.h"
#include "VehicleSimFuelEngineComponent.generated.h"

class UVehicleSimFuelEngineComponent;

namespace Chaos
{
	struct FFuelEngineOutputData : public FEngineOutputData
	{
		virtual FSimOutputData* MakeNewData() override { return FFuelEngineOutputData::MakeNew(); }
		static FSimOutputData* MakeNew() { return new FFuelEngineOutputData(); }
		
		virtual void FillOutputState(const ISimulationModuleBase* SimModule) override;
		virtual void Lerp(const FSimOutputData& InCurrent, const FSimOutputData& InNext, float Alpha) override;
	public:
		float BrakeTorque;
		float LoadTorque;

		float ThrottleValue;
	};
	
	class FFuelEngineSimModule : public FEngineSimModule
	{
	public:
		DEFINE_CHAOSSIMTYPENAME(FFuelEngineSimModule);
		CHAOSVEHICLEEXTEND_API FFuelEngineSimModule(const FEngineSettings& Settings, const UVehicleSimFuelEngineComponent* InOwner);

		virtual ~FFuelEngineSimModule() {}
		
		UAbilitySystemComponent* GetAbilitySystemComponent() const;

		float GetFuelAttributeFloat() const;

		bool HasFuel() const;

		bool IsEngineActivate() const;

        virtual FSimOutputData* GenerateOutputData() const override
        {
	        return FFuelEngineOutputData::MakeNew();
        }

		virtual void Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem) override;

		float GetThrottleValue() const { return ThrottleValue; }
	protected:
		float ThrottleValue;
		const UVehicleSimFuelEngineComponent* Owner;
	};
}

UCLASS(ClassGroup = (ModularVehicle), meta = (BlueprintSpawnableComponent), hidecategories = (Object, Tick, Replication, Cooking, Activation, LOD))
class CHAOSVEHICLEEXTEND_API UVehicleSimFuelEngineComponent : public UVehicleSimEngineComponent
{
	GENERATED_BODY()

public:
	UVehicleSimFuelEngineComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FGameplayAttribute FuelAttribute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FGameplayTag EngineActivateTag;

	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
};
