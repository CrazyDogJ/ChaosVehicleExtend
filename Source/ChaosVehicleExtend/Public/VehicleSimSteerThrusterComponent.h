// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaosModularVehicle/VehicleSimBaseComponent.h"
#include "SimModule/SimulationModuleBase.h"

#include "VehicleSimSteerThrusterComponent.generated.h"

class UVehicleSimSteerThrusterComponent;

namespace Chaos
{
	struct FAllInputs;
	class FSimModuleTree;
	
	struct FSteerThrusterSettings
	{
		FSteerThrusterSettings()
			: MaxThrustForce(0)
			  , ForceAxis(FVector(1.0f, 0.0f, 0.0f))
			  , ForceOffset(FVector::ZeroVector)
			  , BoostMultiplier(1.0f)
			  , MaxSpeed(125.0f)
			  , MaxSteerThrustForce(0)
			  , SteerForceAxis(FVector(0.0f, 1.0f, 0.0f))
			  , SteerForceOffset(FVector::ZeroVector)
			  , SteerMaxSpeed(125.0f),
				CheckInWaterSphereRadius(5),
				WaterCheckOffset(FVector::ZeroVector)
		{
		}

		// Normal thruster
		float MaxThrustForce;
		FVector ForceAxis;
		FVector ForceOffset;
		float BoostMultiplier;
		float MaxSpeed;
		
		// Steer thruster
		float MaxSteerThrustForce;
		FVector SteerForceAxis;
		FVector SteerForceOffset;
		float SteerMaxSpeed;

		// In Water Check
		float CheckInWaterSphereRadius;
		FVector WaterCheckOffset;
	};
	
	class FSteerThrusterSimModule : public ISimulationModuleBase, public TSimModuleSettings<FSteerThrusterSettings>, public TSimulationModuleTypeable<FSteerThrusterSimModule>
	{
	public:
		DEFINE_CHAOSSIMTYPENAME(FSteerThrusterSimModule);
		CHAOSVEHICLEEXTEND_API FSteerThrusterSimModule(const FSteerThrusterSettings& Settings, UWorld* InWorld);

		virtual TSharedPtr<FModuleNetData> GenerateNetData(const int32 NodeArrayIndex) const override { return nullptr; }

		virtual const FString GetDebugName() const override { return TEXT("SteerThruster"); }

		virtual bool IsBehaviourType(eSimModuleTypeFlags InType) const override { return (InType & NonFunctional); }

		CHAOSVEHICLEEXTEND_API virtual void Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem) override;
		
		CHAOSVEHICLEEXTEND_API virtual void Animate() override;
		
	private:
		UWorld* World;
		bool bIsInWater;
		float SteerAngleDegrees;
	};


} // namespace Chaos

UCLASS(ClassGroup = (ModularVehicle), meta = (BlueprintSpawnableComponent), hidecategories = (Object, Tick, Replication, Cooking, Activation, LOD))
class CHAOSVEHICLEEXTEND_API UVehicleSimSteerThrusterComponent : public UVehicleSimBaseComponent
{
	GENERATED_BODY()

public:
	UVehicleSimSteerThrusterComponent();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float MaxThrustForce;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FVector ForceAxis;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FVector ForceOffset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float BoostMultiplier;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float MaxSpeed;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float MaxSteerThrustForce;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FVector SteerForceAxis;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FVector SteerForceOffset;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float SteerMaxSpeed;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	float CheckInWaterSphereRadius = 5.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Attributes)
	FVector WaterCheckOffset = FVector::ZeroVector;
	
	virtual ESimModuleType GetModuleType() const override { return ESimModuleType::Thruster; }

	virtual Chaos::ISimulationModuleBase* CreateNewCoreModule() const override;
};
