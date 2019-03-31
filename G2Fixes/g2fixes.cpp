#include <windows.h>
#include <direct.h>
#include <stdio.h>
#include <stdint.h>
#include "..\G12Dll\MemoryMgr.h"

#define G12DLL_NAME "G2Fixes"

#include "..\G12Dll\G12.h"

#include "..\G12Dll\G2.hpp"
#include "..\G12Dll\G2.h"
#include "g2fixes.hpp"

void hNpc::CreateVobList(float max_dist)
{
	zCVob *vob;
	oCMobInter *mob;
	oCNpc *npc;
	zVEC3 trafo_vec;
	zTBBox3D bbox;
	zCClassDef *classDef;
	int i;

	int delete_vob;

	if (this->homeWorld)
	{
		this->ClearVobList();

		this->trafoObjToWorld.GetTranslation(trafo_vec);

		bbox.maxs.n[0] = trafo_vec.n[0] + max_dist;
		bbox.maxs.n[1] = trafo_vec.n[1] + max_dist;
		bbox.maxs.n[2] = trafo_vec.n[2] + max_dist;

		bbox.mins.n[0] = trafo_vec.n[0] - max_dist;
		bbox.mins.n[1] = trafo_vec.n[1] - max_dist;
		bbox.mins.n[2] = trafo_vec.n[2] - max_dist;

		this->homeWorld->bspTree.bspRoot->CollectVobsInBBox3D(this->vobList, bbox);

		for (i = 0; i < this->vobList.numInArray; i++)
		{
			delete_vob = 0;

			vob = this->vobList.array[i];
			classDef = vob->_GetClassDef();

			if (vob == this)
			{
				delete_vob = 1;
			}

			if (zCObject::CheckInheritance(&oCMobInter::classDef, classDef))
			{
				mob = (oCMobInter *)vob;

				if (mob->IsOccupied())
				{
					delete_vob = 1;
				}
			}

			if (zCObject::CheckInheritance(&oCNpc::classDef, classDef))
			{
				npc = (oCNpc *)vob;

				if (npc->attribute[0] <= 0 && npc->inventory2.IsEmpty(1, 1))
				{
					delete_vob = 1;
				}
			}

			if (delete_vob)
			{
				this->vobList.RemoveIndex(i--);
			}
			else
			{
				vob->refCtr++;
			}
		}
	}
}

void PatchFocus(void)
{
	InjectHook(0x0073369B, &hNpc::CreateVobList); // oCNpc::ToggleFocusVob()
	InjectHook(0x00733BE9, &hNpc::CreateVobList); // oCNpc::CollectFocusVob()
	InjectHook(0x0075DC54, &hNpc::CreateVobList); // oCNpc::PerceiveAll()
	InjectHook(0x0075DE95, &hNpc::CreateVobList); // oCNpc::PerceptionCheck()
}

static zVEC3 defaultCol0;
static zVEC3 defaultCol1;
static zVEC3 defaultCol2;
static zVEC3 defaultCol3;

void hSkyControler_Outdoor::ReadFogColorsFromINI()
{
	this->zCSkyControler_Outdoor::ReadFogColorsFromINI();

	this->fogColorDayVariations.array[0] = defaultCol0;
	this->fogColorDayVariations.array[1] = defaultCol1;
	this->fogColorDayVariations.array[2] = defaultCol2;
	this->fogColorDayVariations.array[3] = defaultCol3;
}

void PatchFogColors(void)
{
	defaultCol0.n[0] = 116;
	defaultCol0.n[1] = 89;
	defaultCol0.n[2] = 75;

	defaultCol1.n[0] = 80;
	defaultCol1.n[1] = 90;
	defaultCol1.n[2] = 80;

	defaultCol2.n[0] = 120;
	defaultCol2.n[1] = 140;
	defaultCol2.n[2] = 180;

	defaultCol3.n[0] = 120;
	defaultCol3.n[1] = 140;
	defaultCol3.n[2] = 180;

	InjectHook(0x005E6443, &hSkyControler_Outdoor::ReadFogColorsFromINI); // zCSkyControler_Outdoor::zCSkyControler_Outdoor()
}

const char *Gothic1AppName = "Gothic - 2.6 (fix)";
const char *Gothic1WorldZen = "WORLD.ZEN";
const char *NoSound = "NEWGAME";

void PatchGothic2(void)
{
	if (G12GetPrivateProfileInt("Gothic1Mode", 0))
	{
		// Fix App Title
		Patch(0x0089D9AC, Gothic1AppName);

		// New game starts WORLD.ZEN
		Patch(0x00429A23 + 1, Gothic1WorldZen);
		Patch(0x00429A52 + 1, Gothic1WorldZen);

		// No GAMESTART menu "music"
		Patch(0x004DB7EE + 1, NoSound);
		Patch(0x004DB815 + 1, NoSound);

		// Fix progress bar on Loading Screen
		Patch(0x006C282B + 1, 6600);
		Patch(0x006C2830 + 1, 6192);
		Patch(0x006C2835 + 1, 6100);
		Patch(0x006C283A + 1, 2000);

		// Patch fogcolors to Gothic 1 fogcolors (perhaps change this so it only happens on WORLD, and gets done dynamically on levelchange?)
		PatchFogColors();
	}

	if (G12GetPrivateProfileInt("NoGamestartMusic", 0))
	{
		// No GAMESTART menu "music"
		Patch(0x004DB7EE + 1, NoSound);
		Patch(0x004DB815 + 1, NoSound);
	}

	if (G12GetPrivateProfileInt("HideFocus", 0))
	{
		// Unlike HideFocus from Systempack which is sometimes buggy and where vobs can still be focused when turning around quickly and spamming ctrl
		// this patches CreateVobList() to the Sequel variant where a dead, empty NPC does not even end up in the focusable voblist
		PatchFocus();
	}
}

void Init(void)
{
	if (GOTHIC2)
	{
		G12AllocConsole();
		PatchGothic2();
	}
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		Init();
	}

	return TRUE;
}