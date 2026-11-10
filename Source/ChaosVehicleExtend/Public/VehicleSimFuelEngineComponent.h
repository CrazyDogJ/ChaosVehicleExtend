// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "ChaosModularVehicle/VehicleSimEngineComponent.h"
#include "SimModule/EngineModule.h"
#include "VehicleSimFuelEngineComponent.generated.h"

class UVehicleSimFuelEngineComponent;
struct FVehicleEngineRuntimeState;

namespace Chaos
{
	struct FFuelEngineOutputData : public FEngineOutputData
	{
		// 修复：区分普通引擎输出和含油门字段的派生输出。
		static FName FuelOutputType() { static const FName Type(TEXT("FuelEngineOutput")); return Type; }
		FFuelEngineOutputData() { AddType(FuelOutputType()); }
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
		// 修复：模块的生命周期与所属 UObject 独立。
		TWeakObjectPtr<const UVehicleSimFuelEngineComponent> Owner;
		TWeakPtr<FVehicleEngineRuntimeState, ESPMode::ThreadSafe> EngineState;
	};
}

UCLASS(ClassGroup = (ModularVehicle), meta = (BlueprintSpawnableComponent), hidecategories = (Object, Tick, Replication, Cooking, Activation, LOD))
class CHAOSVEHICLEEXTEND_API UVehicleSimFuelEngineComponent : public UVehicleSimEngineComponent
{
	GENERATED_BODY()

public:
	UVehicleSimFuelEngineComponent();
	TSharedPtr<FVehicleEngineRuntimeState, ESPMode::ThreadSafe> GetEngineState() const;

protected:
	// 修复：组件注销时物理端的弱快照随即失效。
	virtual void OnUnregister() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	mutable TSharedPtr<FVehicleEngineRuntimeState, ESPMode::ThreadSafe> EngineState;

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FGameplayAttribute FuelAttribute;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FGameplayTag EngineActivateTag;

	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
	virtual void OnOutputReady(const Chaos::FSimOutputData* OutputData) override {}
};
