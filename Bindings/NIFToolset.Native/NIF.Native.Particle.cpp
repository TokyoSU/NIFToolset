#include "NIF.Native.Particle.h"
#include "NIF.Native.Internal.h"
#include "NIF.Native.Mesh.h"

#include <NiFixedString.h>
#include <NiPSEmitter.h>
#include <NiPSParticleSystem.h>

namespace
{
	struct NIF_ParticleMeshHandle_t
	{
		NiMeshPtr spObject;
	};

	NiPSParticleSystem* NIF_GetParticleSystem(NIF_ParticleSystemHandle particleSystem)
	{
		NIF_ParticleSystemHandle_t* pHandle = static_cast<NIF_ParticleSystemHandle_t*>(particleSystem);
		return pHandle ? NiDynamicCast(NiPSParticleSystem, pHandle->spObject) : nullptr;
	}

	NiPSEmitter* NIF_GetEmitter(NIF_PSEmitterHandle emitter)
	{
		NIF_PSEmitterHandle_t* pHandle = static_cast<NIF_PSEmitterHandle_t*>(emitter);
		return pHandle ? NiDynamicCast(NiPSEmitter, pHandle->spObject) : nullptr;
	}
}

NIF_ParticleSystemHandle NIF_CreateParticleSystemHandle(NiPSParticleSystem* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_ParticleSystemHandle_t* pHandle = new NIF_ParticleSystemHandle_t();
	pHandle->spObject = pkObject;
	return pHandle;
}

NIF_PSEmitterHandle NIF_CreatePSEmitterHandle(NiPSEmitter* pkObject)
{
	if (!pkObject)
	{
		return nullptr;
	}

	NIF_PSEmitterHandle_t* pHandle = new NIF_PSEmitterHandle_t();
	pHandle->spObject = pkObject;
	return pHandle;
}

extern "C"
{

void NIF_Particle_System_Destroy(NIF_ParticleSystemHandle particleSystem)
{
	delete static_cast<NIF_ParticleSystemHandle_t*>(particleSystem);
}

NIF_ParticleSystemHandle NIF_Particle_System_Create(unsigned int maxNumParticles, int normalMethod, NIF_Vec3 normalDirection, int upMethod, NIF_Vec3 upDirection, int hasLivingSpawner, int hasColors, int hasRotations, int hasAnimatedTextures, int worldSpace, int dynamicBounds, int createDefaultGenerator, int attachMeshModifiers, int sorted)
{
	NiPSParticleSystem* pkParticleSystem = NiPSParticleSystem::Create(maxNumParticles,
		static_cast<NiPSParticleSystem::AlignMethod>(normalMethod),
		NIF_MakePoint3(normalDirection),
		static_cast<NiPSParticleSystem::AlignMethod>(upMethod),
		NIF_MakePoint3(upDirection),
		hasLivingSpawner != 0,
		hasColors != 0,
		hasRotations != 0,
		hasAnimatedTextures != 0,
		worldSpace != 0,
		dynamicBounds != 0,
		createDefaultGenerator != 0,
		attachMeshModifiers != 0,
		sorted != 0);
	return NIF_CreateParticleSystemHandle(pkParticleSystem);
}

unsigned int NIF_Particle_System_GetMaxNumParticles(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? pkParticleSystem->GetMaxNumParticles() : 0;
}

unsigned int NIF_Particle_System_GetNumParticles(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? pkParticleSystem->GetNumParticles() : 0;
}

float NIF_Particle_System_GetLastTime(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? pkParticleSystem->GetLastTime() : 0.0f;
}

int NIF_Particle_System_HasLivingSpawner(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->HasLivingSpawner()) ? 1 : 0;
}

int NIF_Particle_System_HasColors(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->HasColors()) ? 1 : 0;
}

int NIF_Particle_System_HasRotations(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->HasRotations()) ? 1 : 0;
}

int NIF_Particle_System_HasRotationSpeeds(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->HasRotationSpeeds()) ? 1 : 0;
}

int NIF_Particle_System_HasRotationAxes(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->HasRotationAxes()) ? 1 : 0;
}

int NIF_Particle_System_HasAnimatedTextures(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->HasAnimatedTextures()) ? 1 : 0;
}

int NIF_Particle_System_IsSorted(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->IsSorted()) ? 1 : 0;
}

int NIF_Particle_System_GetWorldSpace(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && pkParticleSystem->GetWorldSpace()) ? 1 : 0;
}

void NIF_Particle_System_SetWorldSpace(NIF_ParticleSystemHandle particleSystem, int worldSpace)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (pkParticleSystem)
	{
		pkParticleSystem->SetWorldSpace(worldSpace != 0);
	}
}

NIF_Transform NIF_Particle_System_GetOriginalWorldTransform(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? NIF_MakeTransform(pkParticleSystem->GetOriginalWorldTransform()) : NIF_Transform{ NIF_MakeIdentityMat3(), { 0.0f, 0.0f, 0.0f }, 1.0f };
}

void NIF_Particle_System_Reset(NIF_ParticleSystemHandle particleSystem, float newLastTime)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (pkParticleSystem)
	{
		pkParticleSystem->ResetParticleSystem(newLastTime);
	}
}

void NIF_Particle_System_ForceSimulationToComplete(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (pkParticleSystem)
	{
		pkParticleSystem->ForceSimulationToComplete();
	}
}

unsigned int NIF_Particle_System_GetSpawnerCount(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? pkParticleSystem->GetSpawnerCount() : 0;
}

int NIF_Particle_System_GetNextSpawnTime(NIF_ParticleSystemHandle particleSystem, float time, float age, float lifeSpan, float* outSpawnTime)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	float spawnTime = 0.0f;
	if (!pkParticleSystem || !outSpawnTime || !pkParticleSystem->GetNextSpawnTime(time, age, lifeSpan, spawnTime))
	{
		return 0;
	}

	*outSpawnTime = spawnTime;
	return 1;
}

float NIF_Particle_System_GetRunUpTime(NIF_ParticleSystemHandle particleSystem, int includeSpawners)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? pkParticleSystem->GetRunUpTime(includeSpawners != 0) : 0.0f;
}

unsigned int NIF_Particle_System_GetEmitterCount(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? pkParticleSystem->GetEmitterCount() : 0;
}

NIF_PSEmitterHandle NIF_Particle_System_GetEmitterAt(NIF_ParticleSystemHandle particleSystem, unsigned int index)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (!pkParticleSystem || index >= pkParticleSystem->GetEmitterCount())
	{
		return nullptr;
	}

	return NIF_CreatePSEmitterHandle(pkParticleSystem->GetEmitterAt(index));
}

NIF_PSEmitterHandle NIF_Particle_System_GetEmitterByName(NIF_ParticleSystemHandle particleSystem, const char* name)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return (pkParticleSystem && name) ? NIF_CreatePSEmitterHandle(pkParticleSystem->GetEmitterByName(NiFixedString(name))) : nullptr;
}

void NIF_Particle_System_AddEmitter(NIF_ParticleSystemHandle particleSystem, NIF_PSEmitterHandle emitter)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkParticleSystem && pkEmitter)
	{
		pkParticleSystem->AddEmitter(pkEmitter);
	}
}

void NIF_Particle_System_RemoveEmitterAt(NIF_ParticleSystemHandle particleSystem, unsigned int index, int maintainOrder)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (pkParticleSystem && index < pkParticleSystem->GetEmitterCount())
	{
		pkParticleSystem->RemoveEmitterAt(index, maintainOrder != 0);
	}
}

void NIF_Particle_System_RemoveAllEmitters(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (pkParticleSystem)
	{
		pkParticleSystem->RemoveAllEmitters();
	}
}

NIF_MeshHandle NIF_Particle_System_AsMesh(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	if (!pkParticleSystem)
	{
		return nullptr;
	}

	NIF_ParticleMeshHandle_t* pHandle = new NIF_ParticleMeshHandle_t();
	pHandle->spObject = pkParticleSystem;
	return static_cast<NIF_MeshHandle>(pHandle);
}

NIF_AVObjectHandle NIF_Particle_System_AsAVObject(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? NIF_CreateAVObjectHandle(pkParticleSystem) : nullptr;
}

NIF_ObjectHandle NIF_Particle_System_AsObject(NIF_ParticleSystemHandle particleSystem)
{
	NiPSParticleSystem* pkParticleSystem = NIF_GetParticleSystem(particleSystem);
	return pkParticleSystem ? NIF_CreateObjectHandle(pkParticleSystem) : nullptr;
}

void NIF_Particle_Emitter_Destroy(NIF_PSEmitterHandle emitter)
{
	delete static_cast<NIF_PSEmitterHandle_t*>(emitter);
}

const char* NIF_Particle_Emitter_GetName(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? static_cast<const char*>(pkEmitter->GetName()) : nullptr;
}

void NIF_Particle_Emitter_SetName(NIF_PSEmitterHandle emitter, const char* name)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter && name)
	{
		pkEmitter->SetName(NiFixedString(name));
	}
}

float NIF_Particle_Emitter_GetSpeed(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetSpeed() : 0.0f;
}

void NIF_Particle_Emitter_SetSpeed(NIF_PSEmitterHandle emitter, float speed)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetSpeed(speed);
	}
}

float NIF_Particle_Emitter_GetSpeedVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetSpeedVar() : 0.0f;
}

void NIF_Particle_Emitter_SetSpeedVar(NIF_PSEmitterHandle emitter, float speedVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetSpeedVar(speedVar);
	}
}

float NIF_Particle_Emitter_GetSpeedFlipRatio(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetSpeedFlipRatio() : 0.0f;
}

void NIF_Particle_Emitter_SetSpeedFlipRatio(NIF_PSEmitterHandle emitter, float speedFlipRatio)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetSpeedFlipRatio(speedFlipRatio);
	}
}

float NIF_Particle_Emitter_GetLifeSpan(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetLifeSpan() : 0.0f;
}

void NIF_Particle_Emitter_SetLifeSpan(NIF_PSEmitterHandle emitter, float lifeSpan)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetLifeSpan(lifeSpan);
	}
}

float NIF_Particle_Emitter_GetLifeSpanVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetLifeSpanVar() : 0.0f;
}

void NIF_Particle_Emitter_SetLifeSpanVar(NIF_PSEmitterHandle emitter, float lifeSpanVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetLifeSpanVar(lifeSpanVar);
	}
}

float NIF_Particle_Emitter_GetDeclination(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetDeclination() : 0.0f;
}

void NIF_Particle_Emitter_SetDeclination(NIF_PSEmitterHandle emitter, float declination)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetDeclination(declination);
	}
}

float NIF_Particle_Emitter_GetDeclinationVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetDeclinationVar() : 0.0f;
}

void NIF_Particle_Emitter_SetDeclinationVar(NIF_PSEmitterHandle emitter, float declinationVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetDeclinationVar(declinationVar);
	}
}

float NIF_Particle_Emitter_GetPlanarAngle(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetPlanarAngle() : 0.0f;
}

void NIF_Particle_Emitter_SetPlanarAngle(NIF_PSEmitterHandle emitter, float planarAngle)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetPlanarAngle(planarAngle);
	}
}

float NIF_Particle_Emitter_GetPlanarAngleVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetPlanarAngleVar() : 0.0f;
}

void NIF_Particle_Emitter_SetPlanarAngleVar(NIF_PSEmitterHandle emitter, float planarAngleVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetPlanarAngleVar(planarAngleVar);
	}
}

float NIF_Particle_Emitter_GetSize(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetSize() : 0.0f;
}

void NIF_Particle_Emitter_SetSize(NIF_PSEmitterHandle emitter, float size)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetSize(size);
	}
}

float NIF_Particle_Emitter_GetSizeVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetSizeVar() : 0.0f;
}

void NIF_Particle_Emitter_SetSizeVar(NIF_PSEmitterHandle emitter, float sizeVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetSizeVar(sizeVar);
	}
}

float NIF_Particle_Emitter_GetRotAngle(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetRotAngle() : 0.0f;
}

void NIF_Particle_Emitter_SetRotAngle(NIF_PSEmitterHandle emitter, float rotAngle)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRotAngle(rotAngle);
	}
}

float NIF_Particle_Emitter_GetRotAngleVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetRotAngleVar() : 0.0f;
}

void NIF_Particle_Emitter_SetRotAngleVar(NIF_PSEmitterHandle emitter, float rotAngleVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRotAngleVar(rotAngleVar);
	}
}

float NIF_Particle_Emitter_GetRotSpeed(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetRotSpeed() : 0.0f;
}

void NIF_Particle_Emitter_SetRotSpeed(NIF_PSEmitterHandle emitter, float rotSpeed)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRotSpeed(rotSpeed);
	}
}

float NIF_Particle_Emitter_GetRotSpeedVar(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? pkEmitter->GetRotSpeedVar() : 0.0f;
}

void NIF_Particle_Emitter_SetRotSpeedVar(NIF_PSEmitterHandle emitter, float rotSpeedVar)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRotSpeedVar(rotSpeedVar);
	}
}

int NIF_Particle_Emitter_GetRandomRotSpeedSign(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return (pkEmitter && pkEmitter->GetRandomRotSpeedSign()) ? 1 : 0;
}

void NIF_Particle_Emitter_SetRandomRotSpeedSign(NIF_PSEmitterHandle emitter, int randomRotSpeedSign)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRandomRotSpeedSign(randomRotSpeedSign != 0);
	}
}

NIF_Vec3 NIF_Particle_Emitter_GetRotAxis(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? NIF_MakeVec3(pkEmitter->GetRotAxis()) : NIF_Vec3{ 0.0f, 0.0f, 0.0f };
}

void NIF_Particle_Emitter_SetRotAxis(NIF_PSEmitterHandle emitter, NIF_Vec3 rotAxis)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRotAxis(NIF_MakePoint3(rotAxis));
	}
}

int NIF_Particle_Emitter_GetRandomRotAxis(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return (pkEmitter && pkEmitter->GetRandomRotAxis()) ? 1 : 0;
}

void NIF_Particle_Emitter_SetRandomRotAxis(NIF_PSEmitterHandle emitter, int randomRotAxis)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->SetRandomRotAxis(randomRotAxis != 0);
	}
}

void NIF_Particle_Emitter_ResetTransformation(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	if (pkEmitter)
	{
		pkEmitter->ResetTransformation();
	}
}

NIF_ObjectHandle NIF_Particle_Emitter_AsObject(NIF_PSEmitterHandle emitter)
{
	NiPSEmitter* pkEmitter = NIF_GetEmitter(emitter);
	return pkEmitter ? NIF_CreateObjectHandle(pkEmitter) : nullptr;
}

}
