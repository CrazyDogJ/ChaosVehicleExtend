// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleSimSteerThrusterComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "WaterBodyActor.h"
#include "WaterRuntimeSettings.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"

namespace Chaos
{
	FSteerThrusterSimModule::FSteerThrusterSimModule(const FSteerThrusterSettings& Settings, const UVehicleSimSteerThrusterComponent* InComp)
		: TSimModuleSettings<FSteerThrusterSettings>(Settings), bIsInWater(false), SteerAngleDegrees(0)
	{
		OuterComponent = InComp;
	}

	UAbilitySystemComponent* FSteerThrusterSimModule::GetAbilitySystemComponent() const
	{
		if (OuterComponent && OuterComponent->GetOwner())
		{
			return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OuterComponent->GetOwner());
		}

		return nullptr;
	}

	float FSteerThrusterSimModule::GetFuelAttributeFloat() const
	{
		if (!OuterComponent)
		{
			return 0.0f;
		}
		
		if (const auto ASC = GetAbilitySystemComponent())
		{
			bool bFound = false;
			const auto FoundValue = ASC->GetGameplayAttributeValue(OuterComponent->FuelAttribute, bFound);
			if (bFound)
			{
				return FoundValue;
			}
		}

		return 0.0f;
	}

	bool FSteerThrusterSimModule::HasFuel() const
	{
		return GetFuelAttributeFloat() > 0.0f;
	}

	bool FSteerThrusterSimModule::IsEngineActivate() const
	{
		if (!OuterComponent)
		{
			return false;
		}
		
		if (const auto ASC = GetAbilitySystemComponent())
		{
			return ASC->HasMatchingGameplayTag(OuterComponent->EngineActivateTag);
		}

		return false;
	}

	void FSteerThrusterSimModule::Simulate(float DeltaTime, const FAllInputs& Inputs,
	                                       FSimModuleTree& VehicleModuleSystem)
	{
		const auto Radius = Setup().CheckInWaterSphereRadius;
		const auto Offset = Setup().WaterCheckOffset;
		const auto Location = Inputs.VehicleWorldTransform.TransformPosition(Offset);
		const ECollisionChannel WaterChannel = GetDefault<UWaterRuntimeSettings>()->CollisionChannelForWaterTraces;
		
		TArray<FHitResult> HitResultsOut;
		if (OuterComponent)
		{
			Chaos::Private::FGenericPhysicsInterface_Internal::SpherecastMulti(OuterComponent->GetWorld()
								, Radius
								, HitResultsOut
								, Location
								, Location
								, WaterChannel
								, FCollisionQueryParams::DefaultQueryParam
								, FCollisionResponseParams::DefaultResponseParam);
		}

		for (const auto Itr : HitResultsOut)
		{
			if (!Itr.GetActor())
			{
				continue;
			}
			
			if (Itr.GetActor()->GetComponentByClass<UWaterBodyComponent>())
			{
				bIsInWater = true;
				break;
			}
			
			bIsInWater = false;
		}

		if (bIsInWater && HasFuel() && IsEngineActivate())
		{
			// Inputs.
			const auto Reverse = Inputs.GetControls().GetMagnitude(ReverseControlName);
			const auto Throttle = Inputs.GetControls().GetMagnitude(ThrottleControlName);
			const auto FinalThrottle = Reverse > 0.0f ? -Reverse : Throttle;
			const auto FinalSteering = Reverse > 0.0f ? 1 : -1;
			
			// Forward
			float BoostEffect = Inputs.GetControls().GetMagnitude(BoostControlName) * Setup().BoostMultiplier;
			FVector Force = Setup().ForceAxis * Setup().MaxThrustForce * FinalThrottle * (1.0f + BoostEffect);
			AddLocalForceAtPosition(Force, Setup().ForceOffset, true, false, false, FColor::Magenta);

			// Side
			SteerAngleDegrees = Inputs.GetControls().GetMagnitude(SteeringControlName);
			const FVector SteerForce = Setup().SteerForceAxis * Setup().MaxSteerThrustForce * SteerAngleDegrees * FinalSteering;
			AddLocalForceAtPosition(SteerForce, Setup().SteerForceOffset, true, false, false, FColor::Magenta);
		}
	}

	void FSteerThrusterSimModule::Animate()
	{
		AnimationData.AnimFlags = EAnimationFlags::AnimateRotation;
		AnimationData.AnimationRotOffset.Yaw = SteerAngleDegrees;
	}
}

UVehicleSimSteerThrusterComponent::UVehicleSimSteerThrusterComponent()
	:MaxThrustForce(0)
	,ForceAxis(FVector(1.0f, 0.0f, 0.0f))
	,ForceOffset(FVector::ZeroVector)
	,BoostMultiplier(1.0f)
	,MaxSpeed(125.0f)
	,MaxSteerThrustForce(0)
	,SteerForceAxis(FVector(0.0f, 1.0f, 0.0f))
	,SteerForceOffset(FVector::ZeroVector)
	,SteerMaxSpeed(125.0f)
{
}

Chaos::ISimulationModuleBase* UVehicleSimSteerThrusterComponent::CreateNewCoreModule() const
{
	Chaos::FSteerThrusterSettings Settings;

	Settings.MaxThrustForce = MaxThrustForce;
	Settings.ForceAxis = ForceAxis;
	Settings.ForceOffset = ForceOffset;
	Settings.BoostMultiplier = BoostMultiplier;
	Settings.MaxSteerThrustForce = MaxSteerThrustForce;
	Settings.SteerForceAxis = SteerForceAxis;
	Settings.SteerForceOffset = SteerForceOffset;
	Settings.SteerMaxSpeed = SteerMaxSpeed;
	Settings.WaterCheckOffset = WaterCheckOffset;
	Settings.CheckInWaterSphereRadius = CheckInWaterSphereRadius;

	Chaos::ISimulationModuleBase* Thruster = new Chaos::FSteerThrusterSimModule(Settings, this);
	Thruster->SetAnimationEnabled(bAnimationEnabled);

	return Thruster;
}
