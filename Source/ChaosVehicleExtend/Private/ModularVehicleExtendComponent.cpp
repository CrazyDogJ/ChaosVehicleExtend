// Fill out your copyright notice in the Description page of Project Settings.

#include "ModularVehicleExtendComponent.h"

#include "Net/UnrealNetwork.h"
#include "WorldPartition/WorldPartitionLevelStreamingPolicy.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

UModularVehicleExtendComponent::UModularVehicleExtendComponent()
{
}

void UModularVehicleExtendComponent::RemoveVehicleModule(AVehicleModuleBaseActor* ModuleActor)
{
	if (!ModuleActor)
	{
		return;
	}
	
	CurrentModules.Remove(ModuleActor);
	
	RemovingComponents.Add(ModuleActor->RootMeshComponent);

	ClusterUnionComponent->RemoveComponentFromCluster(ModuleActor->RootMeshComponent);
}

void UModularVehicleExtendComponent::AddVehicleModule(AVehicleModuleBaseActor* ModuleActor, const FVehicleModuleAttachment& AttachmentData)
{
	if (!ModuleActor)
	{
		return;
	}
	
	CurrentModules.Add(ModuleActor);
	
	if (IsRemovingComponents())
	{
		CachedAddingComponents.Add(ModuleActor->RootMeshComponent, AttachmentData);
	}
	else
	{
		ModuleActor->SetAttachmentData(AttachmentData);
		ClusterUnionComponent->AddComponentToCluster(ModuleActor->RootMeshComponent, {0});
	}
}

void UModularVehicleExtendComponent::AddAttachment(const FVehicleSeat& SeatSetting, const bool bAttach)
{
	if (SeatSetting.Actor && SeatSetting.Primitive)
	{
		if (bAttach)
		{
			SeatSetting.Actor->AttachToComponent(SeatSetting.Primitive, FAttachmentTransformRules::SnapToTargetIncludingScale, SeatSetting.Socket);
		}
		
		VehicleSeats.Add(SeatSetting);
	}
}

void UModularVehicleExtendComponent::RemoveAttachment(AActor* Actor, const bool bAttach)
{
	if (Actor)
	{
		const auto Index = VehicleSeats.IndexOfByPredicate([Actor](const FVehicleSeat& Seat)
		{
			return Seat.Actor == Actor;
		});

		if (VehicleSeats.IsValidIndex(Index))
		{
			if (bAttach)
			{
				VehicleSeats[Index].Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			}
			
			VehicleSeats.RemoveAt(Index);
		}
	}
}

void UModularVehicleExtendComponent::UpdateAttachment(const float DeltaSecond)
{
	for (const auto Seat : VehicleSeats)
	{
		if (Seat.Actor->GetAttachParentActor() == GetOwner() && Seat.Primitive)
		{
			FVector CompVelocity = Seat.Primitive->GetComponentVelocity();
			const auto SocketTransform = Seat.Primitive->GetSocketTransform(Seat.Socket);
			const auto TransformedLocation = SocketTransform.TransformPosition(Seat.Offset.GetLocation());
			const auto TransformedRotator = SocketTransform.TransformRotation(Seat.Offset.GetRotation());
			Seat.Actor->SetActorTransform(FTransform(TransformedRotator, TransformedLocation + CompVelocity * DeltaSecond));
		}
	}
}

void UModularVehicleExtendComponent::UpdateStreamingLevel() const
{
	const auto WPS = GetWorld()->GetSubsystem<UWorldPartitionSubsystem>();
	if (!WPS)
	{
		return;
	}
	
	const auto Location = GetOwner()->GetActorLocation();
	auto Query = FWorldPartitionStreamingQuerySource(Location);
	Query.Radius = 1.0f;
	Query.bUseGridLoadingRange = false;
	const auto IsStreamingCompleted = WPS->IsStreamingCompleted(EWorldPartitionRuntimeCellState::Activated, {Query}, true);

	ClusterUnionComponent->SetSimulatePhysics(IsStreamingCompleted);
	if (VehicleSimulationPT && VehicleSimulationPT->SimModuleTree)
	{
		VehicleSimulationPT->SimModuleTree->SetSimulationEnabled(IsStreamingCompleted);
	}
}

void UModularVehicleExtendComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, CurrentModules);
	DOREPLIFETIME(ThisClass, VehicleSeats);
}

void UModularVehicleExtendComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateAttachment(DeltaTime);
	UpdateStreamingLevel();

	// Reset inputs.
	if (!IsLocallyControlled())
	{
		if (const auto PT = VehicleSimulationPT.Get())
		{
			PT->VehicleInputs.Container = FModuleInputContainer();
		}
	}
}

void UModularVehicleExtendComponent::CreateAssociatedSimComponents(USceneComponent* ParentComponent,
                                                                   USceneComponent* AttachedComponent, int ParentIndex, int TransformIndex, Chaos::FSimTreeUpdates& TreeUpdatesOut)
{
	Super::CreateAssociatedSimComponents(ParentComponent, AttachedComponent, ParentIndex, TransformIndex,
	                                     TreeUpdatesOut);

	OnSimComponentCreated.Broadcast(AttachedComponent, TransformIndex, FSimTreeUpdatesHandle(TreeUpdatesOut));
}

void UModularVehicleExtendComponent::CreateIndependentSimComponents(USceneComponent* RootComponent,
	USceneComponent* AttachedComponent, int ParentIndex, int TransformIndex, Chaos::FSimTreeUpdates& TreeUpdatesOut)
{
	Super::CreateIndependentSimComponents(RootComponent, AttachedComponent, ParentIndex, TransformIndex,
	                                      TreeUpdatesOut);
	
	OnSimComponentCreated.Broadcast(AttachedComponent, TransformIndex, FSimTreeUpdatesHandle(TreeUpdatesOut));
}

void UModularVehicleExtendComponent::AddComponentToSimulationImpl(UPrimitiveComponent* Component,
	const TArray<FClusterUnionBoneData>& BonesData, const TArray<FClusterUnionBoneData>& RemovedBoneIDs, bool bIsNew)
{
	Super::AddComponentToSimulationImpl(Component, BonesData, RemovedBoneIDs, bIsNew);

	TSet<int32> BoneIDsSet;
	Algo::Transform(BonesData, BoneIDsSet, &FClusterUnionBoneData::ID);
	OnComponentAddedEvent.Broadcast(Component, BoneIDsSet, bIsNew);
}

void UModularVehicleExtendComponent::RemoveComponentFromSimulationImpl(UPrimitiveComponent* InComponent,
	const TArray<FClusterUnionBoneData>& RemovedBonesData)
{
	Super::RemoveComponentFromSimulationImpl(InComponent, RemovedBonesData);

	RemovingComponents.Remove(InComponent);
	// Destroy module actor.
	if (InComponent)
	{
		if (const auto VehicleModuleActor = Cast<AVehicleModuleBaseActor>(InComponent->GetOwner()))
		{
			VehicleModuleActor->Destroy(true);
		}
	}
	
	if (!IsRemovingComponents() && CachedAddingComponents.Num() > 0)
	{
		for (const auto Comp : CachedAddingComponents)
		{
			if (const auto VehicleModuleActor = Cast<AVehicleModuleBaseActor>(Comp.Key->GetOwner()))
			{
				VehicleModuleActor->SetAttachmentData(Comp.Value);
			}
			ClusterUnionComponent->AddComponentToCluster(Comp.Key, {0});
		}
		CachedAddingComponents.Empty();
	}
	
	OnComponentRemovedEvent.Broadcast(InComponent);
}
