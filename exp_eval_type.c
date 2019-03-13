#include "exp_eval_a.h"
#include "exp_eval_type_a.h"




typedef struct EXP_EvalTypeDescFrame
{
    EXP_EvalTypeDesc desc;
    EXP_EvalTypeDesc desc1;
    u32 remain;
} EXP_EvalTypeDescFrame;

typedef vec_t(EXP_EvalTypeDescFrame) EXP_EvalTypeDescStack;




typedef struct EXP_EvalTypeContext
{
    Upool* listPool;
    Upool* typePool;
    EXP_EvalTypeDescStack descStack;
} EXP_EvalTypeContext;


EXP_EvalTypeContext* EXP_newEvalTypeContext(void)
{
    EXP_EvalTypeContext* ctx = zalloc(sizeof(EXP_EvalTypeContext));
    ctx->listPool = newUpool(256);
    ctx->typePool = newUpool(256);
    return ctx;
}

void EXP_evalTypeContextFree(EXP_EvalTypeContext* ctx)
{
    vec_free(&ctx->descStack);
    upoolFree(ctx->typePool);
    upoolFree(ctx->listPool);
    free(ctx);
}






static u32 EXP_evalTypeIdByDesc(EXP_EvalTypeContext* ctx, const EXP_EvalTypeDesc* desc)
{
    u32 id = upoolAddElm(ctx->typePool, desc, sizeof(*desc), NULL);
    return id;
}

static u32 EXP_evalTypeList(EXP_EvalTypeContext* ctx, u32 count, const u32* elms)
{
    u32 id = upoolAddElm(ctx->listPool, elms, sizeof(*elms)*count, NULL);
    return id;
}





u32 EXP_evalTypeAtom(EXP_EvalTypeContext* ctx, u32 atom)
{
    EXP_EvalTypeDesc desc = { EXP_EvalTypeType_Atom };
    desc.atom = atom;
    return EXP_evalTypeIdByDesc(ctx, &desc);
}

u32 EXP_evalTypeFun(EXP_EvalTypeContext* ctx, u32 numIns, const u32* ins, u32 numOuts, const u32* outs)
{
    EXP_EvalTypeDesc desc = { EXP_EvalTypeType_Fun };
    desc.fun.ins.count = numIns;
    desc.fun.outs.count = numOuts;
    desc.fun.ins.list = EXP_evalTypeList(ctx, numIns, ins);
    desc.fun.outs.list = EXP_evalTypeList(ctx, numOuts, outs);
    return EXP_evalTypeIdByDesc(ctx, &desc);
}

u32 EXP_evalTypeTuple(EXP_EvalTypeContext* ctx, u32 count, const u32* elms)
{
    EXP_EvalTypeDesc desc = { EXP_EvalTypeType_Tuple };
    desc.tuple.count = count;
    desc.tuple.list = EXP_evalTypeList(ctx, count, elms);
    return EXP_evalTypeIdByDesc(ctx, &desc);
}

u32 EXP_evalTypeArray(EXP_EvalTypeContext* ctx, u32 elm)
{
    EXP_EvalTypeDesc desc = { EXP_EvalTypeType_Array };
    desc.aryElm = elm;
    return EXP_evalTypeIdByDesc(ctx, &desc);
}

u32 EXP_evalTypeVar(EXP_EvalTypeContext* ctx, u32 var)
{
    EXP_EvalTypeDesc desc = { EXP_EvalTypeType_Var };
    desc.var = var;
    return EXP_evalTypeIdByDesc(ctx, &desc);
}

u32 EXP_evalTypeVarS1(EXP_EvalTypeContext* ctx, u32 var)
{
    EXP_EvalTypeDesc desc = { EXP_EvalTypeType_VarS1 };
    desc.var = var;
    return EXP_evalTypeIdByDesc(ctx, &desc);
}






const EXP_EvalTypeDesc* EXP_evalTypeDescById(EXP_EvalTypeContext* ctx, u32 typeId)
{
    return upoolElmData(ctx->typePool, typeId);
}

const u32* EXP_evalTypeListById(EXP_EvalTypeContext* ctx, u32 listId)
{
    return upoolElmData(ctx->listPool, listId);
}













u32* EXP_evalTypeVarValueGet(EXP_EvalTypeVarSpace* varSpace, u32 var)
{
    for (u32 i = 0; i < varSpace->length; ++i)
    {
        if (varSpace->data[i].id == var)
        {
            return &varSpace->data[i].val;
        }
    }
    return NULL;
}

void EXP_evalTypeVarValueSet(EXP_EvalTypeVarSpace* varSpace, u32 var, u32 value)
{
    for (u32 i = 0; i < varSpace->length; ++i)
    {
        if (varSpace->data[i].id == var)
        {
            varSpace->data[i].val = value;
            return;
        }
    }
    EXP_EvalTypeVarBinding b = { var, value };
    vec_push(varSpace, b);
}



















u32 EXP_evalTypeNormForm(EXP_EvalTypeContext* ctx, EXP_EvalTypeVarSpace* varSpace, u32 x)
{
    EXP_EvalTypeDescStack* descStack = &ctx->descStack;
    EXP_EvalTypeDesc desc = *EXP_evalTypeDescById(ctx, x);
    u32 nf = x;
next:
    switch (desc.type)
    {
    case EXP_EvalTypeType_Var:
    {
        u32* pValue = EXP_evalTypeVarValueGet(varSpace, desc.var);
        if (pValue)
        {
            EXP_EvalTypeDescFrame f = { desc };
            vec_push(descStack, f);
            desc = *EXP_evalTypeDescById(ctx, *pValue);
            goto next;
        }
    }
    default:
        assert(false);
    }
    if (descStack->length > 0)
    {
        EXP_EvalTypeDesc* pDesc = &vec_last(descStack).desc;
        switch (pDesc->type)
        {
        case EXP_EvalTypeType_Var:
        {
            if (pDesc->var != nf)
            {
                EXP_evalTypeVarValueSet(varSpace, pDesc->var, nf);
            }
            break;
        }
        default:
            break;
        }
        vec_pop(descStack);
        if (descStack->length > 0)
        {
            EXP_EvalTypeDescFrame* f = &vec_last(descStack);
            desc = f->desc;
            goto next;
        }
    }
    return nf;
}







u32 EXP_evalTypeToS1Form(EXP_EvalTypeContext* ctx, EXP_EvalTypeVarSpace* varSpace, u32 x)
{
    const EXP_EvalTypeDesc* desc = NULL;
enter:
    desc = EXP_evalTypeDescById(ctx, x);
    switch (desc->type)
    {
    case EXP_EvalTypeType_Atom:
    {
        return x;
    }
    case EXP_EvalTypeType_Var:
    {
        x = EXP_evalTypeVarS1(ctx, desc->var);
        goto enter;
    }
    case EXP_EvalTypeType_VarS1:
    {
        u32* pValue = EXP_evalTypeVarValueGet(varSpace, desc->var);
        if (pValue)
        {
            x = *pValue;
            goto enter;
        }
        return x;
    }
    default:
        assert(false);
        return x;
    }
}















bool EXP_evalTypeUnify(EXP_EvalTypeContext* ctx, EXP_EvalTypeVarSpace* varSpace, u32 a, u32 b, u32* pU)
{
enter:
    if (a == b)
    {
        *pU = EXP_evalTypeNormForm(ctx, varSpace, a);
        return true;
    }
    const EXP_EvalTypeDesc* descA = EXP_evalTypeDescById(ctx, a);
    const EXP_EvalTypeDesc* descB = EXP_evalTypeDescById(ctx, b);
    if (descA->type == descB->type)
    {
        switch (descA->type)
        {
        case EXP_EvalTypeType_Atom:
        {
            assert(descA->atom != descB->atom);
            return false;
        }
        default:
            assert(false);
            return false;
        }
    }
    else
    {
    swap:
        if (EXP_EvalTypeType_Var == descB->type)
        {
            u32 t = b;
            b = a;
            a = t;
            const EXP_EvalTypeDesc* descC = descB;
            descB = descA;
            descA = descC;
            goto swap;
        }
        if (EXP_EvalTypeType_Var == descA->type)
        {
            u32* pValue = EXP_evalTypeVarValueGet(varSpace, descA->var);
            if (pValue)
            {
                a = *pValue;
                goto enter;
            }
            u32 v = *pU = EXP_evalTypeNormForm(ctx, varSpace, b);
            // todo move
            EXP_evalTypeVarValueSet(varSpace, descA->var, v);
            return true;
        }
        else
        {
            return false;
        }
    }
}






















bool EXP_evalTypeUnifyVarS1
(
    EXP_EvalTypeContext* ctx, EXP_EvalTypeVarSpace* varSpace, u32 x, EXP_EvalTypeVarSpace* varSpace1, u32 x1
)
{
    const EXP_EvalTypeDesc* descX = EXP_evalTypeDescById(ctx, x);
    const EXP_EvalTypeDesc* descX1 = EXP_evalTypeDescById(ctx, x1);
    u32* pVal1 = NULL;
    if (EXP_EvalTypeType_Var == descX1->type)
    {
        pVal1 = EXP_evalTypeVarValueGet(varSpace1, descX1->var);
    }
    if (pVal1)
    {
        u32 val1 = *pVal1;
        if (EXP_EvalTypeType_Var == descX->type)
        {
            u32* pVal = EXP_evalTypeVarValueGet(varSpace, x);
            // todo
            if (pVal)
            {
                u32 val = *pVal;
                u32 u;
                EXP_evalTypeUnify(ctx, varSpace, val, val1, &u);
                EXP_evalTypeVarValueSet(varSpace, descX->var, u);
                return true;
            }
            else
            {
                EXP_evalTypeVarValueSet(varSpace, descX->var, val1);
                return true;
            }
        }
        else
        {
            return val1 == x;
        }
    }
    else
    {
        EXP_evalTypeVarValueSet(varSpace1, descX1->var, x);
        return true;
    }
    return true;
}























































































































