// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleSimFuelEngineComponent.h"
#include "VehicleEngineRuntimeState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

namespace Chaos
{
	void FFuelEngineOutputData::FillOutputState(const ISimulationModuleBase* SimModule)
	{
		FEngineOutputData::FillOutputState(SimModule);

		check(SimModule->IsSimType<class FEngineSimModule>());

		FSimOutputData::FillOutputState(SimModule);

		if (const FFuelEngineSimModule* Sim = static_cast<const FFuelEngineSimModule*>(SimModule))
		{
			Torque = Sim->GetDriveTorque();
			LoadTorque = Sim->GetLoadTorque();
			BrakeTorque = Sim->GetBrakingTorque();
			ThrottleValue = Sim->GetThrottleValue();
		}
	}

	void FFuelEngineOutputData::Lerp(const FSimOutputData& InCurrent, const FSimOutputData& InNext, float Alpha)
	{
		// 修复：模块替换时相邻帧不一定都是燃料引擎输出。
		if (!InCurrent.IsSimType(FuelOutputType()) || !InNext.IsSimType(FuelOutputType())) return;
		FEngineOutputData::Lerp(InCurrent, InNext, Alpha);
		
		const FFuelEngineOutputData& Current = static_cast<const FFuelEngineOutputData&>(InCurrent);
		const FFuelEngineOutputData& Next = static_cast<const FFuelEngineOutputData&>(InNext);

		RPM = FMath::Lerp(Current.RPM, Next.RPM, Alpha);
		Torque = FMath::Lerp(Current.Torque, Next.Torque, Alpha);
		LoadTorque = FMath::Lerp(Current.LoadTorque, Next.LoadTorque, Alpha);
		BrakeTorque = FMath::Lerp(Current.BrakeTorque, Next.BrakeTorque, Alpha);
		ThrottleValue = FMath::Lerp(Current.ThrottleValue, Next.ThrottleValue, Alpha);
	}

	FFuelEngineSimModule::FFuelEngineSimModule(const FEngineSettings& Settings, const UVehicleSimFuelEngineComponent* InOwner)
		: FEngineSimModule(Settings), ThrottleValue(0)
	{
		Owner = InOwner;
		if (InOwner) EngineState = InOwner->GetEngineState();
	}

	UAbilitySystemComponent* FFuelEngineSimModule::GetAbilitySystemComponent() const
	{
		if (IsInGameThread() && Owner.IsValid() && Owner->GetOwner())
		{
			return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner->GetOwner());
		}

		return nullptr;
	}

	float FFuelEngineSimModule::GetFuelAttributeFloat() const
	{
		const auto State = EngineState.Pin();
		return State ? State->Fuel.Load() : 0.0f;
	}

	bool FFuelEngineSimModule::HasFuel() const
	{
		return GetFuelAttributeFloat() > 0.0f;
	}

	bool FFuelEngineSimModule::IsEngineActivate() const
	{
		const auto State = EngineState.Pin();
		return State && State->bActive.Load();
	}

	void FFuelEngineSimModule::Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem)
	{
		// 修复：车辆拆卸期间输入接口可能已清空。
		if (!Inputs.ControlInputs) { ThrottleValue = 0.0f; return; }
		ThrottleValue = Inputs.GetControls().GetMagnitude(ThrottleControlName);
		
		const auto bHasFuel = HasFuel();
		const auto bEngineActive = IsEngineActivate();
		
		EngineStarted = bHasFuel && bEngineActive;
		if (!EngineStarted)
		{
			DriveTorque = 0.0f;
			LoadTorque = 0.0f;
			AngularVelocity = 0.0f;
			
			TransmitTorque(VehicleModuleSystem, DriveTorque, BrakingTorque);
		}
		else
		{
			FEngineSimModule::Simulate(DeltaTime, Inputs, VehicleModuleSystem);
		}
	}
}

UVehicleSimFuelEngineComponent::UVehicleSimFuelEngineComponent()
	: Super()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

TSharedPtr<FVehicleEngineRuntimeState, ESPMode::ThreadSafe> UVehicleSimFuelEngineComponent::GetEngineState() const
{
	if (!EngineState) EngineState = MakeShared<FVehicleEngineRuntimeState, ESPMode::ThreadSafe>();
	EngineState->Refresh(GetOwner(), FuelAttribute, EngineActivateTag);
	return EngineState;
}

void UVehicleSimFuelEngineComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (EngineState) EngineState->Refresh(GetOwner(), FuelAttribute, EngineActivateTag);
}

void UVehicleSimFuelEngineComponent::OnUnregister()
{
	EngineState.Reset();
	Super::OnUnregister();
}

Chaos::ISimulationModuleBase* UVehicleSimFuelEngineComponent::CreateNewCoreModule() const
{
	Chaos::FEngineSettings Settings;

	Settings.MaxTorque = Chaos::TorqueMToCm(MaxTorque);

	// 修复：MaxRPM 为零时浮点步长也为零，旧循环会无限增长曲线数组。
	constexpr int32 NumSamples = 20;
	const auto* Curve = TorqueCurve.GetRichCurveConst();
	float MinVal = 0.0f, MaxVal = 0.0f;
	Curve->GetValueRange(MinVal, MaxVal);
	for (int32 Sample = 0; Sample <= NumSamples; ++Sample)
	{
		const float RPM = static_cast<float>(MaxRPM) * Sample / NumSamples;
		Settings.TorqueCurve.AddNormalized(FMath::IsNearlyZero(MaxVal) ? 0.0f : Curve->Eval(RPM) / MaxVal);
	}

	Settings.MaxRPM = MaxRPM;
	Settings.IdleRPM = EngineIdleRPM;
	Settings.EngineBrakeEffect = EngineBrakeEffect;
	Settings.EngineInertia = EngineInertia;

	Chaos::ISimulationModuleBase* Engine = new Chaos::FFuelEngineSimModule(Settings, this);
	return Engine;
}
