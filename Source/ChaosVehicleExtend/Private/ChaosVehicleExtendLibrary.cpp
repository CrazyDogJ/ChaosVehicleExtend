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
	if (Comp)
	{
		if (const auto PhysicsObject = Comp->ClusterUnionComponent->GetPhysicsObjectByName(""))
		{
			if (const auto PP = PhysicsObject->PhysicsProxy())
			{
				if (const auto ClusterUnionPP = static_cast<Chaos::FClusterUnionPhysicsProxy*>(PP))
				{
					const auto ParticleEx = ClusterUnionPP->GetParticle_External();
					ParticleEx->SetCenterOfMass(COM);

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
				if (const Chaos::FEngineOutputData* Engine = static_cast<Chaos::FEngineOutputData*>(Itr))
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
				if (const Chaos::FEngineOutputData* Engine = static_cast<Chaos::FEngineOutputData*>(Itr))
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
				if (const Chaos::FFuelEngineOutputData* Engine = static_cast<Chaos::FFuelEngineOutputData*>(Itr))
				{
					return Engine->ThrottleValue;
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

		TUniquePtr<Chaos::FSimModuleTree>& SimModuleTree = Settings.VehicleBaseComponent->VehicleSimulationPT->AccessSimComponentTree();
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
	const auto NewModules = Handle.TreeUpdatesOut.GetNewModules();
	
	if (!Settings.VehicleBaseComponent)
	{
		return;
	}

	if (const auto ComponentData = Settings.VehicleBaseComponent->ComponentToPhysicsObjects.Find(WheelComp))
	{
		const auto FoundGuid = ComponentData->Guid;
		for (const auto Itr : NewModules)
		{
			if (Itr.NewSimModule->GetGuid() == FoundGuid)
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
			Result.Add(Itr.Key.ResolveObjectPtr(), Itr.Value.Guid);
		}
	}

	return Result;
}
