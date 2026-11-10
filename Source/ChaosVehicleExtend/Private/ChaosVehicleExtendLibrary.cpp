// Fill out your copyright notice in the Description page of Project Settings.


#include "ChaosVehicleExtendLibrary.h"

#include "VehicleSimFuelEngineComponent.h"
#include "ChaosModularVehicle/ModularVehicleBaseComponent.h"
#include "PhysicsEngine/ClusterUnionComponent.h"
#include "Runtime/Experimental/Chaos/Private/Chaos/PhysicsObjectInternal.h"

float UChaosVehicleExtendLibrary::GetModularVehicleSpeed(const UModularVehicleBaseComponent* Comp)
{
	if (Comp && Comp->VehicleSimulationPT && Comp->VehicleSimulationPT->SimModuleTree)
	{
		return Comp->VehicleSimulationPT->SimModuleTree->GetVehicleState().ForwardSpeedKmh;
	}

	return -1.0f;
}

void UChaosVehicleExtendLibrary::SetModularVehicleCOM(const UModularVehicleBaseComponent* Comp, const FVector& COM)
{
	// 修复：物理组件和外部粒子在注册、拆卸期间都可能不存在。
	if (Comp && Comp->ClusterUnionComponent)
	{
		if (const auto PhysicsObject = Comp->ClusterUnionComponent->GetPhysicsObjectByName(""))
		{
			if (const auto PP = PhysicsObject->PhysicsProxy())
			{
				if (const auto ClusterUnionPP = static_cast<Chaos::FClusterUnionPhysicsProxy*>(PP))
				{
					const auto ParticleEx = ClusterUnionPP->GetParticle_External();
					if (ParticleEx) ParticleEx->SetCenterOfMass(COM);

					if (const auto Solver = ClusterUnionPP->GetSolver<Chaos::FPhysicsSolverBase>())
					{
						Solver->EnqueueCommandImmediate(
						[ClusterUnionPP, COM]()
							{
								if (const auto Internal = ClusterUnionPP->GetParticle_Internal())
								{
									Internal->SetCenterOfMass(COM);
								}
							}
						);
					}
				}
			}
		}
	}
}

float UChaosVehicleExtendLibrary::GetModularVehicleRPM(const UModularVehicleBaseComponent* Comp)
{
	if (Comp)
	{
		if (const auto OutPut = Comp->PVehicleOutput.Get())
		{
			for (const auto Itr : OutPut->SimTreeOutputData)
			{
				// if there is more than one engine then the last one will inform us of the engine RPM
				// 修复：同一输出数组还包含车轮等模块，转换前检查 Chaos 类型。
				if (const auto* Engine = Itr ? Itr->Cast<Chaos::FEngineOutputData>() : nullptr)
				{
					return Engine->RPM;
				}
			}
		}
	}
	
	return 0.0f;
}

float UChaosVehicleExtendLibrary::GetModularVehicleTorque(const UModularVehicleBaseComponent* Comp)
{
	if (Comp)
	{
		if (const auto OutPut = Comp->PVehicleOutput.Get())
		{
			for (const auto Itr : OutPut->SimTreeOutputData)
			{
				// if there is more than one engine then the last one will inform us of the engine RPM
				// 修复：同一输出数组还包含车轮等模块，转换前检查 Chaos 类型。
				if (const auto* Engine = Itr ? Itr->Cast<Chaos::FEngineOutputData>() : nullptr)
				{
					return Engine->Torque;
				}
			}
		}
	}
	
	return 0.0f;
}

float UChaosVehicleExtendLibrary::GetModularVehicleThrottle(const UModularVehicleBaseComponent* Comp)
{
	if (Comp)
	{
		if (const auto OutPut = Comp->PVehicleOutput.Get())
		{
			for (const auto Itr : OutPut->SimTreeOutputData)
			{
				// if there is more than one engine then the last one will inform us of the engine RPM
				// 修复：只有带燃料输出标记的引擎才包含油门字段。
				if (Itr && Itr->IsSimType(Chaos::FFuelEngineOutputData::FuelOutputType()))
				{
					return static_cast<const Chaos::FFuelEngineOutputData*>(Itr)->ThrottleValue;
				}
			}
		}
	}
	
	return 0.0f;
}

void UChaosVehicleExtendLibrary::SetWheelRepSettings(USceneComponent* WheelComp, FWheelRepSettings Settings)
{
	if (!Settings.VehicleBaseComponent)
	{
		return;
	}

	if (const auto ComponentData = Settings.VehicleBaseComponent->ComponentToPhysicsObjects.Find(WheelComp))
	{
		const auto FoundGuid = ComponentData->Guid;

		// 修复：物理状态在车辆拆卸期间可能先于组件映射释放。
		if (!Settings.VehicleBaseComponent->VehicleSimulationPT) return;
		TUniquePtr<Chaos::FSimModuleTree>& SimModuleTree = Settings.VehicleBaseComponent->VehicleSimulationPT->AccessSimComponentTree();
		if (!SimModuleTree) return;
		for (int I = 0; I < SimModuleTree->GetNumNodes(); I++)
		{
			if (auto SimModule = SimModuleTree->AccessSimModule(I))
			{
				if (SimModule->GetGuid() == FoundGuid)
				{
					if (auto WheelSimModule = SimModule->Cast<Chaos::FWheelSimModule>())
					{
						auto& Setup = WheelSimModule->AccessSetup();
						Setup.SteeringEnabled = Settings.bSteeringEnabled;
						Setup.HandbrakeEnabled = Settings.bHandbrakeEnabled;
						Setup.TractionControlEnabled = Settings.bTractionControlEnabled;
						Setup.MaxSteeringAngle = Settings.MaxSteeringAngle;
						Setup.ReverseDirection = Settings.ReverseDirection;
					}
				}
			}
		}
	}
}

void UChaosVehicleExtendLibrary::SetWheelRepSettingsUpdate(USceneComponent* WheelComp, FWheelRepSettings Settings,
	FSimTreeUpdatesHandle Handle)
{
	// 修复：延迟执行的蓝图不能再解引用创建事件中的模拟模块。
	const auto Active = Handle.ActiveUpdates.Pin();
	if (!Active) return;
	const auto NewModules = Active->GetNewModules();
	
	if (!Settings.VehicleBaseComponent)
	{
		return;
	}

	if (const auto ComponentData = Settings.VehicleBaseComponent->ComponentToPhysicsObjects.Find(WheelComp))
	{
		const auto FoundGuid = ComponentData->Guid;
		for (const auto Itr : NewModules)
		{
			if (Itr.NewSimModule && Itr.NewSimModule->GetGuid() == FoundGuid)
			{
				if (const auto WheelSimModule = Itr.NewSimModule->Cast<Chaos::FWheelSimModule>())
				{
					auto& Setup = WheelSimModule->AccessSetup();
					Setup.SteeringEnabled = Settings.bSteeringEnabled;
					Setup.HandbrakeEnabled = Settings.bHandbrakeEnabled;
					Setup.TractionControlEnabled = Settings.bTractionControlEnabled;
					Setup.MaxSteeringAngle = Settings.MaxSteeringAngle;
					Setup.ReverseDirection = Settings.ReverseDirection;
				}
			}
		}
	}
}

TMap<USceneComponent*, int> UChaosVehicleExtendLibrary::GetComponentToPhysicsObjects(
	const UModularVehicleBaseComponent* Comp)
{
	TMap<USceneComponent*, int> Result;
	if (Comp)
	{
		for (const auto Itr : Comp->ComponentToPhysicsObjects)
		{
			Result.Add(Itr.Key, Itr.Value.Guid);
		}
	}

	return Result;
}
