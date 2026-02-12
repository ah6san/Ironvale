// =============================================================================
// IronvaleHealthComponent.h — Health pool with damage/healing and death events
// Project Ironvale
// =============================================================================

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/IronvaleTypes.h"
#include "IronvaleHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChangedSignature,
	float, CurrentHealth, float, MaxHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDepletedSignature,
	AActor*, DamageCauser);

UCLASS(ClassGroup = (Ironvale), meta = (BlueprintSpawnableComponent))
class IRONVALE_API UIronvaleHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIronvaleHealthComponent();

	virtual void BeginPlay() override;

	/** Apply damage. Returns actual damage dealt after clamping. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Health")
	float ApplyDamage(float Amount, AActor* DamageCauser = nullptr);

	/** Apply healing. Returns actual amount healed. */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Health")
	float Heal(float Amount);

	/** Set health directly (for loading saves) */
	UFUNCTION(BlueprintCallable, Category = "Ironvale|Health")
	void SetHealth(float NewHealth);

	UFUNCTION(BlueprintPure, Category = "Ironvale|Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Ironvale|Health")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Ironvale|Health")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Ironvale|Health")
	void SetMaxHealth(float NewMax);

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Health")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Ironvale|Health")
	FOnHealthDepletedSignature OnHealthDepleted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ironvale|Health", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ironvale|Health")
	float CurrentHealth = 100.0f;
};
