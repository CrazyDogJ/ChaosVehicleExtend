// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuoyancyTypes.h"
#include "ChaosModularVehicle/VehicleSimBaseComponent.h"
#include "SimModule/SimulationModuleBase.h"
#include "VehicleSimBuoyancyComponent.generated.h"

class UVehicleSimBuoyancyComponent;

namespace Chaos
{
	struct FAllInputs;
	class FSimModuleTree;
	
	struct FBuoyancySettings
	{
		FBuoyancySettings(): BuoyancyRampMinVelocity(20.f), BuoyancyRampMaxVelocity(50.f), BuoyancyRampMax(1.f),
		                     BuoyancyCoefficient(1.0f), MaxBuoyantForce(4000000.f)
		{
		}

		float BuoyancyRampMinVelocity;
		float BuoyancyRampMaxVelocity;
		float BuoyancyRampMax;
		float BuoyancyCoefficient;
		float MaxBuoyantForce;
		TArray<FSphericalPontoon> Pontoons;
	};
	
	class FBuoyancySimModule : public ISimulationModuleBase, public TSimModuleSettings<FBuoyancySettings>, public TSimulationModuleTypeable<FBuoyancySimModule>
	{
	public:
		DEFINE_CHAOSSIMTYPENAME(FBuoyancySimModule);
		CHAOSVEHICLEEXTEND_API FBuoyancySimModule(const FBuoyancySettings& Settings, UWorld* InWorld);
		
		void UpdateCurrentWaterBodies(const FAllInputs& Inputs);

		void UpdatePontoons(const FAllInputs& Inputs, const FSimModuleTree& VehicleModuleSystem);
		
		void ComputeBuoyancy(FSphericalPontoon& Pontoon, float ForwardSpeedKmh) const;
		
		virtual TSharedPtr<FModuleNetData> GenerateNetData(const int32 NodeArrayIndex) const override { return nullptr; }

		virtual const FString GetDebugName() const override { return TEXT("SteerThruster"); }

		virtual bool IsBehaviourType(eSimModuleTypeFlags InType) const override { return (InType & NonFunctional); }

		CHAOSVEHICLEEXTEND_API virtual void Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem) override;
		
	private:
		UWorld* World;
		TArray<UWaterBodyComponent*> CurrentWaterBodies;
		TArray<FSphericalPontoon> Pontoons;
	};


} // namespace Chaos

UCLASS(ClassGroup = (ModularVehicle), meta = (BlueprintSpawnableComponent), hidecategories = (Object, Tick, Replication, Cooking, Activation, LOD))
class CHAOSVEHICLEEXTEND_API UVehicleSimBuoyancyComponent : public UVehicleSimBaseComponent
{
	GENERATED_BODY()

public:
	UVehicleSimBuoyancyComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attribute)
	TArray<FSphericalPontoon> Pontoons;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attribute)
	float BuoyancyRampMinVelocity = 20.0f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attribute)
	float BuoyancyRampMaxVelocity = 50.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attribute)
	float BuoyancyRampMax = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attribute)
	float BuoyancyCoefficient = 1.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attribute)
	float MaxBuoyantForce = 4000000.f;
	
	virtual ESimModuleType GetModuleType() const override { return ESimModuleType::Thruster; }

	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
};
