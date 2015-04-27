/*-------------------------------------------------------------------------
 *
 * track_renames.c
 *        Event trigger for tracking object renames
 *
 * Copyright (c) 2015, Pete Hollobon
 * Copyright (c) 2013-2015, Michael Paquier
 * Copyright (c) 1996-2015, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *          track_renames/track_renames.c
 *
 *-------------------------------------------------------------------------
 */

#include <stdio.h>
#include "postgres.h"
#include "commands/event_trigger.h"
#include "nodes/parsenodes.h"
#include "fmgr.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "executor/spi.h"
#include "parser/parse_func.h"
#include "utils/guc.h"
#include "nodes/makefuncs.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(track_renames);

PGDLLEXPORT
Datum
track_renames(PG_FUNCTION_ARGS)
{
    EventTriggerData *trigdata;
    struct RenameStmt *statement;
    const Oid argtypes[] = {TEXTOID, TEXTOID, TEXTOID, TEXTOID, TEXTOID}; /* objtype text, schemaname text, relname text, subname text, newname text */
    Oid function_oid;
    const char *funcname;
    char *objtype;
    FmgrInfo flinfo;
    FunctionCallInfoData fcinfo1;

    if (!CALLED_AS_EVENT_TRIGGER(fcinfo))  /* internal error */
        elog(ERROR, "not fired by event trigger manager");

    trigdata = (EventTriggerData *)fcinfo->context;

    if (trigdata->parsetree->type != T_RenameStmt) {
        /* It's not possible to filter to just rename statements at TAG level when creating the event triigger  */
        PG_RETURN_NULL();
    }

    statement = (RenameStmt *)trigdata->parsetree;

    funcname = GetConfigOption("track_renames.function", true, false);

    if (funcname == NULL)
    {
        elog(WARNING, "track_renames.function parameter not set");
        PG_RETURN_NULL();
    }

    function_oid = LookupFuncName(list_make1(makeString(pstrdup(funcname))), 5, argtypes, true);

    if (function_oid == InvalidOid)
    {
        ereport(WARNING,
                (errcode(ERRCODE_UNDEFINED_FUNCTION),
                 errmsg("function %s does not exist",
                        func_signature_string(list_make1(makeString(pstrdup(funcname))), 5,
                                              NIL, argtypes))));
        PG_RETURN_NULL();
    }

    switch (statement->renameType)
    {
        case OBJECT_AGGREGATE:
            objtype = "aggregate";
            break;
        case OBJECT_ATTRIBUTE:
            objtype = "attribute";
            break;
        case OBJECT_CAST:
            objtype = "cast";
            break;
        case OBJECT_COLUMN:
            objtype = "column";
            break;
        case OBJECT_CONSTRAINT:
            objtype = "constraint";
            break;
        case OBJECT_COLLATION:
            objtype = "collation";
            break;
        case OBJECT_CONVERSION:
            objtype = "conversion";
            break;
        case OBJECT_DATABASE:
            objtype = "database";
            break;
        case OBJECT_DOMAIN:
            objtype = "domain";
            break;
        case OBJECT_EVENT_TRIGGER:
            objtype = "event_trigger";
            break;
        case OBJECT_EXTENSION:
            objtype = "extension";
            break;
        case OBJECT_FDW:
            objtype = "fdw";
            break;
        case OBJECT_FOREIGN_SERVER:
            objtype = "foreign_server";
            break;
        case OBJECT_FOREIGN_TABLE:
            objtype = "foreign_table";
            break;
        case OBJECT_FUNCTION:
            objtype = "function";
            break;
        case OBJECT_INDEX:
            objtype = "index";
            break;
        case OBJECT_LANGUAGE:
            objtype = "language";
            break;
        case OBJECT_LARGEOBJECT:
            objtype = "largeobject";
            break;
        case OBJECT_MATVIEW:
            objtype = "matview";
            break;
        case OBJECT_OPCLASS:
            objtype = "opclass";
            break;
        case OBJECT_OPERATOR:
            objtype = "operator";
            break;
        case OBJECT_OPFAMILY:
            objtype = "opfamily";
            break;
        case OBJECT_ROLE:
            objtype = "role";
            break;
        case OBJECT_RULE:
            objtype = "rule";
            break;
        case OBJECT_SCHEMA:
            objtype = "schema";
            break;
        case OBJECT_SEQUENCE:
            objtype = "sequence";
            break;
        case OBJECT_TABLE:
            objtype = "table";
            break;
        case OBJECT_TABLESPACE:
            objtype = "tablespace";
            break;
        case OBJECT_TRIGGER:
            objtype = "trigger";
            break;
        case OBJECT_TSCONFIGURATION:
            objtype = "tsconfiguration";
            break;
        case OBJECT_TSDICTIONARY:
            objtype = "tsdictionary";
            break;
        case OBJECT_TSPARSER:
            objtype = "tsparser";
            break;
        case OBJECT_TSTEMPLATE:
            objtype = "tstemplate";
            break;
        case OBJECT_TYPE:
            objtype = "type";
            break;
        case OBJECT_VIEW:
            objtype = "view";
            break;
        default:
            objtype = "unknown";
            elog(WARNING, "Unknown object type");
    }

    fmgr_info(function_oid, &flinfo);
    InitFunctionCallInfoData(fcinfo1, &flinfo, 5, InvalidOid, NULL, NULL);
    fcinfo1.arg[0] = CStringGetTextDatum(objtype);
    fcinfo1.argnull[0] = false;

    /* relation / object name */
    if (statement->relation) {
        if (statement->relation->schemaname) {
            fcinfo1.arg[1] = CStringGetTextDatum(statement->relation->schemaname);
            fcinfo1.argnull[1] = false;
        }
        else
        {
            fcinfo1.argnull[1] = true;
        }
        fcinfo1.arg[2] = CStringGetTextDatum(statement->relation->relname);
        fcinfo1.argnull[2] = false;
    }
    else if (statement->renameType == OBJECT_TYPE
             || statement->renameType == OBJECT_FUNCTION
             || statement->renameType == OBJECT_EVENT_TRIGGER
             || statement->renameType == OBJECT_SEQUENCE) {
        /* need schema name for functions etc. */
        fcinfo1.argnull[1] = true;
        fcinfo1.arg[2] = CStringGetTextDatum(NameListToString(statement->object));
        fcinfo1.argnull[2] = false;
    }
    else {
        fcinfo1.argnull[1] = true;
        fcinfo1.argnull[2] = true;
    }

    /* subname (e.g. column) */
    if (statement->subname) {
        fcinfo1.arg[3] = CStringGetTextDatum(statement->subname);
        fcinfo1.argnull[3] = false;
    }
    else {
        fcinfo1.argnull[3] = true;
    }

    /* new name */
    fcinfo1.arg[4] = CStringGetTextDatum(statement->newname);
    fcinfo1.argnull[4] = false;

    FunctionCallInvoke(&fcinfo1);

    PG_RETURN_NULL();
}
