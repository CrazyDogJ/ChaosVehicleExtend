// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleSimSteerWheelComponent.h"

namespace Chaos
{
	FSteerWheelSimModule::FSteerWheelSimModule(const FSteerWheelSettings& Settings)
	: TSimModuleSettings<FSteerWheelSettings>(Settings), SteeringInput(0)
	{
	}

	void FSteerWheelSimModule::Simulate(float DeltaTime, const FAllInputs& Inputs,
											   FSimModuleTree& VehicleModuleSystem)
	{
		ISimulationModuleBase::Simulate(DeltaTime, Inputs, VehicleModuleSystem);

		SteeringInput = Inputs.ControlInputs->GetMagnitude(SteeringControlName);
	}

	void FSteerWheelSimModule::Animate()
	{
		ISimulationModuleBase::Animate();

		const auto Angle = Setup().Angle * SteeringInput;

		FRotator Rotation;
	
		switch (Setup().Axis)
		{
		case EAxis::None:
			break;
		case EAxis::X: Rotation.Roll = Angle;
			break;
		case EAxis::Y: Rotation.Pitch = Angle;
			break;
		case EAxis::Z: Rotation.Yaw = Angle;
			break;
		}
	
		AnimationData.AnimFlags = EAnimationFlags::AnimateRotation;
		AnimationData.CombinedRotation = Rotation.Quaternion();
	}
}

UVehicleSimSteerWheelComponent::UVehicleSimSteerWheelComponent()
{
}

Chaos::ISimulationModuleBase* UVehicleSimSteerWheelComponent::CreateNewCoreModule() const
{
	Chaos::FSteerWheelSettings Settings;
	Settings.Axis = Axis;
	Settings.Angle = Angle;
	
	Chaos::ISimulationModuleBase* SteerWheel = new Chaos::FSteerWheelSimModule(Settings);
	SteerWheel->SetAnimationEnabled(true);

	return SteerWheel;
}
