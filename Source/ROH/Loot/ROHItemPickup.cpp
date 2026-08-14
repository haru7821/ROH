#include "Loot/ROHItemPickup.h"
#include "Items/ROHItemDatabase.h"
#include "Items/ROHInventoryComponent.h"
#include "Character/ROHPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AROHItemPickup::AROHItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(100.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(PickupSphere);

	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(PickupSphere);
	NameLabel->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	NameLabel->SetHorizontalAlignment(EHTA_Center);
	NameLabel->SetWorldSize(28.f);

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AROHItemPickup::OnSphereOverlap);
}

void AROHItemPickup::InitAsItem(const FROHItemInstance& InItem)
{
	Item = InItem;
	GoldAmount = 0;
	RefreshLabel();
}

void AROHItemPickup::InitAsGold(int32 InGoldAmount)
{
	Item = FROHItemInstance();
	GoldAmount = InGoldAmount;
	RefreshLabel();
}

void AROHItemPickup::BeginPlay()
{
	Super::BeginPlay();

	// 이름표가 항상 카메라를 향하도록 쿼터뷰 각도로 고정
	NameLabel->SetWorldRotation(FRotator(35.f, 180.f, 0.f));
	RefreshLabel();
}

void AROHItemPickup::RefreshLabel()
{
	if (!NameLabel)
	{
		return;
	}

	if (GoldAmount > 0)
	{
		NameLabel->SetText(FText::Format(NSLOCTEXT("ROH", "GoldLabel", "골드 {0}"), GoldAmount));
		NameLabel->SetTextRenderColor(FColor(255, 215, 0));
		return;
	}

	if (Item.IsValid())
	{
		const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
		const UROHItemDatabase* Database = GameInstance ? GameInstance->GetSubsystem<UROHItemDatabase>() : nullptr;
		if (Database)
		{
			NameLabel->SetText(Database->GetItemDisplayName(Item));
		}
		NameLabel->SetTextRenderColor(UROHItemDatabase::GetQualityColor(Item.Quality));
	}
}

void AROHItemPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AROHPlayerCharacter* Player = Cast<AROHPlayerCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	UROHInventoryComponent* Inventory = Player->FindComponentByClass<UROHInventoryComponent>();
	if (!Inventory)
	{
		return;
	}

	if (GoldAmount > 0)
	{
		Inventory->AddGold(GoldAmount);
		Destroy();
	}
	else if (Item.IsValid() && Inventory->AddItem(Item))
	{
		Destroy();
	}
	// 인벤토리가 가득 차면 바닥에 남는다
}
