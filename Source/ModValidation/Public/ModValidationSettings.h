#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ModValidationSettings.generated.h"

/**
 * One rule describing a function-call input pin that must be connected
 * (or carry a non-null default) anywhere it is used in a Blueprint graph.
 */
USTRUCT()
struct FRequiredPinRule
{
	GENERATED_BODY()

	/**
	 * Optional owning class of the called function, e.g. "FGPowerConnectionComponent".
	 * Matched with or without the U/A prefix. Leave empty to match the function on any class.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Required Pins")
	FString ClassName;

	/** Name of the called function as declared in C++, e.g. "SetPowerInfo". */
	UPROPERTY(EditAnywhere, config, Category = "Required Pins")
	FString FunctionName;

	/** Name of the input pin that must be connected, e.g. "powerInfo". */
	UPROPERTY(EditAnywhere, config, Category = "Required Pins")
	FString PinName;
};

/**
 * One rule constraining a class-valued property (a TSubclassOf / soft class) set in a
 * Blueprint's class defaults. Catches "attached the wrong kind of class" mistakes — most
 * importantly a buildable pointing mHologramClass at a hologram built for a different
 * buildable type, which crashes the game the instant you click the recipe.
 *
 * A rule can express two independent checks (use either, or both):
 *   1. Type check    -> the referenced class must derive from RequiredBaseClass.
 *   2. Pairing check -> when the referenced class derives from WhenValueDerivesFrom, the
 *                       owning class must derive from RequiredOwnerBaseClass.
 *
 * All class names are matched with or without the U/A prefix, anywhere up the parent chain.
 */
USTRUCT()
struct FClassPropertyRule
{
	GENERATED_BODY()

	/** Only assets whose generated class derives from this are checked. Empty = any class. e.g. "FGBuildable". */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	FString OwnerClassName;

	/** The class-valued property to inspect on the CDO. e.g. "mHologramClass". */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	FString PropertyName;

	/** If true, the property must reference a non-null class: an unset/None or dangling (renamed-away) reference fails. e.g. a descriptor's mBuildableClass. */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	bool bMustBeSet = false;

	/** Type check: the referenced class must derive from this, or the asset fails. Empty = skip. e.g. "FGHologram". */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	FString RequiredBaseClass;

	/** Pairing trigger: the pairing check applies only when the referenced class derives from this. Empty = always. e.g. "FGRailroadTrackHologram". */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	FString WhenValueDerivesFrom;

	/** Pairing requirement: when triggered, the owning class must derive from this, or the asset fails. Empty = skip. e.g. "FGBuildableRailroadTrack". */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	FString RequiredOwnerBaseClass;
};

/**
 * Project Settings -> Plugins -> Mod Validation.
 * Edit the list to add more "this pin must be wired up" rules; saved to Config/DefaultEditor.ini.
 */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Mod Validation"))
class UModValidationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UModValidationSettings();

	/**
	 * Function-call input pins that must be connected. A Blueprint that leaves one of these
	 * unconnected (and without a non-null default) fails validation before it can be packaged.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Required Pins")
	TArray<FRequiredPinRule> RequiredPins;

	/**
	 * Class-valued property constraints. Fails a Blueprint whose class defaults attach a class of
	 * the wrong type — e.g. a non-railroad buildable using a railroad-track hologram.
	 */
	UPROPERTY(EditAnywhere, config, Category = "Class Properties")
	TArray<FClassPropertyRule> ClassPropertyRules;

	virtual FName GetCategoryName() const override { return FName("Plugins"); }
};
