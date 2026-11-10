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
	// 修复：装卸请求可能早于物理初始化，或在模块销毁后到达。
	if (!IsValid(ModuleActor) || !IsValid(ModuleActor->RootMeshComponent) || !ClusterUnionComponent)
	{
		return;
	}
	
	CurrentModules.Remove(ModuleActor);
	
	RemovingComponents.Add(ModuleActor->RootMeshComponent);

	ClusterUnionComponent->RemoveComponentFromCluster(ModuleActor->RootMeshComponent);
}

void UModularVehicleExtendComponent::AddVehicleModule(AVehicleModuleBaseActor* ModuleActor, const FVehicleModuleAttachment& AttachmentData)
{
	// 修复：装卸请求可能早于物理初始化，或在模块销毁后到达。
	if (!IsValid(ModuleActor) || !IsValid(ModuleActor->RootMeshComponent) || !ClusterUnionComponent)
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
	// 修复：复制未解析的座位成员可能为空，移动 Actor 的回调也可能修改座位数组。
	const auto Seats = VehicleSeats;
	for (const auto& Seat : Seats)
	{
		if (IsValid(Seat.Actor) && IsValid(Seat.Primitive) && Seat.Actor->GetAttachParentActor() == GetOwner())
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
	if (!WPS || !ClusterUnionComponent || !GetOwner())
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

void UModularVehicleExtendComponent::BroadcastSimComponentCreated(USceneComponent* Component, int32 TransformIndex, const Chaos::FSimTreeUpdates& Updates)
{
	// 修复：广播返回后自动令蓝图拿到的更新句柄失效。
	const auto Active = MakeShared<Chaos::FSimTreeUpdates>(Updates);
	FSimTreeUpdatesHandle Handle{Updates, Active};
	OnSimComponentCreated.Broadcast(Component, TransformIndex, Handle);
}

void UModularVehicleExtendComponent::CreateAssociatedSimComponents(USceneComponent* ParentComponent,
                                                                   USceneComponent* AttachedComponent, int ParentIndex, int TransformIndex, Chaos::FSimTreeUpdates& TreeUpdatesOut)
{
	Super::CreateAssociatedSimComponents(ParentComponent, AttachedComponent, ParentIndex, TransformIndex,
	                                     TreeUpdatesOut);

	BroadcastSimComponentCreated(AttachedComponent, TransformIndex, TreeUpdatesOut);
}

void UModularVehicleExtendComponent::CreateIndependentSimComponents(USceneComponent* RootComponent,
	USceneComponent* AttachedComponent, int ParentIndex, int TransformIndex, Chaos::FSimTreeUpdates& TreeUpdatesOut)
{
	Super::CreateIndependentSimComponents(RootComponent, AttachedComponent, ParentIndex, TransformIndex,
	                                      TreeUpdatesOut);
	
	BroadcastSimComponentCreated(AttachedComponent, TransformIndex, TreeUpdatesOut);
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
		// 修复：添加模块可同步触发更新事件，先取走当前批次。
		auto Pending = MoveTemp(CachedAddingComponents);
		for (const auto& Comp : Pending)
		{
			if (!IsValid(Comp.Key) || !ClusterUnionComponent) continue;
			if (const auto VehicleModuleActor = Cast<AVehicleModuleBaseActor>(Comp.Key->GetOwner()))
			{
				VehicleModuleActor->SetAttachmentData(Comp.Value);
			}
			ClusterUnionComponent->AddComponentToCluster(Comp.Key, {0});
		}
	}
	
	OnComponentRemovedEvent.Broadcast(InComponent);
}
