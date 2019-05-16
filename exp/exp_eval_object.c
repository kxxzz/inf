#include "exp_eval_a.h"






EXP_EvalObjectTable EXP_newEvalObjectTable(EXP_EvalAtypeInfoVec* atypeTable)
{
    EXP_EvalObjectTable _objectTable = { 0 };
    EXP_EvalObjectTable* objectTable = &_objectTable;
    vec_resize(objectTable, atypeTable->length);
    memset(objectTable->data, 0, sizeof(objectTable->data[0])*objectTable->length);
    return *objectTable;
}




void EXP_evalObjectTableFree(EXP_EvalObjectTable* objectTable, EXP_EvalAtypeInfoVec* atypeTable)
{
    for (u32 i = 0; i < objectTable->length; ++i)
    {
        EXP_EvalObjectPtrVec* mpVec = objectTable->data + i;
        EXP_EvalAtypeInfo* atypeInfo = atypeTable->data + i;
        if ((atypeInfo->allocMemSize > 0) || atypeInfo->aDtor)
        {
            for (u32 i = 0; i < mpVec->length; ++i)
            {
                if (atypeInfo->aDtor)
                {
                    atypeInfo->aDtor(mpVec->data[i]->a);
                }
                if (atypeInfo->allocMemSize > 0)
                {
                    free(mpVec->data[i]);
                }
            }
        }
        else
        {
            assert(0 == atypeInfo->allocMemSize);
            assert(0 == mpVec->length);
        }
        vec_free(mpVec);
    }
    vec_free(objectTable);
}












void EXP_evalSetGcFlag(EXP_EvalContext* ctx, EXP_EvalValue v)
{
    switch (v.type)
    {
    case EXP_EvalValueType_AtomObj:
    {
        EXP_EvalObject* m = (EXP_EvalObject*)((char*)v.a - offsetof(EXP_EvalObject, a));
        if (m->gcFlag != ctx->gcFlag)
        {
            m->gcFlag = ctx->gcFlag;
        }
        break;
    }
    default:
        break;
    }
}







void EXP_evalGC(EXP_EvalContext* ctx)
{
    EXP_EvalValueVec* varStack = &ctx->varStack;
    EXP_EvalValueVec* dataStack = &ctx->dataStack;
    EXP_EvalAtypeInfoVec* atypeTable = &ctx->atypeTable;
    EXP_EvalObjectTable* objectTable = &ctx->objectTable;

    ctx->gcFlag = !ctx->gcFlag;

    for (u32 i = 0; i < varStack->length; ++i)
    {
        EXP_evalSetGcFlag(ctx, varStack->data[i]);
    }
    for (u32 i = 0; i < dataStack->length; ++i)
    {
        EXP_evalSetGcFlag(ctx, dataStack->data[i]);
    }

    for (u32 i = 0; i < objectTable->length; ++i)
    {
        EXP_EvalObjectPtrVec* mpVec = objectTable->data + i;
        EXP_EvalAtypeInfo* atypeInfo = atypeTable->data + i;
        if ((atypeInfo->allocMemSize > 0) || atypeInfo->aDtor)
        {
            for (u32 i = 0; i < mpVec->length; ++i)
            {
                bool gcFlag = mpVec->data[i]->gcFlag;
                if (gcFlag != ctx->gcFlag)
                {
                    if (atypeInfo->aDtor)
                    {
                        atypeInfo->aDtor(mpVec->data[i]->a);
                    }
                    if (atypeInfo->allocMemSize > 0)
                    {
                        free(mpVec->data[i]);
                    }
                    mpVec->data[i] = vec_last(mpVec);
                    vec_pop(mpVec);
                    --i;
                }
            }
        }
        else
        {
            assert(0 == atypeInfo->allocMemSize);
            assert(0 == mpVec->length);
        }
    }
}









EXP_EvalValue EXP_evalNewAtomObject(EXP_EvalContext* ctx, const char* str, u32 len, u32 atype)
{
    EXP_EvalAtypeInfoVec* atypeTable = &ctx->atypeTable;
    EXP_EvalObjectTable* objectTable = &ctx->objectTable;

    EXP_EvalValue v = { 0 };
    EXP_EvalAtypeInfo* atypeInfo = atypeTable->data + atype;
    if (atypeInfo->allocMemSize > 0)
    {
        EXP_EvalObject* m = (EXP_EvalObject*)zalloc(offsetof(EXP_EvalObject, a) + atypeInfo->allocMemSize);
        v.a = m->a;
        EXP_EvalObjectPtrVec* mpVec = objectTable->data + atype;
        vec_push(mpVec, m);
        v.type = EXP_EvalValueType_AtomObj;
        m->gcFlag = ctx->gcFlag;
    }
    assert(atypeInfo->ctorByStr);
    if (!atypeInfo->ctorByStr(str, len, &v))
    {
        assert(false);
    }
    return v;
}


















































