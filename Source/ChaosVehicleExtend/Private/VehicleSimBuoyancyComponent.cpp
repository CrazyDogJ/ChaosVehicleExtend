// Fill out your copyright notice in the Description page of Project Settings.

#include "VehicleSimBuoyancyComponent.h"

#include "VehicleSimSteerThrusterComponent.h"
#include "WaterBodyActor.h"
#include "WaterRuntimeSettings.h"
#include "WaterSplineComponent.h"
#include "SimModule/SimModuleTree.h"

float GetWaterSplineKeyFastSim(FVector Location, const UWaterBodyComponent* WaterBodyComponent, TMap<const UWaterBodyComponent*, float>& OutSegmentMap)/*const*/
{
	if (!OutSegmentMap.Contains(WaterBodyComponent))
	{
		OutSegmentMap.Add(WaterBodyComponent, -1);
	}

	const UWaterSplineComponent* WaterSpline = WaterBodyComponent->GetWaterSpline();
	const FVector LocalLocation = WaterBodyComponent->GetComponentTransform().InverseTransformPosition(Location);
	const FInterpCurveVector& InterpCurve = WaterSpline->GetSplinePointsPosition();
	float& Segment = OutSegmentMap[WaterBodyComponent];

	if (Segment == -1)
	{
		float DummyDistance;
		return InterpCurve.FindNearest(LocalLocation, DummyDistance, Segment);
	}

	//We have the cached segment, so search for the best point as in FInterpCurve<T>::FindNearest
	//but only in the current segment and the two immediate neighbors

	//River splines aren't looped, so we don't have to handle that case
	const int32 NumPoints = InterpCurve.Points.Num();
	const int32 LastSegmentIdx = FMath::Max(0, NumPoints - 2);
	if (NumPoints > 1)
	{
		float BestDistanceSq = BIG_NUMBER;
		float BestResult = BIG_NUMBER;
		float BestSegment = Segment;
		for (int32 i = Segment - 1; i <= Segment + 1; ++i)
		{
			const int32 SegmentIdx = FMath::Clamp(i, 0, LastSegmentIdx);
			float LocalDistanceSq;
			float LocalResult = InterpCurve.FindNearestOnSegment(LocalLocation, SegmentIdx, LocalDistanceSq);
			if (LocalDistanceSq < BestDistanceSq)
			{
				BestDistanceSq = LocalDistanceSq;
				BestResult = LocalResult;
				BestSegment = SegmentIdx;
			}
		}

		if (FMath::IsNearlyEqual(BestResult, Segment - 1) || FMath::IsNearlyEqual(BestResult, Segment + 1))
		{
			//We're at either end of the search - it's possible we skipped a segment so just do a full lookup in this case
			float DummyDistance;
			return InterpCurve.FindNearest(LocalLocation, DummyDistance, Segment);
		}

		Segment = BestSegment;
		return BestResult;
	}

	if (NumPoints == 1)
	{
		Segment = 0;
		return InterpCurve.Points[0].InVal;
	}

	return 0.0f;
}

float GetWaterHeight(TArray<UWaterBodyComponent*> CurrentWaterBodyComponents, const FVector& Position,
	const TMap<const UWaterBodyComponent*, float>& SplineKeyMap, float DefaultHeight,
	UWaterBodyComponent*& OutWaterBodyComponent, float& OutWaterDepth, FVector& OutWaterPlaneLocation,
	FVector& OutWaterPlaneNormal, FVector& OutWaterSurfacePosition, FVector& OutWaterVelocity, int32& OutWaterBodyIdx,
	bool bShouldIncludeWaves)
{
	float WaterHeight = DefaultHeight;
	OutWaterBodyComponent = nullptr;
	OutWaterDepth = 0.f;
	OutWaterPlaneLocation = FVector::ZeroVector;
	OutWaterPlaneNormal = FVector::UpVector;

	float MaxImmersionDepth = -1.f;
	for (UWaterBodyComponent* CurrentWaterBodyComponent : CurrentWaterBodyComponents)
	{
		if (CurrentWaterBodyComponent)
		{
			const float SplineInputKey = SplineKeyMap.FindRef(CurrentWaterBodyComponent);

			EWaterBodyQueryFlags QueryFlags =
				EWaterBodyQueryFlags::ComputeLocation
				| EWaterBodyQueryFlags::ComputeNormal
				| EWaterBodyQueryFlags::ComputeImmersionDepth
				| EWaterBodyQueryFlags::ComputeVelocity;

			if (bShouldIncludeWaves)
			{
				QueryFlags |= EWaterBodyQueryFlags::IncludeWaves;
			}

			FWaterBodyQueryResult QueryResult = CurrentWaterBodyComponent->QueryWaterInfoClosestToWorldLocation(Position, QueryFlags, SplineInputKey);
			if (QueryResult.IsInWater() && QueryResult.GetImmersionDepth() > MaxImmersionDepth)
			{
				check(!QueryResult.IsInExclusionVolume());
				WaterHeight = Position.Z + QueryResult.GetImmersionDepth();
				OutWaterBodyComponent = CurrentWaterBodyComponent;
				if (EnumHasAnyFlags(QueryResult.GetQueryFlags(), EWaterBodyQueryFlags::ComputeDepth))
				{
					OutWaterDepth = QueryResult.GetWaterSurfaceDepth();
				}
				OutWaterPlaneLocation = QueryResult.GetWaterPlaneLocation();
				OutWaterPlaneNormal = QueryResult.GetWaterPlaneNormal();
				OutWaterSurfacePosition = QueryResult.GetWaterSurfaceLocation();
				OutWaterVelocity = QueryResult.GetVelocity();
				OutWaterBodyIdx = CurrentWaterBodyComponent ? CurrentWaterBodyComponent->GetWaterBodyIndex() : 0;
				MaxImmersionDepth = QueryResult.GetImmersionDepth();
			}
		}
	}
	return WaterHeight;
}

namespace Chaos
{
	FBuoyancySimModule::FBuoyancySimModule(const FBuoyancySettings& Settings, UWorld* InWorld)
		: TSimModuleSettings<FBuoyancySettings>(Settings)
	{
		Pontoons = Settings.Pontoons;
		World = InWorld;
	}

	void FBuoyancySimModule::UpdateCurrentWaterBodies(const FAllInputs& Inputs)
	{
		CurrentWaterBodies.Empty();
		const ECollisionChannel WaterChannel = GetDefault<UWaterRuntimeSettings>()->CollisionChannelForWaterTraces;
		
		if (!World)
		{
			return;
		}
		
		for (auto& Itr : Pontoons)
		{
			Itr.CenterLocation = Inputs.VehicleWorldTransform.TransformPosition(Itr.RelativeLocation);
			
			TArray<FHitResult> HitResultsOut;
			Chaos::Private::FGenericPhysicsInterface_Internal::SpherecastMulti(World
									, Itr.Radius
									, HitResultsOut
									, Itr.CenterLocation
									, Itr.CenterLocation
									, WaterChannel
									, FCollisionQueryParams::DefaultQueryParam
									, FCollisionResponseParams::DefaultResponseParam);

			for (const auto Hit : HitResultsOut)
			{
				if (!Hit.GetActor())
				{
					continue;
				}
				
				if (const auto WaterBody = Hit.GetActor()->GetComponentByClass<UWaterBodyComponent>())
				{
					// Update Pontoon Data.
					if (WaterBody && WaterBody->GetWaterBodyType() == EWaterBodyType::River)
					{
						float SplineInputKey;
						SplineInputKey = GetWaterSplineKeyFastSim(Itr.CenterLocation, WaterBody, Itr.SplineSegments);
						Itr.SplineInputKeys.Add(WaterBody, SplineInputKey);
					}
					
					CurrentWaterBodies.AddUnique(WaterBody);
				}
			}
		}
	}

	void FBuoyancySimModule::UpdatePontoons(const FAllInputs& Inputs, const FSimModuleTree& VehicleModuleSystem)
	{
		for (auto& Pontoon : Pontoons)
		{
			const FVector PontoonBottom = Pontoon.CenterLocation - FVector(0, 0, Pontoon.Radius);
			UWaterBodyComponent* TempWaterBodyComponent = Pontoon.CurrentWaterBodyComponent;
			/*Pass in large negative default value so we don't accidentally assume we're in water when we're not.*/
			Pontoon.WaterHeight = GetWaterHeight(CurrentWaterBodies, PontoonBottom - FVector::UpVector * 100.f,
				Pontoon.SplineInputKeys, -100000.f, TempWaterBodyComponent, Pontoon.WaterDepth,
				Pontoon.WaterPlaneLocation, Pontoon.WaterPlaneNormal, Pontoon.WaterSurfacePosition,
				Pontoon.WaterVelocity, Pontoon.WaterBodyIndex, false);
		
			Pontoon.CurrentWaterBodyComponent = TempWaterBodyComponent;

			const float ImmersionDepth = Pontoon.WaterHeight - PontoonBottom.Z;
			/*check if the pontoon is currently in water*/
			if (ImmersionDepth >= 0.f)
			{
				Pontoon.bIsInWater = true;
				Pontoon.ImmersionDepth = ImmersionDepth;
			}
			else
			{
				Pontoon.bIsInWater = false;
				Pontoon.ImmersionDepth = 0.f;
			}
			
			ComputeBuoyancy(Pontoon, VehicleModuleSystem.GetVehicleState().ForwardSpeedKmh);
			auto LocalForce = Inputs.VehicleWorldTransform.InverseTransformVector(Pontoon.LocalForce);
				
			AddLocalForceAtPosition(LocalForce, Pontoon.RelativeLocation, true, false, false, FColor::Magenta);
		}
	}

	void FBuoyancySimModule::ComputeBuoyancy(FSphericalPontoon& Pontoon, float ForwardSpeedKmh) const
	{
		auto ComputeBuoyantForce = [&](FVector CenterLocation, float Radius, float InBuoyancyCoefficient, float CurrentWaterLevel) -> float
		{
			const float Bottom = CenterLocation.Z - Radius;
			const float SubDiff = FMath::Clamp(CurrentWaterLevel - Bottom, 0.f, 2.f * Radius);
	
			// The following was obtained by integrating the volume of a sphere
			// over a linear section of SubmersionDiff length.
			static const float Pi = (float)PI;
			const float SubDiffSq = SubDiff * SubDiff;
			const float SubVolume = (Pi / 3.f) * SubDiffSq * ((3.f * Radius) - SubDiff);
	
			// The buoyant force scales with submersed volume
			return SubVolume * (InBuoyancyCoefficient);
		};
	
		const float MinVelocity = Setup().BuoyancyRampMinVelocity;
		const float MaxVelocity = Setup().BuoyancyRampMaxVelocity;
		const float RampFactor = FMath::Clamp((ForwardSpeedKmh - MinVelocity) / (MaxVelocity - MinVelocity), 0.f, 1.f);
		const float BuoyancyRamp = RampFactor * (Setup().BuoyancyRampMax - 1);
		float BuoyancyCoefficientWithRamp = Setup().BuoyancyCoefficient * (1 + BuoyancyRamp);
	
		const float BuoyantForce = FMath::Clamp(ComputeBuoyantForce(Pontoon.CenterLocation, Pontoon.Radius, BuoyancyCoefficientWithRamp, Pontoon.WaterHeight), 0.f, Setup().MaxBuoyantForce);
		Pontoon.LocalForce = FVector::UpVector * BuoyantForce * Pontoon.PontoonCoefficient;
	}

	void FBuoyancySimModule::Simulate(float DeltaTime, const FAllInputs& Inputs, FSimModuleTree& VehicleModuleSystem)
	{
		UpdateCurrentWaterBodies(Inputs);
		UpdatePontoons(Inputs, VehicleModuleSystem);
	}
}

UVehicleSimBuoyancyComponent::UVehicleSimBuoyancyComponent()
{
}

Chaos::ISimulationModuleBase* UVehicleSimBuoyancyComponent::CreateNewCoreModule() const
{
	Chaos::FBuoyancySettings Settings;
	Settings.BuoyancyCoefficient = BuoyancyCoefficient;
	Settings.MaxBuoyantForce = MaxBuoyantForce;
	Settings.BuoyancyRampMax = BuoyancyRampMax;
	Settings.BuoyancyRampMaxVelocity = BuoyancyRampMaxVelocity;
	Settings.BuoyancyRampMinVelocity = BuoyancyRampMinVelocity;
	Settings.Pontoons = Pontoons;
	
	Chaos::ISimulationModuleBase* Thruster = new Chaos::FBuoyancySimModule(Settings, GetWorld());
	
	return Thruster;
}

