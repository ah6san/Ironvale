// =============================================================================
// IronvaleHealthComponent.cpp
// Project Ironvale
// =============================================================================

#include "Combat/IronvaleHealthComponent.h"
#include "Ironvale.h"

UIronvaleHealthComponent::UIronvaleHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UIronvaleHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
}

float UIronvaleHealthComponent::ApplyDamage(float Amount, AActor* DamageCauser)
{
	if (Amount <= 0.0f || CurrentHealth <= 0.0f) return 0.0f;

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Amount);
	const float ActualDamage = OldHealth - CurrentHealth;

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, -ActualDamage);

	if (CurrentHealth <= 0.0f)
	{
		UE_LOG(LogIronvale, Log, TEXT("%s health depleted by %s"),
			*GetOwner()->GetName(),
			DamageCauser ? *DamageCauser->GetName() : TEXT("unknown"));
		OnHealthDepleted.Broadcast(DamageCauser);
	}

	return ActualDamage;
}

float UIronvaleHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || CurrentHealth <= 0.0f) return 0.0f;

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
	const float ActualHeal = CurrentHealth - OldHealth;

	if (ActualHeal > 0.0f)
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, ActualHeal);
	}

	return ActualHeal;
}

void UIronvaleHealthComponent::SetHealth(float NewHealth)
{
	CurrentHealth = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
}

float UIronvaleHealthComponent::GetHealthPercent() const
{
	return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UIronvaleHealthComponent::SetMaxHealth(float NewMax)
{
	MaxHealth = FMath::Max(1.0f, NewMax);
	CurrentHealth = FMath::Min(CurrentHealth, MaxHealth);
}
