#include "ModValidationModule.h"

#define LOCTEXT_NAMESPACE "FModValidationModule"

void FModValidationModule::StartupModule()
{
	// Nothing to do here. The editor's UEditorValidatorSubsystem automatically
	// discovers every UEditorValidatorBase subclass (incl. UModBlueprintValidator)
	// and runs the enabled ones on save and during the Alpakit cook.
}

void FModValidationModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModValidationModule, ModValidation)
