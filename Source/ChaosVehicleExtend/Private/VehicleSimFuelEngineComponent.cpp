// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleSimFuelEngineComponent.h"

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
	}

	UAbilitySystemComponent* FFuelEngineSimModule::GetAbilitySystemComponent() const
	{
		if (Owner && Owner->GetOwner())
		{
			return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner->GetOwner());
		}

		return nullptr;
	}

	float FFuelEngineSimModule::GetFuelAttributeFloat() const
	{
		if (!Owner)
		{
			return 0.0f;
		}
		
		if (const auto ASC = GetAbilitySystemComponent())
		{
			bool bFound = false;
			const auto FoundValue = ASC->GetGameplayAttributeValue(Owner->FuelAttribute, bFound);
			if (bFound)
			{
				return FoundValue;
			}
		}

		return 0.0f;
	}

	bool FFuelEngineSimModule::HasFuel() const
	{
		return GetFuelAttributeFloat() > 0.0f;
	}

	bool FFuelEngineSimModule::IsEngineActivate() const
	{
		if (!Owner)
		{
			return false;
		}
		
		if (const auto ASC = GetAbilitySystemComponent())
		{
			return ASC->HasMatchingGameplayTag(Owner->EngineActivateTag);
		}

		return false;
	}

	void FFuelEngineSimModule::Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem)
	{
		ThrottleValue = Inputs.GetControls().GetMagnitude(ThrottleControlName);
		
		EngineStarted = HasFuel() && IsEngineActivate();
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
}

Chaos::ISimulationModuleBase* UVehicleSimFuelEngineComponent::CreateNewCoreModule() const
{
	Chaos::FEngineSettings Settings;

	Settings.MaxTorque = Chaos::TorqueMToCm(MaxTorque);

	float NumSamples = 20;
	for (float X = 0; X <= MaxRPM; X += (MaxRPM / NumSamples))
	{
		float MinVal = 0.f, MaxVal = 0.f;
		TorqueCurve.GetRichCurveConst()->GetValueRange(MinVal, MaxVal);
		float Y = this->TorqueCurve.GetRichCurveConst()->Eval(X) / MaxVal;
		Settings.TorqueCurve.AddNormalized(Y);
	}

	Settings.MaxRPM = MaxRPM;
	Settings.IdleRPM = EngineIdleRPM;
	Settings.EngineBrakeEffect = EngineBrakeEffect;
	Settings.EngineInertia = EngineInertia;

	Chaos::ISimulationModuleBase* Engine = new Chaos::FFuelEngineSimModule(Settings, this);
	return Engine;
}
