// =============================================================================
// IronvalePlayerCharacter.cpp — Player character implementation
// Project Ironvale
// =============================================================================

#include "Characters/IronvalePlayerCharacter.h"
#include "Ironvale.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "InputTriggers.h"
#include "Combat/IronvaleCombatComponent.h"
#include "Combat/IronvaleStaminaComponent.h"
#include "Combat/IronvaleLockOnComponent.h"
#include "Needs/IronvaleNeedsComponent.h"
#include "Needs/IronvaleEncumbranceComponent.h"
#include "Immersion/IronvaleInteractionComponent.h"
#include "Core/IronvaleStatics.h"

// Helper: create a UInputAction with a given value type
static UInputAction* CreateInputActionHelper(UObject* Outer, const TCHAR* Name, EInputActionValueType ValueType)
{
	UInputAction* Action = NewObject<UInputAction>(Outer, Name);
	Action->ValueType = ValueType;
	return Action;
}

AIronvalePlayerCharacter::AIronvalePlayerCharacter()
{
	// Camera setup
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = FirstPersonArmLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Player-specific components
	NeedsComponent = CreateDefaultSubobject<UIronvaleNeedsComponent>(TEXT("NeedsComponent"));
	EncumbranceComponent = CreateDefaultSubobject<UIronvaleEncumbranceComponent>(TEXT("EncumbranceComponent"));
	LockOnComponent = CreateDefaultSubobject<UIronvaleLockOnComponent>(TEXT("LockOnComponent"));
	InteractionComponent = CreateDefaultSubobject<UIronvaleInteractionComponent>(TEXT("InteractionComponent"));

	// Movement defaults
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 400.0f;
		Movement->MaxWalkSpeedCrouched = 200.0f;
		Movement->bCanWalkOffLedges = true;
	}

	// =========================================================================
	// Create Enhanced Input actions and mapping context in C++ so the game
	// can run without any editor-created Input Action / Mapping Context assets.
	// =========================================================================

	MoveAction = CreateInputActionHelper(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = CreateInputActionHelper(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction = CreateInputActionHelper(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = CreateInputActionHelper(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	AttackAction = CreateInputActionHelper(this, TEXT("IA_Attack"), EInputActionValueType::Boolean);
	BlockAction = CreateInputActionHelper(this, TEXT("IA_Block"), EInputActionValueType::Boolean);
	LockOnAction = CreateInputActionHelper(this, TEXT("IA_LockOn"), EInputActionValueType::Boolean);
	InteractAction = CreateInputActionHelper(this, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	CombatDirectionAction = CreateInputActionHelper(this, TEXT("IA_CombatDirection"), EInputActionValueType::Axis2D);

	// Build the mapping context with keyboard + mouse bindings
	DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	// Move: WASD -> Axis2D via Swizzle (W/S = Y, A/D = X)
	{
		// W (+Y forward)
		FEnhancedActionKeyMapping& W = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
		UInputModifierSwizzleAxis* SwizzleW = NewObject<UInputModifierSwizzleAxis>(DefaultMappingContext);
		SwizzleW->Order = EInputAxisSwizzle::YXZ;
		W.Modifiers.Add(SwizzleW);

		// S (-Y backward)
		FEnhancedActionKeyMapping& S = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
		UInputModifierSwizzleAxis* SwizzleS = NewObject<UInputModifierSwizzleAxis>(DefaultMappingContext);
		SwizzleS->Order = EInputAxisSwizzle::YXZ;
		S.Modifiers.Add(SwizzleS);
		UInputModifierNegate* NegS = NewObject<UInputModifierNegate>(DefaultMappingContext);
		S.Modifiers.Add(NegS);

		// A (-X left)
		FEnhancedActionKeyMapping& A = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
		UInputModifierNegate* NegA = NewObject<UInputModifierNegate>(DefaultMappingContext);
		A.Modifiers.Add(NegA);

		// D (+X right)
		DefaultMappingContext->MapKey(MoveAction, EKeys::D);
	}

	// Look: Mouse XY (negate Y for correct pitch direction)
	{
		FEnhancedActionKeyMapping& MouseLook = DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D);
		UInputModifierNegate* NegLook = NewObject<UInputModifierNegate>(DefaultMappingContext);
		NegLook->bX = false;
		NegLook->bY = true;
		NegLook->bZ = false;
		MouseLook.Modifiers.Add(NegLook);
	}

	// Jump: Space
	DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);

	// Sprint: Left Shift
	DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);

	// Attack: Left Mouse Button
	DefaultMappingContext->MapKey(AttackAction, EKeys::LeftMouseButton);

	// Block: Right Mouse Button
	DefaultMappingContext->MapKey(BlockAction, EKeys::RightMouseButton);

	// Lock-on: Q
	DefaultMappingContext->MapKey(LockOnAction, EKeys::Q);

	// Interact: E
	DefaultMappingContext->MapKey(InteractAction, EKeys::E);

	// Combat direction: Mouse XY (processed separately for directional combat)
	DefaultMappingContext->MapKey(CombatDirectionAction, EKeys::Mouse2D);
}

void AIronvalePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add input mapping context
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void AIronvalePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AIronvalePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);

	// Bind movement
	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AIronvalePlayerCharacter::HandleMove);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AIronvalePlayerCharacter::HandleLook);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleJump);
	}

	// Sprint
	if (SprintAction)
	{
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleSprintStart);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AIronvalePlayerCharacter::HandleSprintStop);
	}

	// Combat
	if (AttackAction)
	{
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleAttack);
		EnhancedInput->BindAction(AttackAction, ETriggerEvent::Completed, this, &AIronvalePlayerCharacter::HandleAttackRelease);
	}
	if (BlockAction)
	{
		EnhancedInput->BindAction(BlockAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleBlockStart);
		EnhancedInput->BindAction(BlockAction, ETriggerEvent::Completed, this, &AIronvalePlayerCharacter::HandleBlockStop);
	}
	if (LockOnAction)
	{
		EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleLockOnToggle);
	}
	if (CombatDirectionAction)
	{
		EnhancedInput->BindAction(CombatDirectionAction, ETriggerEvent::Triggered, this, &AIronvalePlayerCharacter::HandleCombatDirection);
	}

	// Interaction
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AIronvalePlayerCharacter::HandleInteract);
	}
}

// =============================================================================
// INPUT HANDLERS
// =============================================================================

void AIronvalePlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AIronvalePlayerCharacter::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AIronvalePlayerCharacter::HandleJump()
{
	Jump();
}

void AIronvalePlayerCharacter::HandleSprintStart()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 600.0f;
	}
}

void AIronvalePlayerCharacter::HandleSprintStop()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 400.0f;
	}
}

void AIronvalePlayerCharacter::HandleAttack(const FInputActionValue& Value)
{
	if (!CombatComponent) return;

	// Convert current directional input to attack direction
	const EIronvaleAttackDirection Direction =
		UIronvaleStatics::InputVectorToAttackDirection(CurrentCombatDirectionInput);

	CombatComponent->StartAttack(Direction);
}

void AIronvalePlayerCharacter::HandleAttackRelease(const FInputActionValue& Value)
{
	if (!CombatComponent) return;
	CombatComponent->ReleaseAttack();
}

void AIronvalePlayerCharacter::HandleBlockStart()
{
	if (!CombatComponent) return;
	CombatComponent->StartBlock();
}

void AIronvalePlayerCharacter::HandleBlockStop()
{
	if (!CombatComponent) return;
	CombatComponent->StopBlock();
}

void AIronvalePlayerCharacter::HandleLockOnToggle()
{
	if (!LockOnComponent) return;
	LockOnComponent->ToggleLockOn();
}

void AIronvalePlayerCharacter::HandleInteract()
{
	if (!InteractionComponent) return;
	InteractionComponent->TryInteract();
}

void AIronvalePlayerCharacter::HandleCombatDirection(const FInputActionValue& Value)
{
	CurrentCombatDirectionInput = Value.Get<FVector2D>();
}

// =============================================================================
// CAMERA
// =============================================================================

void AIronvalePlayerCharacter::ToggleCameraMode()
{
	bIsFirstPerson = !bIsFirstPerson;

	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = bIsFirstPerson ? FirstPersonArmLength : ThirdPersonArmLength;
	}

	// In first person, hide the character mesh (arms-only mesh would be used instead)
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(bIsFirstPerson);
	}
}
