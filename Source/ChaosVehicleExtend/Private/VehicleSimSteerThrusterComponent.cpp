// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleSimSteerThrusterComponent.h"
#include "VehicleEngineRuntimeState.h"

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
		if (InComp) EngineState = InComp->GetEngineState();
		SimulationWorld = InComp ? InComp->GetWorld() : nullptr;
		WaterChannel = GetDefault<UWaterRuntimeSettings>()->CollisionChannelForWaterTraces;
	}

	UAbilitySystemComponent* FSteerThrusterSimModule::GetAbilitySystemComponent() const
	{
		if (IsInGameThread() && OuterComponent.IsValid() && OuterComponent->GetOwner())
		{
			return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OuterComponent->GetOwner());
		}

		return nullptr;
	}

	float FSteerThrusterSimModule::GetFuelAttributeFloat() const
	{
		const auto State = EngineState.Pin();
		return State ? State->Fuel.Load() : 0.0f;
	}

	bool FSteerThrusterSimModule::HasFuel() const
	{
		return GetFuelAttributeFloat() > 0.0f;
	}

	bool FSteerThrusterSimModule::IsEngineActivate() const
	{
		const auto State = EngineState.Pin();
		return State && State->bActive.Load();
	}

	void FSteerThrusterSimModule::Simulate(float DeltaTime, const FAllInputs& Inputs,
	                                       FSimModuleTree& VehicleModuleSystem)
	{
		const auto Radius = Setup().CheckInWaterSphereRadius;
		const auto Offset = Setup().WaterCheckOffset;
		const auto Location = Inputs.VehicleWorldTransform.TransformPosition(Offset);
		// 修复：在 GC 或世界销毁期间跳过查询，当前物理步内保持世界存活。
		bool bCanPin = false;
		const auto World = SimulationWorld.TryPin(bCanPin);
		bIsInWater = false;
		
		TArray<FHitResult> HitResultsOut;
		if (World.IsValid())
		{
			Chaos::Private::FGenericPhysicsInterface_Internal::SpherecastMulti(World.Get()
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

		// 修复：拆卸过程中输入可能已失效。
		if (Inputs.ControlInputs && bIsInWater && HasFuel() && IsEngineActivate())
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
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

TSharedPtr<FVehicleEngineRuntimeState, ESPMode::ThreadSafe> UVehicleSimSteerThrusterComponent::GetEngineState() const
{
	if (!EngineState) EngineState = MakeShared<FVehicleEngineRuntimeState, ESPMode::ThreadSafe>();
	EngineState->Refresh(GetOwner(), FuelAttribute, EngineActivateTag);
	return EngineState;
}

void UVehicleSimSteerThrusterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (EngineState) EngineState->Refresh(GetOwner(), FuelAttribute, EngineActivateTag);
}

void UVehicleSimSteerThrusterComponent::OnUnregister()
{
	EngineState.Reset();
	Super::OnUnregister();
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
