// Copyright AndresAdarve. All Rights Reserved.

#include "InventoryExpansionModule.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "InventoryExpansion"

void FInventoryExpansionModule::StartupModule()
{
}

void FInventoryExpansionModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FInventoryExpansionModule, InventoryExpansion)
