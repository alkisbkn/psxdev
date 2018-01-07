/* $PSLibId: Runtime Library Release 3.6$ */
/*
 *
 *	TOD functions (part1)
 : 	TOD�A�j���[�V���������֐��Q�i���̂P�j
 *
 *
 *		Version 1.30	Apr,  17, 1996
 *
 *		Copyright (C) 1995 by Sony Computer Entertainment Inc.
 *			All rights Reserved
 */

#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include <libgs.h>
#include "tod.h"

/*
 *  Prototype definition
 :  �v���g�^�C�v�錾
 */
void TodInitObjTable();
GsDOBJ2 *TodSearchObjByID();
GsDOBJ2 *TodCreateNewObj();
GsDOBJ2 *TodRemoveObj();
u_long *TodSearchTMDByID();

/*
 *  Initialize Object table
 :  �I�u�W�F�N�g�e�[�u���̏�����
 *       GsDOBJ2,GsCOORDINATE2, GsCOORD2PARAM ��g�ɂ��ėp��
 */
void TodInitObjTable(tblP, objAreaP, objCoordP, objParamP, nObj)
TodOBJTABLE *tblP;		/* : �e�[�u���ւ̃|�C���^ */
GsDOBJ2 *objAreaP;		/* : �I�u�W�F�N�g(DOBJ2)�̈� */
GsCOORDINATE2 *objCoordP;	/* : ���[�J�����W�̈� */
GsCOORD2PARAM *objParamP;	/* : ���[�J�����W�̃p�����[�^�̈� */
int nObj;			/* Maximum number of object
				 : �ő�OBJ���i�e�[�u���̑傫���j */
{
	int i;

	tblP->nobj = 0;		/* number of available object: �L���I�u�W�F�N�g�� */
	tblP->maxobj = nObj;	/* maximum number of object: �ő�I�u�W�F�N�g�� */
	tblP->top = objAreaP;	/* object area: �I�u�W�F�N�g�̈� */
	i = 0;
	for(i = 0; i < nObj; i++) {
		/* Initialize members: �I�u�W�F�N�g�\���̂̃����o�������� */
		objAreaP->attribute = 0x80000000;
		objAreaP->id = TOD_OBJ_UNDEF;
		objAreaP->coord2 = objCoordP;
		objAreaP->coord2->param = objParamP;
		objAreaP->tmd = NULL;
		objAreaP++;
		objCoordP++;
		objParamP++;
	}
}

/*
 *  Search object in the object table by ID
 :  �I�u�W�F�N�g�e�[�u������ID�ԍ��Ō���
 */
GsDOBJ2 *TodSearchObjByID(tblP, id)
TodOBJTABLE *tblP;	/* : �I�u�W�F�N�g�e�[�u���փ|�C���^ */
u_long id;		/* ID number for search: ��������I�u�W�F�N�g��ID�ԍ� */
{
	GsDOBJ2 *objP;
	int i;

	objP = tblP->top;
	for(i = 0; i < tblP->nobj; i++) {
		if(id == objP->id) break;
		objP++;
	}
	if(i == tblP->nobj) {
		/* Not Found */
		return(NULL);
	}
	else {
		/* Return pointer */
		return(objP);
	}
}

/*
 *  Create new object in the object table
 :  �V�����I�u�W�F�N�g���e�[�u�����ɐ���
 */
GsDOBJ2 *TodCreateNewObj(tblP, id)
TodOBJTABLE *tblP;	/* �I�u�W�F�N�g�e�[�u���փ|�C���^ */
u_long id;		/* ID number to create: ��������I�u�W�F�N�g��ID�ԍ� */
{
	GsDOBJ2 *objP;
	int i;

	/* Search undefined area in the table(ID=TOD_OBJ_UNDEF)
	 : �󂫗̈�(ID==TOD_OBJ_UNDEF)��T�� */
	objP = tblP->top;
	for(i = 0; i < tblP->nobj; i++) {
		if(objP->id == TOD_OBJ_UNDEF) break;
		objP++;
	}

	/* Search deleted area
	 : Deleted�̗̈�(����)�����邩�H */
	if(i < tblP->nobj) {
		objP->id = id;
		objP->attribute = 0;
		GsInitCoordinate2(WORLD, objP->coord2);
		objP->tmd = NULL;
		return(objP);
	}
	else {
		/* Add new object to bottom of the table
		 : �e�[�u���̍Ō���ɐV�K�I�u�W�F�N�g��ǉ� */
		if(i < tblP->maxobj) {		/* Table is Full? */
			objP = tblP->top+tblP->nobj;
			tblP->nobj++;
			objP->id = id;
			objP->attribute = 0;
			GsInitCoordinate2(WORLD, objP->coord2);
			objP->tmd = NULL;
			return(objP);
		}
		else {
			/* Table is full: �e�[�u���������ς��Ȃ�NULL��Ԃ� */
			return(NULL);
		}
	}
}

/*
 *  Delete object from object table
 :  �e�[�u������I�u�W�F�N�g���폜
 */
GsDOBJ2 *TodRemoveObj(tblP, id)
TodOBJTABLE *tblP;	/* Object table: �I�u�W�F�N�g�e�[�u���փ|�C���^ */
u_long id;		/* ID number to delete: �폜����I�u�W�F�N�g��ID�ԍ� */
{
	GsDOBJ2 *objP;
	int i;

	/* Search object: ID�ԍ��ŃI�u�W�F�N�g��T�� */
	objP = tblP->top;
	for(i = 0; i < tblP->nobj; i++) {
		if(objP->id == TOD_OBJ_UNDEF) break;
		objP++;
	}

	/* Delete it: �Y���I�u�W�F�N�g��T���č폜 */
	if(i < tblP->nobj) {
		objP->id = TOD_OBJ_UNDEF;
		if(i == tblP->nobj-1) {
			while(objP->id == TOD_OBJ_UNDEF) {
				tblP->nobj--;
				objP--;
			}
		}

		/* return object addr: �A�h���X��Ԃ� */
		return(objP);
	}
	else {
		/* retuen NULL when not found: ������Ȃ����NULL��Ԃ� */
		return(NULL);
	}
}

/*
 *  Search modeling data in TMD
 :  TMD���̃��f�����O�f�[�^��ID�ԍ��Ō���
 */
u_long *TodSearchTMDByID(tmdP, idListP, id)
u_long *tmdP;
int *idListP;
u_long id;
{
	int n;

	tmdP++;		/* Skip header: �w�b�_��ǂݔ�΂� */
	n = *tmdP++;	/* Number of object: ���f�����O�f�[�^�̐� */

	while(n > 0) {
		if(id == *idListP) break;
		tmdP += 7;	/* next object: �|�C���^��i�߂� */
		idListP++;
		n--;
	}
	if(n == 0) {
		/* return NULL when not found: ������Ȃ����NULL��Ԃ� */
		return(NULL);
	}
	else {
		/* return TMD addr: �A�h���X��Ԃ� */
		return(tmdP);
	}
}