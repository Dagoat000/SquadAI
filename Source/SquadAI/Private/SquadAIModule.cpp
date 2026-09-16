// SquadAIModule.cpp
// Minimal module entry point - this system has no startup/shutdown work to do
// (no subsystem registration needed beyond what UWorldSubsystem/UActorComponent
// already handle automatically), so this is intentionally boilerplate.

#include "Modules/ModuleManager.h"

IMPLEMENT_MODULE(FDefaultModuleImpl, SquadAI);
