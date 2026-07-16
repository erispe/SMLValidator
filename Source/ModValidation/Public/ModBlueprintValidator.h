#pragma once

#include "CoreMinimal.h"
#include "Misc/DataValidation.h"
#include "EditorValidatorBase.h"
#include "ModBlueprintValidator.generated.h"

/**
 * Editor validator that fails any Blueprint which calls a function listed in
 * UModValidationSettings::RequiredPins while leaving the named input pin unconnected
 * and without a non-null default value.
 *
 * Runs automatically on save and during the Alpakit cook (the cook commandlet invokes
 * the EditorValidatorSubsystem), and on demand via right-click -> "Validate Assets".
 */
UCLASS()
class UModBlueprintValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

public:
	UModBlueprintValidator();

protected:
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) const override;

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) override;
};
