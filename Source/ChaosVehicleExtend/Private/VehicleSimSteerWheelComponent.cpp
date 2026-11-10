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

		// 修复：输入容器尚未建立时以零转向模拟。
		SteeringInput = Inputs.ControlInputs ? Inputs.ControlInputs->GetMagnitude(SteeringControlName) : 0.0f;
	}

	void FSteerWheelSimModule::Animate()
	{
		ISimulationModuleBase::Animate();

		const auto Angle = Setup().Angle * SteeringInput;

		// 修复：未使用的旋转轴也参与四元数运算，必须初始化。
		FRotator Rotation = FRotator::ZeroRotator;
	
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
